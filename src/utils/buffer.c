#include <arpa/inet.h>  /* htonl, htons */
#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* memcpy */

#include "utils/buffer.h"

#define GROW(buffer, size) \
    do { \
      int r; \
      r = mc_buffer__check_grow((buffer), (size)); \
      if (r != 0) \
        return r; \
    } while (0)

#define PTR(buffer, type) \
    ((type*) ((buffer)->data + (buffer)->len))

static int mc_buffer__check_grow(mc_buffer_t* buffer, int size);

static const int kDefaultCapacity = 256;
static const int kCapacityIncrement = 1024;


int mc_buffer_init(mc_buffer_t* buffer, int capacity) {
  buffer->len = 0;
  buffer->capacity = capacity == 0 ? kDefaultCapacity : capacity;
  buffer->data = malloc(buffer->len);
  if (buffer->data == NULL)
    return -1;
  return 0;
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


int mc_buffer_len(mc_buffer_t* buffer) {
  return buffer->len;
}


void mc_buffer_replace(mc_buffer_t* buffer, unsigned char* out, int len) {
  free(buffer->data);
  buffer->data = out;
  buffer->len = len;
  buffer->capacity = len;
}


int mc_buffer_write_u8(mc_buffer_t* buffer, uint8_t value) {
  GROW(buffer, 1);
  *PTR(buffer, uint8_t) = value;
  buffer->len += 1;
  return 0;
}


int mc_buffer_write_u16(mc_buffer_t* buffer, uint16_t value) {
  GROW(buffer, 2);
  *PTR(buffer, uint16_t) = htons(value);
  buffer->len += 2;
  return 0;
}


int mc_buffer_write_u32(mc_buffer_t* buffer, uint32_t value) {
  GROW(buffer, 4);
  *PTR(buffer, uint32_t) = htonl(value);
  buffer->len += 4;
  return 0;
}


int mc_buffer_write_u64(mc_buffer_t* buffer, uint64_t value) {
  GROW(buffer, 8);
  *PTR(buffer, uint32_t) = htonl((value >> 32) & 0xffffffff);
  buffer->len += 4;
  *PTR(buffer, uint32_t) = htonl(value & 0xffffffff);
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
  memcpy(PTR(buffer, void), data, len);
  buffer->len += len;
  return 0;
}


int mc_buffer__check_grow(mc_buffer_t* buffer, int size) {
  int new_capacity;
  unsigned char* new_data;

  /* No need to grow */
  if (buffer->len + size <= buffer->capacity)
    return 0;

  /* Grow */
  new_capacity = buffer->capacity + kCapacityIncrement;
  new_data = malloc(new_capacity);
  if (new_data == NULL)
    return -1;

  /* Copy old data */
  memcpy(new_data, buffer->data, buffer->len);
  free(buffer->data);
  buffer->data = new_data;
  buffer->capacity = new_capacity;

  return 0;
}
