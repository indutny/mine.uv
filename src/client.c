#include <assert.h>  /* assert */
#include <stdint.h>  /* uint8_t */
#include <stdlib.h>  /* free */
#include <string.h>  /* memmove, strlen */

#include "client.h"
#include "uv.h"
#include "common.h"  /* mc_string_t */
#include "common-private.h"  /* ARRAY_SIZE */
#include "framer.h"  /* mc_framer_t */
#include "openssl/evp.h"  /* EVP_* */
#include "openssl/rand.h"  /* RAND_bytes */
#include "parser.h"  /* mc_parser_execute */
#include "server.h"  /* mc_server_t */

static void mc_client__on_close(uv_handle_t* handle);
static uv_buf_t mc_client__on_alloc(uv_handle_t* handle, size_t suggested_size);
static void mc_client__on_read(uv_stream_t* stream,
                               ssize_t nread,
                               uv_buf_t buf);
static void mc_client__cycle(mc_client_t* client);
static int mc_client__send_kick(mc_client_t* client, const char* reason);
static void mc_client__after_kick(mc_framer_t* framer, int status);
static int mc_client__send_enc_req(mc_client_t* client);
static int mc_client__check_enc_res(mc_client_t* client, mc_frame_t* frame);
static int mc_client__compute_api_hash(mc_client_t* client);
static int mc_client__handle_handshake(mc_client_t* client, mc_frame_t* frame);
static int mc_client__handle_frame(mc_client_t* client, mc_frame_t* frame);

mc_client_t* mc_client_init(mc_server_t* server) {
  int r;
  mc_client_t* client;

  client = malloc(sizeof(*client));
  if (client == NULL)
    return NULL;

  r = uv_tcp_init(server->loop, &client->tcp);
  if (r != 0)
    goto fatal;

  /* Do not use Nagle algorithm */
  r = uv_tcp_nodelay(&client->tcp, 1);
  if (r != 0)
    goto fatal;

  r = uv_accept((uv_stream_t*) &server->tcp, (uv_stream_t*) &client->tcp);
  if (r != 0)
    goto fatal;

  client->server = server;

  /* TODO(indutny): add timeout */
  r = uv_read_start((uv_stream_t*) &client->tcp,
                    mc_client__on_alloc,
                    mc_client__on_read);
  if (r != 0)
    goto read_start_failed;

  r = mc_framer_init(&client->framer);
  if (r != 0)
    goto fatal;

  client->destroyed = 0;
  client->state = kMCInitialState;
  client->encrypted.len = 0;
  client->cleartext.len = 0;

  mc_string_init(&client->username);
  client->ascii_username = NULL;
  client->secret = NULL;
  client->secret_len = 0;

  return client;

read_start_failed:
  uv_close((uv_handle_t*) &client->tcp, mc_client__on_close);

fatal:
  free(client);
  return NULL;
}


void mc_client_destroy(mc_client_t* client, const char* reason) {
  /* Regardless of config, stop reading data */
  uv_read_stop((uv_stream_t*) &client->tcp);

  if (client->destroyed)
    return;
  client->destroyed = 1;

  /* Kick client gracefully */
  if (reason != NULL && mc_client__send_kick(client, reason) == 0)
    return;

  uv_close((uv_handle_t*) &client->tcp, mc_client__on_close);
  mc_framer_destroy(&client->framer);
  client->encrypted.len = 0;
  client->cleartext.len = 0;
  mc_string_destroy(&client->username);
  free(client->ascii_username);
  client->ascii_username = NULL;
  free(client->secret);
  client->secret = NULL;
}


void mc_client__on_close(uv_handle_t* handle) {
  mc_client_t* client;

  client = container_of(handle, mc_client_t, tcp);
  free(client);
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

    /* Handle frame */
    if (client->state != kMCReadyState)
      r = mc_client__handle_handshake(client, &frame);
    else
      r = mc_client__handle_frame(client, &frame);
    if (r != 0)
      return mc_client_destroy(client, "Failed to handle frame");

    /* Advance parser */
    assert(offset <= len);
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


int mc_client__send_enc_req(mc_client_t* client) {
  int r;

  /* Initialize verify token */
  r = RAND_bytes(client->verify_token, sizeof(client->verify_token));
  if (r != 1)
    return -1;

  mc_string_t server_id;

  mc_string_set(&server_id,
                client->server->server_id,
                ARRAY_SIZE(client->server->server_id));

  /* Send encryption key request */
  r = mc_framer_enc_key_req(&client->framer,
                            &server_id,
                            client->server->rsa_pub_asn1,
                            client->server->rsa_pub_asn1_len,
                            client->verify_token,
                            sizeof(client->verify_token));
  if (r != 0)
    return r;

  return mc_framer_send(&client->framer, (uv_stream_t*) &client->tcp, NULL);
}


int mc_client__check_enc_res(mc_client_t* client, mc_frame_t* frame) {
  int r;
  int max_len;
  unsigned char* out;
  const EVP_CIPHER* cipher;

  max_len = RSA_size(client->server->rsa);
  out = malloc(max_len);
  if (out == NULL)
    return -1;

  r = RSA_private_decrypt(frame->body.enc_resp.token_len,
                          frame->body.enc_resp.token,
                          out,
                          client->server->rsa,
                          RSA_PKCS1_PADDING);
  if (r < 0)
    return r;

  /* Verify that token is the same */
  if (r != sizeof(client->verify_token))
    return -1;
  if (memcmp(out, client->verify_token, r) != 0)
    return -1;

  r = RSA_private_decrypt(frame->body.enc_resp.secret_len,
                          frame->body.enc_resp.secret,
                          out,
                          client->server->rsa,
                          RSA_PKCS1_PADDING);
  if (r < 0)
    return r;

  client->secret = out;
  client->secret_len = r;

  /* Init AES keys */
  switch (client->secret_len * 8) {
    case 128:
      cipher = EVP_aes_128_cfb8();
      break;
    case 192:
      cipher = EVP_aes_192_cfb8();
      break;
    case 256:
      cipher = EVP_aes_256_cfb8();
      break;
    default:
      cipher = NULL;
      break;
  }
  if (cipher == NULL)
    return -1;
  EVP_CIPHER_CTX_init(&client->aes_in);
  EVP_CIPHER_CTX_init(&client->aes_out);

  r = EVP_DecryptInit(&client->aes_in, cipher, client->secret, client->secret);
  if (r != 1)
    return -1;
  r = EVP_EncryptInit(&client->aes_out, cipher, client->secret, client->secret);
  if (r != 1)
    return -1;

  r = mc_client__compute_api_hash(client);
  if (r != 0)
    return r;

  /* Send enc key response with empty payload */
  r = mc_framer_enc_key_res(&client->framer, NULL, 0, NULL, 0);
  if (r != 0)
    return r;

  r = mc_framer_send(&client->framer, (uv_stream_t*) &client->tcp, NULL);
  if (r != 0)
    return r;

  mc_framer_use_aes(&client->framer, &client->aes_out);
  return 0;
}


/* TODO(indutny): fix it */
int mc_client__compute_api_hash(mc_client_t* client) {
  int r;
  int i;
  int sign_change;
  int carry;
  unsigned char hash_out[EVP_MAX_MD_SIZE];
  unsigned int s;
  char* api_hash;

  EVP_MD_CTX sha;
  EVP_MD_CTX_init(&sha);

  r = EVP_DigestInit(&sha, EVP_sha1());
  if (r != 1)
    return -1;

  r = EVP_DigestUpdate(&sha,
                       client->server->ascii_server_id,
                       sizeof(client->server->ascii_server_id));
  if (r != 1)
    goto final;

  r = EVP_DigestUpdate(&sha, client->secret, client->secret_len);
  if (r != 1)
    goto final;

  r = EVP_DigestUpdate(&sha,
                       client->server->rsa_pub_asn1,
                       client->server->rsa_pub_asn1_len);
  if (r != 1)
    goto final;

  r = EVP_DigestFinal(&sha, hash_out, &s);
  if (r != 1)
    goto final;

  assert(s > 0 && s <= sizeof(hash_out));

  api_hash = client->api_hash;
  sign_change = hash_out[0] >= 0x80 ? 1 : 0;

  // Change sign and add `1`
  if (sign_change) {
    api_hash[0] = '-';
    api_hash++;
    carry = 1;
    for (i = s - 1; i >= 0; i--) {
      if (carry == 1) {
        if (hash_out[i] == 0x0)
          continue;
        else
          hash_out[i]--;
        carry = 0;
      }
      hash_out[i] = ~hash_out[i];
    }
  }

  /* Convert hash to ascii */
  for (i = 0; i < (int) s; i++)
    snprintf(api_hash + i * 2, (s - i) * 2 + 1, "%02x", hash_out[i]);

  /* Skip leading zeroes */
  for (i = 0; i < (int) s * 2; i++)
    if (api_hash[i] != '0')
      break;
  if (i != 0)
    memmove(api_hash, api_hash + i, s * 2 - i + 1);

final:
  EVP_MD_CTX_cleanup(&sha);
  return r == 1 ? 0 : -1;
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

      client->ascii_username = mc_string_to_ascii(&client->username);
      if (client->ascii_username == NULL)
        return -1;

      r = mc_client__send_enc_req(client);
      if (r != 0)
        return r;

      client->state = kMCInHandshakeState;
      break;
    case kMCInHandshakeState:
      if (frame->type != kMCEncryptionResType)
        return -1;

      r = mc_client__check_enc_res(client, frame);
      if (r != 0)
        return r;

      client->state = kMCLoginState;
      break;
    case kMCLoginState:
      if (frame->type != kMCClientStatusType)
        return -1;

      if (frame->body.client_status != kMCInitialSpawnStatus)
        return -1;

      r = mc_framer_login_req(&client->framer,
                              1,
                              &client->username,
                              0,
                              0,
                              0,
                              100);
      if (r != 0)
        return r;

      r = mc_framer_send(&client->framer, (uv_stream_t*) &client->tcp, NULL);
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
