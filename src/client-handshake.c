#include <assert.h>  /* assert */
#include <stdlib.h>  /* free */
#include <string.h>  /* memmove, strlen */

#include "client.h"
#include "client-private.h"
#include "uv.h"
#include "openssl/evp.h"  /* EVP_* */
#include "openssl/rand.h"  /* RAND_bytes */
#include "openssl/rsa.h"  /* RSA_* */
#include "protocol/framer.h"  /* mc_framer_t */
#include "server.h"  /* mc_server_t */
#include "session.h"  /* mc_session_verify_t */
#include "utils/string.h"  /* mc_string_t */
#include "utils/common-private.h"  /* ARRAY_SIZE */

static int mc_client__send_enc_req(mc_client_t* client);
static int mc_client__check_enc_res(mc_client_t* client, mc_frame_t* frame);
static int mc_client__compute_api_hash(mc_client_t* client);
static void mc_client__verify_cb(mc_client_t* client,
                                 mc_session_verify_status_t status);
static int mc_client__finish_login(mc_client_t* client);


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

  r = EVP_DecryptInit(&client->aes_in, cipher, client->secret, client->secret);
  if (r != 1)
    return -1;
  r = EVP_EncryptInit(&client->aes_out, cipher, client->secret, client->secret);
  if (r != 1)
    return -1;

  r = mc_client__compute_api_hash(client);
  if (r != 0)
    return r;

  /* Send verifying request to server */
  mc_session_verify(client->verify, mc_client__verify_cb);

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

  /* Change sign and add `1` */
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
  client->api_hash_len = s * 2;
  for (i = 0; i < (int) s; i++) {
    snprintf(api_hash + i * 2,
             client->api_hash_len - i * 2 + 1,
             "%02x",
             hash_out[i]);
  }

  /* Skip leading zeroes */
  for (i = 0; i < (int) s * 2; i++)
    if (api_hash[i] != '0')
      break;
  if (i != 0) {
    client->api_hash_len -= i;
    memmove(api_hash, api_hash + i, client->api_hash_len + 1);
  }
  client->api_hash_len += sign_change;

final:
  EVP_MD_CTX_cleanup(&sha);
  return r == 1 ? 0 : -1;
}


void mc_client__verify_cb(mc_client_t* client,
                          mc_session_verify_status_t status) {
  assert(client->verify != NULL);
  mc_session_verify_destroy(client->verify);
  client->verify = NULL;

  client->verified = status == kMCVerifyOk;
  if (client->verified) {
    if (client->state == kMCAwaitsVerification) {
      mc_client__finish_login(client);
    } else {
      /* Or we'll do it in mc_client__handle_handshake */
    }
  } else {
    char err[128];
    snprintf(err, sizeof(err), "Failed to verify user identity: %02x", status);
    return mc_client_destroy(client, err);
  }
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
      client->ascii_username_len = strlen(client->ascii_username);

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

      if (client->verified)
        return mc_client__finish_login(client);
      else
        client->state = kMCAwaitsVerification;
      break;
    default:
      return -1;
  }
  return 0;
}


int mc_client__finish_login(mc_client_t* client) {
  int r;

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

  r = mc_client__client_limit(client);
  if (r != 0)
    return r;

  client->server->clients++;
  client->state = kMCReadyState;

  return 0;
}
