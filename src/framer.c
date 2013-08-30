#include <arpa/inet.h>  /* ntohs, ntohl */
#include <stdlib.h>  /* malloc, free, realloc */
#include <sys/types.h>  /* ssize_t */

#include "framer.h"
#include "uv.h"  /* uv_write */
#include "common.h"  /* mc_frame_t */
#include "common-private.h"  /* container_of */
#include "openssl/evp.h"  /* EVP_* */

#define GROW(framer, size) \
    do { \
      r = mc_framer__check_grow((framer), (size)); \
      if (r != 0) \
        return r; \
    } while (0)

#define WRITE(framer, type, v) \
    do { \
      r = mc_framer__write_##type((framer), (v)); \
      if (r != 0) \
        return r; \
    } while (0)


#define WRITE_RAW(framer, data, size) \
    do { \
      r = mc_framer__write_raw((framer), (data), (size)); \
      if (r != 0) \
        return r; \
    } while (0)

typedef struct mc_framer__req_s mc_framer__req_t;

struct mc_framer__req_s {
  uv_write_t req;
  mc_framer_t* framer;
  mc_framer_send_cb_t cb;

  /* Not really necessary, but might be useful for debugging */
  ssize_t len;
};

static void mc_framer__after_send(uv_write_t* req, int status);
static int mc_framer__check_grow(mc_framer_t* framer, ssize_t size);
static int mc_framer__write_u8(mc_framer_t* framer, uint8_t v);
static int mc_framer__write_u16(mc_framer_t* framer, uint16_t v);
static int mc_framer__write_u32(mc_framer_t* framer, uint32_t v);
static int mc_framer__write_string(mc_framer_t* framer, mc_string_t* str);
static int mc_framer__write_raw(mc_framer_t* framer,
                                const unsigned char* data,
                                size_t size);

static const int kFramerInitialLen = 1024;


int mc_framer_init(mc_framer_t* framer) {
  framer->data = malloc(kFramerInitialLen);
  if (framer->data == NULL)
    return -1;
  framer->offset = 0;
  framer->len = kFramerInitialLen;
  framer->aes = NULL;

  return 0;
}


void mc_framer_destroy(mc_framer_t* framer) {
  free(framer->data);
  framer->data = NULL;
  framer->offset = 0;
  framer->len = 0;
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

  packet_len = framer->offset;

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
    memcpy(data, framer->data, req->len);
  } else {
    aes_len = req->len;
    r = EVP_EncryptUpdate(framer->aes,
                          (unsigned char*) data,
                          &aes_len,
                          framer->data,
                          framer->offset);
    if (r != 1) {
      free(req);
      return -1;
    }
    req->len = aes_len;
  }

  buf = uv_buf_init(data, req->len);
  r = uv_write(&req->req, stream, &buf, 1, mc_framer__after_send);

  /* Clear state */
  framer->offset = 0;

  return r;
}


void mc_framer__after_send(uv_write_t* req, int status) {
  mc_framer__req_t* freq;

  freq = container_of(req, mc_framer__req_t, req);
  if (freq->cb != NULL)
    freq->cb(freq->framer, status);

  free(freq);
}


int mc_framer__check_grow(mc_framer_t* framer, ssize_t size) {
  if (framer->offset + size <= framer->len)
    return 0;

  /* Allocation required */
  framer->len += kFramerInitialLen;
  framer->data = realloc(framer->data, framer->len);
  if (framer->data == NULL)
    return -1;

  return 0;
}


int mc_framer__write_u8(mc_framer_t* framer, uint8_t v) {
  int r;
  GROW(framer, sizeof(v));

  framer->data[framer->offset] = v;
  framer->offset += sizeof(v);

  return 0;
}


int mc_framer__write_u16(mc_framer_t* framer, uint16_t v) {
  int r;
  GROW(framer, sizeof(v));

  *(uint16_t*) (framer->data + framer->offset) = htons(v);
  framer->offset += sizeof(v);

  return 0;
}


int mc_framer__write_u32(mc_framer_t* framer, uint32_t v) {
  int r;
  GROW(framer, sizeof(v));

  *(uint32_t*) (framer->data + framer->offset) = htonl(v);
  framer->offset += sizeof(v);

  return 0;
}


int mc_framer__write_raw(mc_framer_t* framer,
                         const unsigned char* data,
                         size_t size) {
  int r;

  GROW(framer, size);
  memcpy(framer->data + framer->offset, data, size);
  framer->offset += size;

  return 0;
}


int mc_framer__write_string(mc_framer_t* framer, mc_string_t* str) {
  int r;
  uint16_t len;

  len = str->len;
  GROW(framer, sizeof(len) + len * 2);
  WRITE(framer, u16, len);
  WRITE_RAW(framer, (const unsigned char*) str->data, len * 2);

  return 0;
}


int mc_framer_enc_key_req(mc_framer_t* framer,
                          mc_string_t* server_id,
                          const unsigned char* public_key,
                          uint16_t public_key_len,
                          const unsigned char* token,
                          uint16_t token_len) {
  int r;

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
  int r;

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
  int r;

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
  int r;

  WRITE(framer, u8, kMCKickType);
  WRITE(framer, string, reason);

  return 0;
}
