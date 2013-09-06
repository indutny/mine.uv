#include <assert.h>  /* assert */
#include <stdint.h>  /* uint8_t */
#include <stdlib.h>  /* free */
#include <string.h>  /* memmove, strlen */

#include "client.h"
#include "client-private.h"
#include "uv.h"
#include "common.h"  /* mc_string_t */
#include "common-private.h"  /* container_of */
#include "openssl/evp.h"  /* EVP_* */
#include "protocol/framer.h"  /* mc_framer_t */
#include "protocol/parser.h"  /* mc_parser_execute */
#include "server.h"  /* mc_server_t */

static void mc_client__on_close(uv_handle_t* handle);
static void mc_client__on_timeout(uv_timer_t* handle, int status);
static uv_buf_t mc_client__on_alloc(uv_handle_t* handle, size_t suggested_size);
static void mc_client__on_read(uv_stream_t* stream,
                               ssize_t nread,
                               uv_buf_t buf);
static int mc_client__restart_timer(mc_client_t* client);
static void mc_client__cycle(mc_client_t* client);
static int mc_client__send_kick(mc_client_t* client, const char* reason);
static void mc_client__after_kick(mc_framer_t* framer, int status);

mc_client_t* mc_client_new(mc_server_t* server) {
  int r;
  mc_client_t* client;

  client = malloc(sizeof(*client));
  if (client == NULL)
    return NULL;

  /* Store reference to the server */
  client->server = server;

  /* Number of handles waiting for close event */
  client->close_await = 0;

  /* Create read timeout's timer */
  r = uv_timer_init(server->loop, &client->timeout);
  if (r != 0)
    goto timer_init_failed;
  client->timeout.data = client;

  /* NOTE: Its safe to call it here */
  r = mc_client__restart_timer(client);
  if (r != 0)
    goto tcp_init_failed;

  r = uv_tcp_init(server->loop, &client->tcp);
  if (r != 0)
    goto tcp_init_failed;
  client->tcp.data = client;

  /* Do not use Nagle algorithm */
  r = uv_tcp_nodelay(&client->tcp, 1);
  if (r != 0)
    goto nodelay_failed;

  r = uv_accept((uv_stream_t*) server->tcp, (uv_stream_t*) &client->tcp);
  if (r != 0)
    goto nodelay_failed;

  r = uv_read_start((uv_stream_t*) &client->tcp,
                    mc_client__on_alloc,
                    mc_client__on_read);
  if (r != 0)
    goto nodelay_failed;

  r = mc_framer_init(&client->framer);
  if (r != 0)
    goto nodelay_failed;

  client->verify = mc_session_verify_new(client);
  client->verified = 0;
  if (client->verify == NULL)
    goto verify_new_failed;

  client->destroyed = 0;
  client->state = kMCInitialState;
  client->encrypted.len = 0;
  client->cleartext.len = 0;

  mc_string_init(&client->username);
  client->ascii_username = NULL;
  client->ascii_username_len = 0;
  client->api_hash_len = 0;
  client->secret = NULL;
  client->secret_len = 0;

  EVP_CIPHER_CTX_init(&client->aes_in);
  EVP_CIPHER_CTX_init(&client->aes_out);

  /*
   * I know its quite late, but its better than pressing
   * session server and doing AES just to figure out that there was no room
   * for the client
   */
  r = mc_client__client_limit(client);
  if (r != 0)
    goto client_limit_failed;

  return client;

client_limit_failed:
  mc_session_verify_destroy(client->verify);
  client->verify = NULL;

verify_new_failed:
  mc_framer_destroy(&client->framer);

nodelay_failed:
  if (client->close_await == 0)
    client->close_await = 2;
  uv_close((uv_handle_t*) &client->tcp, mc_client__on_close);

tcp_init_failed:
  if (client->close_await == 0)
    client->close_await = 1;
  uv_close((uv_handle_t*) &client->timeout, mc_client__on_close);

timer_init_failed:
  client->server = NULL;
  if (client->close_await == 0)
    free(client);
  return NULL;
}


void mc_client_destroy(mc_client_t* client, const char* reason) {
  /* Regardless of config, stop reading data */
  uv_read_stop((uv_stream_t*) &client->tcp);

  /* Kick client gracefully */
  if (reason != NULL && mc_client__send_kick(client, reason) == 0)
    return;

  if (client->destroyed)
    return;
  client->destroyed = 1;

  /* Decrement number of connections */
  if (client->state == kMCReadyState)
    client->server->clients--;

  client->close_await = 2;
  uv_close((uv_handle_t*) &client->tcp, mc_client__on_close);
  uv_close((uv_handle_t*) &client->timeout, mc_client__on_close);
  mc_framer_destroy(&client->framer);
  client->encrypted.len = 0;
  client->cleartext.len = 0;
  mc_string_destroy(&client->username);

  /* NOTE: We might destroy verify on response to lower memory usage */
  if (client->verify != NULL)
    mc_session_verify_destroy(client->verify);
  client->verify = NULL;

  free(client->ascii_username);
  client->ascii_username = NULL;
  client->ascii_username_len = 0;
  client->api_hash_len = 0;

  free(client->secret);
  client->secret = NULL;

  EVP_CIPHER_CTX_cleanup(&client->aes_in);
  EVP_CIPHER_CTX_cleanup(&client->aes_out);
}


void mc_client__on_close(uv_handle_t* handle) {
  mc_client_t* client;

  client = handle->data;
  handle->data = NULL;

  if (--client->close_await == 0)
    free(client);
}


void mc_client__on_timeout(uv_timer_t* handle, int status) {
  mc_client_t* client;

  client = container_of(handle, mc_client_t, timeout);
  if (status == 0)
    mc_client__send_kick(client, "Connection timed out");
}


uv_buf_t mc_client__on_alloc(uv_handle_t* handle, size_t suggested_size) {
  mc_client_t* client;

  client = container_of(handle, mc_client_t, tcp);
  return uv_buf_init((char*) client->encrypted.data + client->encrypted.len,
                     sizeof(client->encrypted.data) - client->encrypted.len);
}


void mc_client__on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf) {
  int r;
  mc_client_t* client;

  client = container_of(stream, mc_client_t, tcp);

  if (nread > 0) {
    client->encrypted.len += nread;

    mc_client__cycle(client);

    /* Buffer is full, stop reading unitl processing */
    if (client->encrypted.len == sizeof(client->encrypted.data)) {
      r = uv_read_stop(stream);
      if (r != 0)
        mc_client_destroy(client, NULL);
    }

    return;
  } else {
    /* nread might be equal to UV_EOF, but who cares */
    mc_client_destroy(client, NULL);
  }
}


int mc_client__restart_timer(mc_client_t* client) {
  int r;

  if (client->server->config.client_timeout == 0)
    return 0;

  r = uv_timer_stop(&client->timeout);
  if (r != 0)
    return -1;

  r = uv_timer_start(&client->timeout,
                     mc_client__on_timeout,
                     client->server->config.client_timeout,
                     0);
  return r;
}


int mc_client__client_limit(mc_client_t* client) {
  if (client->server->clients != 0 &&
      client->server->clients == client->server->config.max_clients) {
    mc_client_destroy(client, "Maximum connections limit reached");
    return -1;
  }
  return 0;
}


/*
 * Decipher (if needed) data from `encrypted` and put it into `cleartext`,
 * and run parser over the input
 */
void mc_client__cycle(mc_client_t* client) {
  uint8_t* data;
  size_t block_size;
  int avail;
  int is_full;
  ssize_t r;
  ssize_t offset;
  ssize_t len;
  mc_frame_t frame;

  is_full = client->encrypted.len == sizeof(client->encrypted.data);

  while (client->encrypted.len != 0 || client->cleartext.len != 0) {
    if (client->secret_len != 0) {
      /* If there's enough encrypted input, and enough space in cleartext */
      block_size = EVP_CIPHER_CTX_block_size(&client->aes_in);
      if (client->encrypted.len >= block_size) {
        /* Get amount of data available for write in cleartext */
        avail = sizeof(client->cleartext.data) - client->cleartext.len;
        if ((size_t) avail < client->encrypted.len + block_size)
          break;

        r = EVP_DecryptUpdate(&client->aes_in,
                              client->cleartext.data + client->cleartext.len,
                              &avail,
                              client->encrypted.data,
                              client->encrypted.len);
        if (r != 1)
          return mc_client_destroy(client, "Decryption failed");
        client->cleartext.len += avail;
        assert((size_t) client->cleartext.len <=
               sizeof(client->cleartext.data));
      }

      /* All written */
      client->encrypted.len = 0;

      data = client->cleartext.data;
      len = client->cleartext.len;
    } else {
      data = client->encrypted.data;
      len = client->encrypted.len;
    }

    /*
     * Parse one frame, note that frame has the same lifetime as data, and
     * directly depends on it
     */
    offset = mc_parser_execute(data, len, &frame);

    /* Parse error */
    if (offset < 0) {
      char err[128];
      if (len >= 1) {
        snprintf(err, sizeof(err), "Failed to parse frame: %02x", data[0]);
        return mc_client_destroy(client, err);
      } else {
        return mc_client_destroy(client, "Frame OOB");
      }
    }

    /* Not enough data yet */
    if (offset == 0)
      break;

    r = mc_client__restart_timer(client);
    if (r != 0)
      return mc_client_destroy(client, "Failed to restart timeout timer");

    /* Handle frame */
    if (frame.type == kMCServerListPingType || frame.type == kMCPluginMsgType)
      /* Just ignore */
      r = 0;
    else if (client->state != kMCReadyState)
      r = mc_client__handle_handshake(client, &frame);
    else
      r = mc_client__handle_frame(client, &frame);

    if (r != 0) {
      char err[128];
      snprintf(err, sizeof(err), "Failed to handle frame: %02x", frame.type);
      return mc_client_destroy(client, err);
    }

    /* Advance parser */
    assert(offset <= len);
    if (offset != 0)
      memmove(data, data + offset, len - offset);
    if (data == client->cleartext.data)
      client->cleartext.len -= offset;
    else
      client->encrypted.len -= offset;
  }

  /*
   * If encrypted was full and not has some space inside -
   * we can safely re-enable reading from socket
   */
  if (is_full && client->encrypted.len < sizeof(client->encrypted.data)) {
    r = uv_read_start((uv_stream_t*) &client->tcp,
                      mc_client__on_alloc,
                      mc_client__on_read);
    if (r != 0)
      return mc_client_destroy(client, NULL);
  }

  return;
}


int mc_client__send_kick(mc_client_t* client, const char* reason) {
  int r;
  mc_string_t mc_reason;

  mc_string_init(&mc_reason);
  r = mc_string_from_ascii(&mc_reason, reason);
  if (r != 0)
    return r;

  r = mc_framer_kick(&client->framer, &mc_reason);

  /* Destroy string regardless of result */
  mc_string_destroy(&mc_reason);

  if (r != 0)
    return r;

  return mc_framer_send(&client->framer,
                        (uv_stream_t*) &client->tcp,
                        mc_client__after_kick);
}


void mc_client__after_kick(mc_framer_t* framer, int status) {
  mc_client_t* client;

  client = container_of(framer, mc_client_t, framer);
  mc_client_destroy(client, NULL);
}
