#include <arpa/inet.h>  /* htons, htonl */
#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* memcpy */

#include "utils/encoder.h"
#include "format/nbt.h"
#include "format/nbt-private.h"

static int mc_nbt__encode(mc_encoder_t* encoder,
                          int with_name,
                          mc_nbt_t* val);
static mc_nbt__tag_t mc_nbt__type_to_tag(mc_nbt_type_t type);
static int mc_nbt__encode_payload(mc_encoder_t* encoder,
                                  mc_nbt_t* val);
static int mc_nbt__encode_high_order(mc_encoder_t* encoder,
                                     mc_nbt_t* val);
static int mc_nbt__compress(mc_encoder_t* encoder, mc_nbt_comp_t comp);


int mc_nbt_encode(mc_nbt_t* val,
                  mc_nbt_comp_t comp,
                  unsigned char** out) {
  int r;
  mc_encoder_t encoder;

  r = mc_encoder_init(&encoder, 0);
  if (r != 0)
    return r;

  r = mc_nbt__encode(&encoder, 1, val);
  if (r != 0)
    goto fatal;

  if (comp != kNBTUncompressed) {
    r = mc_nbt__compress(&encoder, comp);
    if (r != 0)
      goto fatal;
  }

  *out = mc_encoder_data(&encoder);
  return mc_encoder_len(&encoder);

fatal:
  mc_encoder_destroy(&encoder);
  return -1;
}


int mc_nbt__encode(mc_encoder_t* encoder, int with_name, mc_nbt_t* val) {
  mc_nbt__tag_t tag;

  tag = mc_nbt__type_to_tag(val->type);

  if (with_name) {
    MC_ENCODER_WRITE(encoder, u8, tag);
    MC_ENCODER_WRITE(encoder, i16, val->name.len);
    MC_ENCODER_WRITE_DATA(encoder, val->name.value, val->name.len);
  } else {
    MC_ENCODER_WRITE(encoder, u8, tag);
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


int mc_nbt__encode_payload(mc_encoder_t* encoder, mc_nbt_t* val) {
  switch (val->type) {
    case kNBTByte:
      MC_ENCODER_WRITE(encoder, i8, val->value.i8);
      break;
    case kNBTShort:
      MC_ENCODER_WRITE(encoder, i16, val->value.i16);
      break;
    case kNBTFloat:
    case kNBTInt:
      MC_ENCODER_WRITE(encoder, i32, val->value.i32);
      break;
    case kNBTDouble:
    case kNBTLong:
      MC_ENCODER_WRITE(encoder, i64, val->value.i64);
      break;
    case kNBTByteArray:
      MC_ENCODER_WRITE(encoder, i32, val->value.i8l.len);
      MC_ENCODER_WRITE_DATA(encoder, val->value.i8l.list, val->value.i8l.len);
      break;
    case kNBTString:
      MC_ENCODER_WRITE(encoder, i16, val->value.str.len);
      MC_ENCODER_WRITE_DATA(encoder, val->value.str.value, val->value.str.len);
      break;
    case kNBTIntArray:
      MC_ENCODER_WRITE(encoder, i32, val->value.i32l.len);
      MC_ENCODER_WRITE_DATA(encoder,
                            val->value.i32l.list,
                            val->value.i32l.len * 4);
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


int mc_nbt__encode_high_order(mc_encoder_t* encoder, mc_nbt_t* val) {
  int r;
  int32_t i;
  int with_name;

  if (val->type == kNBTList) {
    with_name = 0;
    if (val->value.values.len == 0) {
      MC_ENCODER_WRITE(encoder, u8, kNBTTagByte);
    } else {
      MC_ENCODER_WRITE(encoder,
                       u8,
                       mc_nbt__type_to_tag(val->value.values.list[0]->type));
    }
    MC_ENCODER_WRITE(encoder, i32, val->value.values.len);
  } else {
    with_name = 1;
  }

  for (i = 0; i < val->value.values.len; i++) {
    r = mc_nbt__encode(encoder, with_name, val->value.values.list[i]);
    if (r != 0)
      return r;
  }

  /* Insert End tag */
  if (val->type == kNBTCompound)
    MC_ENCODER_WRITE(encoder, u8, kNBTTagEnd);

  return 0;
}


int mc_nbt__compress(mc_encoder_t* encoder, mc_nbt_comp_t comp) {
  unsigned char* out;
  int len;

  len = mc_nbt__zlib(mc_encoder_data(encoder),
                     mc_encoder_len(encoder),
                     comp,
                     0,
                     &out);
  if (len < 0)
    return len;

  mc_encoder_replace(encoder, out, len);

  return 0;
}
