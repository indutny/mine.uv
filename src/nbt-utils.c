#include <stdlib.h>  /* NULL */
#include <string.h>  /* strncmp */

#include "nbt.h"

mc_nbt_t* mc_nbt_get(mc_nbt_t* val,
                     const char* prop,
                     int len,
                     mc_nbt_type_t type) {
  int i;
  mc_nbt_t* item;
  if (val->type != kNBTCompound)
    return NULL;

  for (i = 0; i < val->value.values.len;i++) {
    item = val->value.values.list[i];
    if (item->name.len != len ||
        strncmp(item->name.value, prop, len) != 0) {
      continue;
    }
    if (item->type != type)
      return NULL;
    return item;
  }

  return NULL;
}


int mc_nbt_read(mc_nbt_t* val,
                const char* prop,
                int len,
                mc_nbt_type_t type,
                void* to) {
  mc_nbt_t* r;
  int byte_size;
  unsigned char* tmp;

  r = mc_nbt_get(val, prop, len, type);
  if (r == NULL)
    return -1;
  switch (type) {
    case kNBTByte:
      *(int8_t*) to = r->value.i8;
      break;
    case kNBTShort:
      *(int16_t*) to = r->value.i16;
      break;
    case kNBTInt:
      *(int32_t*) to = r->value.i32;
      break;
    case kNBTLong:
      *(int64_t*) to = r->value.i64;
      break;
    case kNBTFloat:
      *(float*) to = r->value.f32;
      break;
    case kNBTDouble:
      *(double*) to = r->value.f64;
      break;
    case kNBTByteArray:
    case kNBTIntArray:
    case kNBTString:
      byte_size = r->value.i8_list.len * (type == kNBTIntArray ? 4 : 1);
      tmp = malloc(byte_size);
      if (tmp == NULL)
        return -1;
      memcpy(tmp, r->value.i8_list.list, byte_size);
      *(void**) to = tmp;
      break;
    default:
      /* Not supported */
      return - 1;
  }

  return 0;
}
