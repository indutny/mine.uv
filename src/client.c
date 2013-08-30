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
static int mc_client__send_enc_req(mc_client_t* client);
static int mc_client__check_enc_res(mc_client_t* client, mc_frame_t* frame);
static int mc_client__compute_api_hash(mc_client_t* client);
static int mc_client__handle_handshake(mc_client_t* client, mc_frame_t* frame);
static int mc_client__handle_frame(mc_client_t* client, mc_frame_t* frame);

int mc_client_init(mc_server_t* server, mc_client_t* client) {
  int r;
  client->server = server;
  client->tcp.data = client;

  /* TODO(indutny): add timeout */
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
  client->ascii_username = NULL;
  client->secret = NULL;
  client->secret_len = 0;

  return 0;
}


void mc_client_destroy(mc_client_t* client) {
  uv_read_stop((uv_stream_t*) &client->tcp);
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
  int block_size;
  int avail;
  ssize_t r;
  ssize_t offset;
  ssize_t len;
  mc_frame_t frame;

  while (client->encrypted.len != 0 || client->cleartext.len != 0) {
    if (client->secret_len != 0) {
      /* If there's enough encrypted input, and enough space in cleartext */
      block_size = EVP_CIPHER_CTX_block_size(&client->aes_in);
      while (client->encrypted.len >= block_size) {
        /* Get amount of data available for write in cleartext */
        avail = sizeof(client->cleartext.data) - client->cleartext.len;
        if (avail < client->encrypted.len + block_size)
          break;

        r = EVP_DecryptUpdate(&client->aes_in,
                              client->cleartext.data + client->cleartext.len,
                              &avail,
                              client->encrypted.data,
                              client->encrypted.len);
        if (r != 1)
          return mc_client_destroy(client);
        client->cleartext.len += avail;
        assert((size_t) client->cleartext.len <=
               sizeof(client->cleartext.data));
      }

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

  return mc_framer_send(&client->framer, (uv_stream_t*) &client->tcp);
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

  /* Send enc key response with empty payload */
  r = mc_framer_enc_key_res(&client->framer, NULL, 0, NULL, 0);
  if (r != 0)
    return r;

  r = mc_client__compute_api_hash(client);
  if (r != 0)
    return r;

  return mc_framer_send(&client->framer, (uv_stream_t*) &client->tcp);
}


/* TODO(indutny): fix it */
int mc_client__compute_api_hash(mc_client_t* client) {
  int r;
  int i;
  int carry;
  int sign_change;
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
      hash_out[i] = ~hash_out[i];
      if (carry) {
        if (hash_out[i] == 0xff) {
          hash_out[i] = 0;
        } else {
          hash_out[i]++;
          carry = 1;
        }
      }
    }
  }

  /* Convert hash to ascii */
  for (i = 0; i < (int) s; i++) {
    snprintf(api_hash + i * 2, (s - i) * 2, "%02x", hash_out[i]);
  }
  api_hash[i * 2] = 0;

  /* Skip leading zeroes */
  for (i = 0; i < (int) s * 2; i++)
    if (api_hash[i] != '0')
      break;
  if (i != sign_change)
    memmove(api_hash, api_hash + i, s* 2 - i + 1);

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
      if (frame->type != kMCClientStatus)
        return -1;

      if (frame->body.client_status != kMCInitialSpawnStatus)
        return -1;

      /* r = mc_framer_login_req(client); */
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
