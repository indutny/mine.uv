#include <arpa/inet.h>  /* ntohs, ntohl */
#include <stdlib.h>  /* malloc, free, realloc */
#include <string.h>  /* memcpy */

#include "protocol/framer.h"
#include "uv.h"  /* uv_write */
#include "utils/common.h"  /* mc_frame_t */
#include "utils/common-private.h"  /* container_of */
#include "utils/encoder.h"  /* mc_encoder_t */
#include "openssl/evp.h"  /* EVP_* */

#define WRITE(framer, t, v) MC_ENCODER_WRITE(&(framer)->encoder, t, v)
#define WRITE_RAW(framer, d, l) MC_ENCODER_WRITE_DATA(&(framer)->encoder, d, l)

typedef struct mc_framer__req_s mc_framer__req_t;

struct mc_framer__req_s {
  uv_write_t req;
  mc_framer_t* framer;
  mc_framer_send_cb_t cb;

  /* Not really necessary, but might be useful for debugging */
  int len;
};

static void mc_framer__after_send(uv_write_t* req, int status);


int mc_framer_init(mc_framer_t* framer) {
  int r;

  r = mc_encoder_init(&framer->encoder, 0);
  if (r != 0)
    return r;
  framer->aes = NULL;

  return 0;
}


void mc_framer_destroy(mc_framer_t* framer) {
  mc_encoder_destroy(&framer->encoder);
  framer->aes = NULL;
}


void mc_framer_use_aes(mc_framer_t* framer, EVP_CIPHER_CTX* aes) {
  framer->aes = aes;
}


int mc_framer_send(mc_framer_t* framer,
                   uv_stream_t* stream,
                   mc_framer_send_cb_t cb) {
  int r;
  int aes_len;
  mc_framer__req_t* req;
  uv_buf_t buf;
  char* data;
  int packet_len;

  packet_len = mc_encoder_len(&framer->encoder);

  /* Account space for possible AES padding */
  if (framer->aes != NULL)
    packet_len += EVP_CIPHER_CTX_block_size(framer->aes) - 1;

  req = malloc(sizeof(*req) + packet_len);
  if (req == NULL)
    return -1;

  req->framer = framer;
  req->cb = cb;
  req->len = packet_len;

  data = ((char*) req) + sizeof(*req);
  if (framer->aes == NULL) {
    memcpy(data, mc_encoder_data(&framer->encoder), req->len);
  } else {
    aes_len = req->len;
    r = EVP_EncryptUpdate(framer->aes,
                          (unsigned char*) data,
                          &aes_len,
                          mc_encoder_data(&framer->encoder),
                          mc_encoder_len(&framer->encoder));
    if (r != 1) {
      free(req);
      return -1;
    }
    req->len = aes_len;
  }

  buf = uv_buf_init(data, req->len);
  r = uv_write(&req->req, stream, &buf, 1, mc_framer__after_send);

  /* Clear state */
  mc_encoder_reset(&framer->encoder);

  return r;
}


void mc_framer__after_send(uv_write_t* req, int status) {
  mc_framer__req_t* freq;

  freq = container_of(req, mc_framer__req_t, req);
  if (freq->cb != NULL)
    freq->cb(freq->framer, status);

  free(freq);
}


int mc_framer_enc_key_req(mc_framer_t* framer,
                          mc_string_t* server_id,
                          const unsigned char* public_key,
                          uint16_t public_key_len,
                          const unsigned char* token,
                          uint16_t token_len) {
  WRITE(framer, u8, kMCEncryptionReqType);
  WRITE(framer, string, server_id);
  WRITE(framer, u16, public_key_len);
  WRITE_RAW(framer, public_key, public_key_len);
  WRITE(framer, u16, token_len);
  WRITE_RAW(framer, token, token_len);

  return 0;
}


int mc_framer_enc_key_res(mc_framer_t* framer,
                          const unsigned char* secret,
                          uint16_t secret_len,
                          const unsigned char* token,
                          uint16_t token_len) {
  WRITE(framer, u8, kMCEncryptionResType);
  WRITE(framer, u16, secret_len);
  WRITE_RAW(framer, secret, secret_len);
  WRITE(framer, u16, token_len);
  WRITE_RAW(framer, token, token_len);

  return 0;
}


int mc_framer_login_req(mc_framer_t* framer,
                        uint32_t entity_id,
                        mc_string_t* level_type,
                        uint8_t mode,
                        int8_t dimension,
                        uint8_t difficulty,
                        uint8_t max_players) {
  WRITE(framer, u8, kMCLoginReqType);
  WRITE(framer, u32, entity_id);
  WRITE(framer, string, level_type);
  WRITE(framer, u8, mode);
  WRITE(framer, u8, (uint8_t) dimension);
  WRITE(framer, u8, difficulty);
  WRITE(framer, u8, 0);
  WRITE(framer, u8, max_players);

  return 0;
}


int mc_framer_kick(mc_framer_t* framer, mc_string_t* reason) {
  WRITE(framer, u8, kMCKickType);
  WRITE(framer, string, reason);

  return 0;
}
