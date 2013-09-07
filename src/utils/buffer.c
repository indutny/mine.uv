#include <arpa/inet.h>  /* htonl, htons, ntohl, ntohs */
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

#define WRITE_PTR(buffer, type) \
    ((type*) ((buffer)->data + (buffer)->len))

#define READ_PTR(buffer, type) \
    ((type*) ((buffer)->data + (buffer)->offset))

static int mc_buffer__check_grow(mc_buffer_t* buffer, int size);

static const int kDefaultCapacity = 256;
static const int kCapacityIncrement = 1024;


int mc_buffer_init(mc_buffer_t* buffer, int capacity) {
  buffer->len = 0;
  buffer->offset = 0;
  buffer->capacity = capacity == 0 ? kDefaultCapacity : capacity;
  buffer->data = malloc(buffer->len);
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


int mc_buffer_read_data(mc_buffer_t* buffer, void* data, int len) {
  if (buffer->offset + len > buffer->len)
    return kMCBufferOOB;

  memcpy(data, READ_PTR(buffer, void), len);
  buffer->offset += len;

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
    return kMCBufferNoMem;

  /* Copy old data */
  memcpy(new_data, buffer->data, buffer->len);
  free(buffer->data);
  buffer->data = new_data;
  buffer->capacity = new_capacity;

  return 0;
}
