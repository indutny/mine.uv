#include <arpa/inet.h>  /* ntohs, ntohl */
#include <assert.h>  /* assert */
#include <stdint.h>  /* uint8_t */
#include <stdlib.h>  /* abort */
#include <sys/types.h>  /* ssize_t */

#include "parser.h"

#define PARSE_READ(p, type, out) \
    do { \
      r = mc_parser__read_##type((p), (out)); \
      if (r <= 0) \
        return r; \
    } while (0)

#define PARSE_READ_RAW(p, out, size) \
    do { \
      r = mc_parser__read_raw((p), (out), (size)); \
      if (r <= 0) \
        return r; \
    } while (0)

typedef struct mc_parser_s mc_parser_t;

static int mc_parser__read_u8(mc_parser_t* p, uint8_t* out);
static int mc_parser__read_u16(mc_parser_t* p, uint16_t* out);
static int mc_parser__read_u32(mc_parser_t* p, uint32_t* out);
static int mc_parser__read_string(mc_parser_t* p, mc_string_t* str);
static int mc_parser__read_raw(mc_parser_t* p,
                               unsigned char** out,
                               size_t size);
static int mc_parser__parse_handshake(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_enc_resp(mc_parser_t* p, mc_frame_t* frame);

struct mc_parser_s {
  const unsigned char* data;
  size_t offset;
  size_t len;
};


int mc_parser_execute(uint8_t* data, ssize_t len, mc_frame_t* frame) {
  int r;
  uint8_t type;
  uint8_t tmp;
  mc_parser_t parser;

  parser.data = data;
  parser.offset = 0;
  parser.len = len;

  PARSE_READ(&parser, u8, &type);
  frame->type = (mc_frame_type_t) type;

  switch (frame->type) {
    case kMCKeepAliveType:
      PARSE_READ(&parser, u16, &frame->body.keepalive);
      return parser.offset;
    case kMCHandshakeType:
      return mc_parser__parse_handshake(&parser, frame);
    case kMCClientStatus:
      PARSE_READ(&parser, u8, &tmp);
      frame->body.client_status = (mc_client_status_t) tmp;
      return parser.offset;
    case kMCEncryptionResType:
      return mc_parser__parse_enc_resp(&parser, frame);
    default:
      /* Unknown frame, or frame should be sent by server */
      return -1;
  }
}


int mc_parser__read_u8(mc_parser_t* p, uint8_t* out) {
  if (p->len < 1)
    return 0;
  *out = p->data[0];
  p->data++;
  p->offset++;
  p->len--;

  return 1;
}


int mc_parser__read_u16(mc_parser_t* p, uint16_t* out) {
  if (p->len < 2)
    return 0;
  *out = ntohs(*(uint16_t*) p->data);
  p->data += 2;
  p->offset += 2;
  p->len -= 2;

  return 1;
}


int mc_parser__read_u32(mc_parser_t* p, uint32_t* out) {
  if (p->len < 4)
    return 0;
  *out = ntohl(*(uint32_t*) p->data);
  p->data += 4;
  p->offset += 4;
  p->len -= 4;

  return 1;
}


int mc_parser__read_string(mc_parser_t* p, mc_string_t* str) {
  int r;
  uint16_t len;

  mc_string_init(str);

  PARSE_READ(p, u16, &len);
  if (p->len < (size_t) len * 2)
    return 0;

  str->data = (uint16_t*) p->data;
  str->len = len * 2;

  p->data += str->len;
  p->offset += str->len;
  p->len -= str->len;

  return 1;
}


int mc_parser__read_raw(mc_parser_t* p, unsigned char** out, size_t size) {
  if (p->len < size)
    return 0;

  *out = (unsigned char*) p->data;
  p->data += size;
  p->offset += size;
  p->len -= size;

  return 1;
}


int mc_parser__parse_handshake(mc_parser_t* p, mc_frame_t* frame) {
  int r;

  if (p->len < 10)
    return 0;

  PARSE_READ(p, u8, &frame->body.handshake.version);
  PARSE_READ(p, string, &frame->body.handshake.username);
  PARSE_READ(p, string, &frame->body.handshake.host);
  PARSE_READ(p, u32, &frame->body.handshake.port);

  return p->offset;
}


int mc_parser__parse_enc_resp(mc_parser_t* p, mc_frame_t* frame) {
  int r;

  if (p->len < 4)
    return 0;

  PARSE_READ(p, u16, &frame->body.enc_resp.secret_len);
  PARSE_READ_RAW(p,
                 &frame->body.enc_resp.secret,
                 frame->body.enc_resp.secret_len);
  PARSE_READ(p, u16, &frame->body.enc_resp.token_len);
  PARSE_READ_RAW(p,
                 &frame->body.enc_resp.token,
                 frame->body.enc_resp.token_len);

  return p->offset;
}
