#include <stdlib.h>  /* NULL */
#include <string.h>  /* strncmp */

#include "format/nbt.h"

mc_nbt_t** mc_nbt_find(mc_nbt_t* obj,
                       const char* prop,
                       int len,
                       mc_nbt_type_t type) {
  int i;
  mc_nbt_t* item;
  if (obj->type != kNBTCompound)
    return NULL;

  for (i = 0; i < obj->value.values.len;i++) {
    item = obj->value.values.list[i];
    if (item->name.len != len ||
        strncmp(item->name.value, prop, len) != 0) {
      continue;
    }
    if (item->type != type)
      return NULL;
    return &obj->value.values.list[i];
  }

  return NULL;
}


mc_nbt_t* mc_nbt_get(mc_nbt_t* obj,
                     const char* prop,
                     int len,
                     mc_nbt_type_t type) {
  mc_nbt_t** slot;

  slot = mc_nbt_find(obj, prop, len, type);
  if (slot == NULL)
    return NULL;

  return *slot;
}


int mc_nbt_set(mc_nbt_t* obj,
               const char* prop,
               int len,
               mc_nbt_type_t type,
               void* val) {
  mc_nbt_t* r;

  r = mc_nbt_get(obj, prop, len, type);
  if (r == NULL)
    return -1;

  switch (type) {
    case kNBTByte:
      r->value.i8 = *(int8_t*) val;
      break;
    case kNBTShort:
      r->value.i16 = *(int16_t*) val;
      break;
    case kNBTInt:
      r->value.i32 = *(int32_t*) val;
      break;
    case kNBTLong:
      r->value.i64 = *(int64_t*) val;
      break;
    case kNBTFloat:
      r->value.f32 = *(float*) val;
      break;
    case kNBTDouble:
      r->value.f64 = *(double*) val;
      break;
    default:
      /* Not supported */
      return - 1;
  }
  return 0;
}


int mc_nbt_read(mc_nbt_t* obj,
                const char* prop,
                int len,
                mc_nbt_type_t type,
                void* to) {
  mc_nbt_t* r;
  int byte_size;
  unsigned char* tmp;

  r = mc_nbt_get(obj, prop, len, type);
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
      byte_size = r->value.i8l.len * (type == kNBTIntArray ? 4 : 1);
      tmp = malloc(byte_size);
      if (tmp == NULL)
        return -1;
      memcpy(tmp, r->value.i8l.list, byte_size);
      *(void**) to = tmp;
      break;
    default:
      /* Not supported */
      return - 1;
  }

  return 0;
}


mc_nbt_t* mc_nbt_clone(const mc_nbt_t* val) {
  int i;
  int additional_size;
  mc_nbt_t* res;

  switch (val->type) {
    case kNBTByteArray:
    case kNBTString:
      additional_size = val->value.i8l.len;
      break;
    case kNBTIntArray:
      additional_size = val->value.i32l.len * 4;
      break;
    case kNBTList:
    case kNBTCompound:
      additional_size = (val->value.values.len - 1) *
                        sizeof(*val->value.values.list);
      break;
    default:
      additional_size = 0;
      break;
  }

  res = malloc(sizeof(*res) + additional_size + val->name.len);
  if (res == NULL)
    return NULL;

  switch (val->type) {
    case kNBTList:
    case kNBTCompound:
      /* Clone children */
      for (i = 0; i < val->value.values.len; i++) {
        res->value.values.list[i] = mc_nbt_clone(val->value.values.list[i]);
        if (res->value.values.list[i] == NULL)
          goto high_level_failed;
      }
      res->type = val->type;
      res->value.values.len = val->value.values.len;
      break;
    default:
      /* Copy all primitive data */
      memcpy(res, val, sizeof(*res));

      /* Copy byte array */
      if (val->type == kNBTByteArray ||
          val->type == kNBTString ||
          val->type == kNBTIntArray) {
        res->value.i8l.list = (int8_t*) res + sizeof(*res);
        memcpy(res->value.i8l.list, val->value.i8l.list, additional_size);
      }
      break;
  }

  /* Copy name */
  res->name.value = (char*) res + sizeof(*res) + additional_size;
  res->name.len = val->name.len;
  memcpy((char*) res->name.value, val->name.value, val->name.len);

  return res;

high_level_failed:
  while (--i >= 0)
    mc_nbt_destroy(res->value.values.list[i]);
  free(res);
  return NULL;
}
