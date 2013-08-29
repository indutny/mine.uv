#include <assert.h>  /* assert */
#include <stdint.h>  /* uint8_t */
#include <stdlib.h>  /* free */
#include <string.h>  /* memmove */

#include "client.h"
#include "uv.h"
#include "common.h"  /* mc_string_t */
#include "common-private.h"  /* ARRAY_SIZE */
#include "framer.h"  /* mc_framer_t */
#include "openssl/rand.h"  /* RAND_bytes */
#include "parser.h"  /* mc_parser_execute */
#include "server.h"  /* mc_server_t */

static void mc_client__on_close(uv_handle_t* handle);
static uv_buf_t mc_client__on_alloc(uv_handle_t* handle, size_t suggested_size);
static void mc_client__on_read(uv_stream_t* stream,
                               ssize_t nread,
                               uv_buf_t buf);
static void mc_client__cycle(mc_client_t* client);
static int mc_client__handle_handshake(mc_client_t* client, mc_frame_t* frame);
static int mc_client__handle_frame(mc_client_t* client, mc_frame_t* frame);

int mc_client_init(mc_server_t* server, mc_client_t* client) {
  int r;
  client->server = server;
  client->tcp.data = client;

  // TODO(indutny): add timeout
  r = uv_read_start((uv_stream_t*) &client->tcp,
                    mc_client__on_alloc,
                    mc_client__on_read);
  if (r != 0)
    return r;

  r = mc_framer_init(&client->framer);
  if (r != 0)
    return r;

  client->state = kMCInitialState;
  client->encrypted.len = 0;
  client->cleartext.len = 0;

  mc_string_init(&client->username);

  return 0;
}


void mc_client_destroy(mc_client_t* client) {
  uv_read_stop((uv_stream_t*) &client->tcp);
  uv_close((uv_handle_t*) &client->tcp, mc_client__on_close);
  mc_framer_destroy(&client->framer);
  client->encrypted.len = 0;
  client->cleartext.len = 0;
  mc_string_destroy(&client->username);
}


void mc_client__on_close(uv_handle_t* handle) {
  mc_client_t* client;

  client = handle->data;
  free(handle->data);
}


uv_buf_t mc_client__on_alloc(uv_handle_t* handle, size_t suggested_size) {
  mc_client_t* client;

  client = handle->data;
  return uv_buf_init((char*) client->encrypted.data + client->encrypted.len,
                     sizeof(client->encrypted.data) - client->encrypted.len);
}


void mc_client__on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf) {
  mc_client_t* client;

  client = stream->data;

  if (nread > 0) {
    client->encrypted.len += nread;
    mc_client__cycle(client);

    return;
  } else {
    /* nread might be equal to UV_EOF, but who cares */
    mc_client_destroy(client);
  }
}


/*
 * Decipher (if needed) data from `encrypted` and put it into `cleartext`,
 * and run parser over the input
 */
void mc_client__cycle(mc_client_t* client) {
  uint8_t* data;
  ssize_t r;
  ssize_t offset;
  ssize_t len;
  mc_frame_t frame;

  while (client->encrypted.len != 0 || client->cleartext.len != 0) {
    if (client->state == kMCReadyState) {
      /* NOT SUPPORTED YET */
      abort();
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
    if (offset < 0)
      return mc_client_destroy(client);

    /* Not enough data yet */
    if (offset == 0)
      break;

    /* Malformed data */
    if (r < 0)
      return mc_client_destroy(client);

    /* Handle frame */
    if (client->state != kMCReadyState)
      r = mc_client__handle_handshake(client, &frame);
    else
      r = mc_client__handle_frame(client, &frame);
    if (r != 0)
      return mc_client_destroy(client);

    /* Advance parser */
    assert(offset <= len);
    memmove(data, data + offset, len - offset);
    if (data == client->cleartext.data)
      client->cleartext.len -= offset;
    else
      client->encrypted.len -= offset;
  }
}


int mc_client__send_enc_req(mc_client_t* client) {
  int r;

  /* Initialize verify token */
  r = RAND_bytes(client->verify_token, sizeof(client->verify_token));
  if (r != 1)
    return -1;

  mc_string_t server_id;
  mc_string_t public_key;
  mc_string_t token;

  mc_string_set(&server_id,
                client->server->server_id,
                ARRAY_SIZE(client->server->server_id));
  mc_string_set(&public_key,
                (const uint16_t*) client->server->rsa_pub_asn1,
                client->server->rsa_pub_asn1_len);
  mc_string_set(&token,
                client->verify_token,
                ARRAY_SIZE(client->verify_token));

  /* Send encryption key request */
  r = mc_framer_enc_key_req(&client->framer,
                            &server_id,
                            client->server->rsa_pub_asn1,
                            client->server->rsa_pub_asn1_len,
                            client->verify_token,
                            sizeof(client->verify_token));
  if (r != 0)
    return r;

  return mc_framer_send(&client->framer, (uv_stream_t*) &client->tcp);
}


int mc_client__handle_handshake(mc_client_t* client, mc_frame_t* frame) {
  int r;
  switch (client->state) {
    case kMCInitialState:
      if (frame->type != kMCHandshakeType)
        return -1;

      /* Incompatible version */
      if (frame->body.handshake.version != client->server->version)
        return -1;

      /* Persist username */
      r = mc_string_copy(&client->username, &frame->body.handshake.username);
      if (r != 0)
        return r;

      r = mc_client__send_enc_req(client);
      if (r != 0)
        return r;

      client->state = kMCInHandshakeState;
      break;
    case kMCInHandshakeState:
      if (frame->type != kMCEncryptionResType)
        return -1;
      // r = mc_framer_enc_key_res(client, frame);
      if (r != 0)
        return r;
      client->state = kMCLoginState;
      break;
    case kMCLoginState:
      if (frame->type != kMCClientStatus)
        return -1;

      if (frame->body.client_status != kMCInitialSpawnStatus)
        return -1;

      // r = mc_framer_login_req(client);
      if (r != 0)
        return r;
      client->state = kMCReadyState;
      break;
    default:
      return -1;
  }
  return 0;
}


int mc_client__handle_frame(mc_client_t* client, mc_frame_t* frame) {
  return 0;
}
