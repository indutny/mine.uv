#include <arpa/inet.h>  /* htons, htonl */
#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* memcpy */

#include "nbt.h"
#include "nbt-private.h"

typedef struct mc_nbt_encoder_s mc_nbt_encoder_t;

struct mc_nbt_encoder_s {
  unsigned char* out;
  int offset;
  int len;
};

static int mc_nbt__grow(mc_nbt_encoder_t* encoder, int size);
static int mc_nbt__encode(mc_nbt_encoder_t* encoder,
                          int with_name,
                          mc_nbt_t* val);
static mc_nbt__tag_t mc_nbt__type_to_tag(mc_nbt_type_t type);
static int mc_nbt__encode_payload(mc_nbt_encoder_t* encoder,
                                  mc_nbt_t* val);
static int mc_nbt__encode_high_order(mc_nbt_encoder_t* encoder,
                                     mc_nbt_t* val);
static int mc_nbt__compress(mc_nbt_encoder_t* encoder,
                            mc_nbt_comp_t comp);

#define GROW(encoder, size, block) \
    do { \
      int r; \
      r = mc_nbt__grow((encoder), (size)); \
      if (r != 0) \
        return r; \
      block \
      (encoder)->offset += (size); \
    } while (0)

#define PTR(encoder, type, off) \
    ((type*) ((encoder)->out + (encoder)->offset + off))

static const int kEncodeIncrement = 1024;


int mc_nbt_encode(mc_nbt_t* val,
                  mc_nbt_comp_t comp,
                  unsigned char** out) {
  int r;
  mc_nbt_encoder_t encoder;

  encoder.len = kEncodeIncrement;
  encoder.offset = 0;
  encoder.out = malloc(encoder.len);
  if (encoder.out == NULL)
    return -1;

  r = mc_nbt__encode(&encoder, 1, val);
  if (r != 0)
    goto fatal;

  if (comp != kNBTUncompressed) {
    r = mc_nbt__compress(&encoder, comp);
    if (r != 0)
      goto fatal;
  }

  *out = encoder.out;
  return encoder.offset;

fatal:
  free(encoder.out);
  return -1;
}


int mc_nbt__grow(mc_nbt_encoder_t* encoder, int size) {
  unsigned char* out;
  int new_len;

  if (encoder->offset + size <= encoder->len)
    return 0;

  new_len = encoder->len + kEncodeIncrement;
  out = malloc(new_len);
  if (out == NULL)
    return -1;

  memcpy(out, encoder->out, encoder->len);
  free(encoder->out);
  encoder->out = out;
  encoder->len = new_len;

  return 0;
}


int mc_nbt__encode(mc_nbt_encoder_t* encoder,
                   int with_name,
                   mc_nbt_t* val) {
  mc_nbt__tag_t tag;

  tag = mc_nbt__type_to_tag(val->type);

  if (with_name) {
    GROW(encoder, 3 + val->name.len, {
      *PTR(encoder, uint8_t, 0) = tag;
      *PTR(encoder, int16_t, 1) = htons(val->name.len);
      memcpy(PTR(encoder, char, 3), val->name.value, val->name.len);
    });
  } else {
    GROW(encoder, 1, {
      *PTR(encoder, uint8_t, 0) = tag;
    });
  }

  return mc_nbt__encode_payload(encoder, val);
}


mc_nbt__tag_t mc_nbt__type_to_tag(mc_nbt_type_t type) {
  switch (type) {
    case kNBTByte: return kNBTTagByte; break;
    case kNBTShort: return kNBTTagShort; break;
    case kNBTInt: return kNBTTagInt; break;
    case kNBTLong: return kNBTTagLong; break;
    case kNBTFloat: return kNBTTagFloat; break;
    case kNBTDouble: return kNBTTagDouble; break;
    case kNBTByteArray: return kNBTTagByteArray; break;
    case kNBTIntArray: return kNBTTagIntArray; break;
    case kNBTString: return kNBTTagString; break;
    case kNBTList: return kNBTTagList; break;
    case kNBTCompound: return kNBTTagCompound; break;
    default: abort(); break;
  }
}


int mc_nbt__encode_payload(mc_nbt_encoder_t* encoder, mc_nbt_t* val) {
  switch (val->type) {
    case kNBTByte:
      GROW(encoder, 1, {
        *PTR(encoder, int8_t, 0) = val->value.i8;
      });
      break;
    case kNBTShort:
      GROW(encoder, 2, {
        *PTR(encoder, int16_t, 0) = htons(val->value.i16);
      });
      break;
    case kNBTInt:
      GROW(encoder, 4, {
        *PTR(encoder, int32_t, 0) = htonl(val->value.i32);
      });
      break;
    case kNBTLong:
      GROW(encoder, 8, {
        *PTR(encoder, int32_t, 0) = htonl(val->value.i64 >> 32);
        *PTR(encoder, int32_t, 4) = htonl(val->value.i64 & 0xffffffff);
      });
      break;
    case kNBTFloat:
      GROW(encoder, 4, {
        *PTR(encoder, float, 0) = val->value.f32;
      });
      break;
    case kNBTDouble:
      GROW(encoder, 8, {
        *PTR(encoder, double, 0) = val->value.f64;
      });
      break;
    case kNBTByteArray:
      GROW(encoder, 4 + val->value.i8_list.len, {
        *PTR(encoder, int32_t, 0) = htonl(val->value.i8_list.len);
        memcpy(PTR(encoder, char, 4),
               val->value.i8_list.list,
               val->value.i8_list.len);
      });
      break;
    case kNBTString:
      GROW(encoder, 2 + val->value.str.len, {
        *PTR(encoder, int16_t, 0) = htons(val->value.str.len);
        memcpy(PTR(encoder, char, 2),
               val->value.str.value,
               val->value.str.len);
      });
      break;
    case kNBTIntArray:
      GROW(encoder, 4 + 4 * val->value.i32_list.len, {
        *PTR(encoder, int32_t, 0) = htonl(val->value.i32_list.len);
        memcpy(PTR(encoder, char, 4),
               val->value.i32_list.list,
               val->value.i32_list.len * 4);
      });
      break;
    /* High-order structures */
    case kNBTList:
    case kNBTCompound:
      return mc_nbt__encode_high_order(encoder, val);
    default:
      return -1;
  }

  return 0;
}


int mc_nbt__encode_high_order(mc_nbt_encoder_t* encoder, mc_nbt_t* val) {
  int r;
  int32_t i;
  int with_name;

  if (val->type == kNBTList) {
    with_name = 0;
    GROW(encoder, 5, {
      if (val->value.values.len == 0) {
        *PTR(encoder, uint8_t, 0) = kNBTTagByte;
        *PTR(encoder, int32_t, 1) = 0;
      } else {
        *PTR(encoder, uint8_t, 0) =
            mc_nbt__type_to_tag(val->value.values.list[0]->type);
        *PTR(encoder, int32_t, 1) = 0;
      }
    });
  } else {
    with_name = 1;
  }

  for (i = 0; i < val->value.values.len; i++) {
    r = mc_nbt__encode(encoder, with_name, val->value.values.list[i]);
    if (r != 0)
      return r;
  }

  /* Insert End tag */
  if (val->type == kNBTCompound) {
    GROW(encoder, 1, {
      *PTR(encoder, uint8_t, 0) = kNBTTagEnd;
    });
  }

  return 0;
}


int mc_nbt__compress(mc_nbt_encoder_t* encoder, mc_nbt_comp_t comp) {
  unsigned char* out;
  int len;

  len = mc_nbt__zlib(encoder->out, encoder->offset, comp, 0, &out);
  if (len < 0)
    return len;

  free(encoder->out);
  encoder->out = out;
  encoder->offset = len;

  return 0;
}
