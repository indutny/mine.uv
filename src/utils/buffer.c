#include <arpa/inet.h>  /* htonl, htons, ntohl, ntohs */
#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* memcpy */

#include "utils/buffer.h"
#include "utils/common.h"  /* mc_slot_t */
#include "utils/string.h"  /* mc_string_t */

#define GROW(buffer, size) \
    do { \
      int r; \
      r = mc_buffer__check_grow((buffer), (size)); \
      if (r != 0) \
        return r; \
    } while (0)

#define WRITE_PTR(buffer, type) \
    ((type*) ((buffer)->data + (buffer)->len))

#define READ_PTR(buffer, type) \
    ((type*) ((buffer)->data + (buffer)->offset))

static int mc_buffer__check_grow(mc_buffer_t* buffer, int size);

static const int kDefaultCapacity = 256;
static const int kCapacityIncrement = 16384;


int mc_buffer_init(mc_buffer_t* buffer, int capacity) {
  buffer->len = 0;
  buffer->offset = 0;
  buffer->capacity = capacity == 0 ? kDefaultCapacity : capacity;
  buffer->data = malloc(buffer->capacity);
  if (buffer->data == NULL)
    return kMCBufferNoMem;
  return 0;
}


void mc_buffer_from_data(mc_buffer_t* buffer, unsigned char* data, int len) {
  buffer->len = len;
  buffer->offset = 0;
  buffer->capacity = len;
  buffer->data = data;
}


void mc_buffer_reset(mc_buffer_t* buffer) {
  buffer->len = 0;
}


void mc_buffer_destroy(mc_buffer_t* buffer) {
  free(buffer->data);
  buffer->data = NULL;
  buffer->capacity = 0;
  buffer->len = 0;
}


unsigned char* mc_buffer_data(mc_buffer_t* buffer) {
  return buffer->data;
}


int mc_buffer_offset(mc_buffer_t* buffer) {
  return buffer->offset;
}


int mc_buffer_len(mc_buffer_t* buffer) {
  return buffer->len;
}


void mc_buffer_replace(mc_buffer_t* buffer, unsigned char* out, int len) {
  free(buffer->data);
  buffer->data = out;
  buffer->len = len;
  buffer->capacity = len;
}


int mc_buffer_reserve(mc_buffer_t* buffer, int size) {
  GROW(buffer, size);
  memset(WRITE_PTR(buffer, void), 0, size);
  buffer->len += size;
  return buffer->len - size;
}


unsigned char* mc_buffer_reserve_ptr(mc_buffer_t* buffer, int reserve_off) {
  return buffer->data + reserve_off;
}


int mc_buffer_write_u8(mc_buffer_t* buffer, uint8_t value) {
  GROW(buffer, 1);
  *WRITE_PTR(buffer, uint8_t) = value;
  buffer->len += 1;
  return 0;
}


int mc_buffer_write_u16(mc_buffer_t* buffer, uint16_t value) {
  GROW(buffer, 2);
  *WRITE_PTR(buffer, uint16_t) = htons(value);
  buffer->len += 2;
  return 0;
}


int mc_buffer_write_u32(mc_buffer_t* buffer, uint32_t value) {
  GROW(buffer, 4);
  *WRITE_PTR(buffer, uint32_t) = htonl(value);
  buffer->len += 4;
  return 0;
}


int mc_buffer_write_u64(mc_buffer_t* buffer, uint64_t value) {
  GROW(buffer, 8);
  *WRITE_PTR(buffer, uint32_t) = htonl((value >> 32) & 0xffffffff);
  buffer->len += 4;
  *WRITE_PTR(buffer, uint32_t) = htonl(value & 0xffffffff);
  buffer->len += 4;
  return 0;
}


int mc_buffer_write_i8(mc_buffer_t* buffer, int8_t value) {
  return mc_buffer_write_u8(buffer, (uint8_t) value);
}


int mc_buffer_write_i16(mc_buffer_t* buffer, int16_t value) {
  return mc_buffer_write_u16(buffer, (uint16_t) value);
}


int mc_buffer_write_i32(mc_buffer_t* buffer, int32_t value) {
  return mc_buffer_write_u32(buffer, (uint32_t) value);
}


int mc_buffer_write_i64(mc_buffer_t* buffer, int64_t value) {
  return mc_buffer_write_u64(buffer, (uint64_t) value);
}


int mc_buffer_write_string(mc_buffer_t* buffer, mc_string_t* str) {
  uint16_t len;

  len = str->len;
  MC_BUFFER_WRITE(buffer, u16, len);
  MC_BUFFER_WRITE_DATA(buffer, str->data, len * sizeof(*str->data));

  return 0;
}


int mc_buffer_write_data(mc_buffer_t* buffer, const void* data, int len) {
  GROW(buffer, len);
  memcpy(WRITE_PTR(buffer, void), data, len);
  buffer->len += len;
  return 0;
}

int mc_buffer_read_u8(mc_buffer_t* buffer, uint8_t* value) {
  if (buffer->offset + 1 > buffer->len)
    return kMCBufferOOB;

  *value = *READ_PTR(buffer, uint8_t);
  buffer->offset += 1;

  return 0;
}


int mc_buffer_read_u16(mc_buffer_t* buffer, uint16_t* value) {
  if (buffer->offset + 2 > buffer->len)
    return kMCBufferOOB;

  *value = ntohs(*READ_PTR(buffer, uint16_t));
  buffer->offset += 2;

  return 0;
}


int mc_buffer_read_u32(mc_buffer_t* buffer, uint32_t* value) {
  if (buffer->offset + 4 > buffer->len)
    return kMCBufferOOB;

  *value = ntohl(*READ_PTR(buffer, uint32_t));
  buffer->offset += 4;

  return 0;
}


int mc_buffer_read_u64(mc_buffer_t* buffer, uint64_t* value) {
  uint32_t hi;
  uint32_t lo;

  MC_BUFFER_READ(buffer, u32, &hi);
  MC_BUFFER_READ(buffer, u32, &lo);

  *value = ((uint64_t) hi << 32) | lo;

  return 0;
}


int mc_buffer_read_i8(mc_buffer_t* buffer, int8_t* value) {
  return mc_buffer_read_u8(buffer, (uint8_t*) value);
}


int mc_buffer_read_i16(mc_buffer_t* buffer, int16_t* value) {
  return mc_buffer_read_u16(buffer, (uint16_t*) value);
}


int mc_buffer_read_i32(mc_buffer_t* buffer, int32_t* value) {
  return mc_buffer_read_u32(buffer, (uint32_t*) value);
}


int mc_buffer_read_i64(mc_buffer_t* buffer, int64_t* value) {
  return mc_buffer_read_u64(buffer, (uint64_t*) value);
}


int mc_buffer_read_float(mc_buffer_t* buffer, float* value) {
  if (buffer->offset + 4 > buffer->len)
    return kMCBufferOOB;

  *value = *READ_PTR(buffer, float);
  buffer->offset += 4;

  return 0;
}


int mc_buffer_read_double(mc_buffer_t* buffer, double* value) {
  if (buffer->offset + 8 > buffer->len)
    return kMCBufferOOB;

  *value = *READ_PTR(buffer, double);
  buffer->offset += 8;

  return 0;
}


int mc_buffer_read_data(mc_buffer_t* buffer, unsigned char** data, int len) {
  if (buffer->offset + len > buffer->len)
    return kMCBufferOOB;

  *data = READ_PTR(buffer, unsigned char);
  buffer->offset += len;

  return 0;
}


int mc_buffer_read_string(mc_buffer_t* buffer, mc_string_t* str) {
  uint16_t len;
  int raw_len;

  mc_string_init(str);

  MC_BUFFER_READ(buffer, u16, &len);
  raw_len = len * sizeof(*str->data);
  if (raw_len + buffer->offset > buffer->len)
    return kMCBufferOOB;

  str->data = READ_PTR(buffer, uint16_t);
  str->len = len;

  buffer->offset += raw_len;

  return 0;
}


int mc_buffer_read_slot(mc_buffer_t* buffer, mc_slot_t* slot) {
  mc_slot_init(slot);

  MC_BUFFER_READ(buffer, u16, &slot->id);
  if (slot->id == 0xffff)
    return 0;

  MC_BUFFER_READ(buffer, u16, &slot->count);
  MC_BUFFER_READ(buffer, u16, &slot->damage);
  MC_BUFFER_READ(buffer, u16, &slot->nbt_len);
  if (slot->nbt_len == 0xfff)
    return 0;

  MC_BUFFER_READ_DATA(buffer, &slot->nbt, slot->nbt_len);

  return 0;
}


int mc_buffer__check_grow(mc_buffer_t* buffer, int size) {
  int new_capacity;
  unsigned char* new_data;

  /* No need to grow */
  if (buffer->len + size <= buffer->capacity)
    return 0;

  /* Grow */
  new_capacity = buffer->capacity + size;
  if (new_capacity % kCapacityIncrement != 0)
    new_capacity += kCapacityIncrement - (new_capacity % kCapacityIncrement);
  new_data = malloc(new_capacity);
  if (new_data == NULL)
    return kMCBufferNoMem;

  /* Copy old data */
  memcpy(new_data, buffer->data, buffer->len);
  free(buffer->data);
  buffer->data = new_data;
  buffer->capacity = new_capacity;

  return 0;
}
