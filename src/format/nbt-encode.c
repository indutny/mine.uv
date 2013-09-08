#include <arpa/inet.h>  /* htons, htonl */
#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* memcpy */

#include "utils/buffer.h"
#include "format/nbt.h"
#include "format/nbt-private.h"

static int mc_nbt__encode(mc_buffer_t* buffer,
                          int with_name,
                          mc_nbt_t* val);
static mc_nbt__tag_t mc_nbt__type_to_tag(mc_nbt_type_t type);
static int mc_nbt__encode_payload(mc_buffer_t* buffer,
                                  mc_nbt_t* val);
static int mc_nbt__encode_high_order(mc_buffer_t* buffer,
                                     mc_nbt_t* val);
static int mc_nbt__compress(mc_buffer_t* buffer, mc_nbt_comp_t comp);


int mc_nbt_encode(mc_nbt_t* val,
                  mc_nbt_comp_t comp,
                  unsigned char** out) {
  int r;
  mc_buffer_t buffer;

  r = mc_buffer_init(&buffer, 0);
  if (r != 0)
    return r;

  r = mc_nbt__encode(&buffer, 1, val);
  if (r != 0)
    goto fatal;

  if (comp != kNBTUncompressed) {
    r = mc_nbt__compress(&buffer, comp);
    if (r != 0)
      goto fatal;
  }

  *out = mc_buffer_data(&buffer);
  return mc_buffer_len(&buffer);

fatal:
  mc_buffer_destroy(&buffer);
  return -1;
}


int mc_nbt__encode(mc_buffer_t* buffer, int with_name, mc_nbt_t* val) {
  mc_nbt__tag_t tag;

  tag = mc_nbt__type_to_tag(val->type);

  if (with_name) {
    MC_BUFFER_WRITE(buffer, u8, tag);
    MC_BUFFER_WRITE(buffer, i16, val->name.len);
    MC_BUFFER_WRITE_DATA(buffer, val->name.value, val->name.len);
  }

  return mc_nbt__encode_payload(buffer, val);
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


int mc_nbt__encode_payload(mc_buffer_t* buffer, mc_nbt_t* val) {
  switch (val->type) {
    case kNBTByte:
      MC_BUFFER_WRITE(buffer, i8, val->value.i8);
      break;
    case kNBTShort:
      MC_BUFFER_WRITE(buffer, i16, val->value.i16);
      break;
    case kNBTFloat:
    case kNBTInt:
      MC_BUFFER_WRITE(buffer, i32, val->value.i32);
      break;
    case kNBTDouble:
    case kNBTLong:
      MC_BUFFER_WRITE(buffer, i64, val->value.i64);
      break;
    case kNBTByteArray:
      MC_BUFFER_WRITE(buffer, i32, val->value.i8l.len);
      MC_BUFFER_WRITE_DATA(buffer, val->value.i8l.list, val->value.i8l.len);
      break;
    case kNBTString:
      MC_BUFFER_WRITE(buffer, i16, val->value.str.len);
      MC_BUFFER_WRITE_DATA(buffer, val->value.str.value, val->value.str.len);
      break;
    case kNBTIntArray:
      MC_BUFFER_WRITE(buffer, i32, val->value.i32l.len);
      MC_BUFFER_WRITE_DATA(buffer,
                            val->value.i32l.list,
                            val->value.i32l.len * 4);
      break;
    /* High-order structures */
    case kNBTList:
    case kNBTCompound:
      return mc_nbt__encode_high_order(buffer, val);
    default:
      return -1;
  }

  return 0;
}


int mc_nbt__encode_high_order(mc_buffer_t* buffer, mc_nbt_t* val) {
  int r;
  int32_t i;
  int with_name;

  if (val->type == kNBTList) {
    with_name = 0;
    if (val->value.values.len == 0) {
      MC_BUFFER_WRITE(buffer, u8, kNBTTagByte);
    } else {
      MC_BUFFER_WRITE(buffer,
                       u8,
                       mc_nbt__type_to_tag(val->value.values.list[0]->type));
    }
    MC_BUFFER_WRITE(buffer, i32, val->value.values.len);
  } else {
    with_name = 1;
  }

  for (i = 0; i < val->value.values.len; i++) {
    r = mc_nbt__encode(buffer, with_name, val->value.values.list[i]);
    if (r != 0)
      return r;
  }

  /* Insert End tag */
  if (val->type == kNBTCompound)
    MC_BUFFER_WRITE(buffer, u8, kNBTTagEnd);

  return 0;
}


int mc_nbt__compress(mc_buffer_t* buffer, mc_nbt_comp_t comp) {
  unsigned char* out;
  int len;

  len = mc_nbt__zlib(mc_buffer_data(buffer),
                     mc_buffer_len(buffer),
                     comp,
                     0,
                     &out);
  if (len < 0)
    return len;

  mc_buffer_replace(buffer, out, len);

  return 0;
}
