#include <stdlib.h>  /* malloc, free */
#include <string.h>  /* memcpy */

#include "nbt.h"
#include "nbt-private.h"

static mc_nbt_t* mc_nbt__create(const char* name,
                                int name_len,
                                mc_nbt_type_t type,
                                int additional_payload);
static mc_nbt_t* mc_nbt__create_vl(mc_nbt_type_t type,
                                   const char* name,
                                   int name_len,
                                   int len);


mc_nbt_t* mc_nbt_create_i8(const char* name, int name_len, int8_t val) {
  mc_nbt_t* res;

  res = mc_nbt__create(name, name_len, kNBTByte, 0);
  if (res != NULL)
    res->value.i8 = val;
  return res;
}


mc_nbt_t* mc_nbt_create_i16(const char* name, int name_len, int16_t val) {
  mc_nbt_t* res;

  res = mc_nbt__create(name, name_len, kNBTShort, 0);
  if (res != NULL)
    res->value.i16 = val;
  return res;
}


mc_nbt_t* mc_nbt_create_i32(const char* name, int name_len, int32_t val) {
  mc_nbt_t* res;

  res = mc_nbt__create(name, name_len, kNBTInt, 0);
  if (res != NULL)
    res->value.i32 = val;
  return res;
}


mc_nbt_t* mc_nbt_create_i64(const char* name, int name_len, int64_t val) {
  mc_nbt_t* res;

  res = mc_nbt__create(name, name_len, kNBTLong, 0);
  if (res != NULL)
    res->value.i64 = val;
  return res;
}


mc_nbt_t* mc_nbt_create_f32(const char* name, int name_len, float val) {
  mc_nbt_t* res;

  res = mc_nbt__create(name, name_len, kNBTFloat, 0);
  if (res != NULL)
    res->value.f32 = val;
  return res;
}


mc_nbt_t* mc_nbt_create_f64(const char* name, int name_len, double val) {
  mc_nbt_t* res;

  res = mc_nbt__create(name, name_len, kNBTDouble, 0);
  if (res != NULL)
    res->value.f64 = val;
  return res;
}


mc_nbt_t* mc_nbt_create_str(const char* name,
                            int name_len,
                            const char* value,
                            int value_len) {
  mc_nbt_t* res;

  res = mc_nbt__create(name, name_len, kNBTString, value_len - 1);
  if (res != NULL) {
    res->value.str.len = value_len;
    memcpy(res->value.str.value, value, value_len);
  }
  return res;
}


mc_nbt_t* mc_nbt_create_i8l(const char* name, int name_len, int len) {
  mc_nbt_t* res;

  res = mc_nbt__create(name, name_len, kNBTByteArray, len - 1);
  if (res != NULL)
    res->value.i8_list.len = len;

  return res;
}


mc_nbt_t* mc_nbt_create_i32l(const char* name, int name_len, int len) {
  mc_nbt_t* res;

  res = mc_nbt__create(name, name_len, kNBTIntArray, 4 * (len - 1));
  if (res != NULL)
    res->value.i32_list.len = len;

  return res;
}


mc_nbt_t* mc_nbt_create_list(const char* name, int name_len, int len) {
  return mc_nbt__create_vl(kNBTList, name, name_len, len);
}


mc_nbt_t* mc_nbt_create_compound(const char* name, int name_len, int len) {
  return mc_nbt__create_vl(kNBTCompound, name, name_len, len);
}


void mc_nbt_destroy(mc_nbt_t* val) {
  int32_t i;
  if (val->type == kNBTList || val->type == kNBTCompound)
    for (i = 0; i < val->value.values.len; i++)
      mc_nbt_destroy(val->value.values.list[i]);
  free(val);
}


mc_nbt_t* mc_nbt__create(const char* name,
                         int name_len,
                         mc_nbt_type_t type,
                         int additional_payload) {
  mc_nbt_t* res;

  res = malloc(sizeof(*res) + additional_payload + name_len);
  if (res == NULL)
    return NULL;

  res->type = type;
  res->name.value = (char*) res + sizeof(*res) + additional_payload;
  res->name.len = name_len;
  memcpy((char*) res->name.value, name, name_len);

  return res;
}


mc_nbt_t* mc_nbt__create_vl(mc_nbt_type_t type,
                            const char* name,
                            int name_len,
                            int len) {
  mc_nbt_t* res;

  res = mc_nbt__create(name,
                       name_len,
                       type,
                       sizeof(*res->value.values.list) * (len - 1));
  if (res != NULL)
    res->value.values.len = len;

  return res;
}
