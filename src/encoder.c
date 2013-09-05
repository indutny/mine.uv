#include <arpa/inet.h>  /* htonl, htons */
#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* memcpy */

#include "encoder.h"

#define GROW(encoder, size) \
    do { \
      int r; \
      r = mc_encoder__check_grow((encoder), (size)); \
      if (r != 0) \
        return r; \
    } while (0)

#define PTR(encoder, type) \
    ((type*) ((encoder)->data + (encoder)->len))

static int mc_encoder__check_grow(mc_encoder_t* encoder, int size);

static const int kDefaultCapacity = 256;
static const int kCapacityIncrement = 1024;


int mc_encoder_init(mc_encoder_t* encoder, int capacity) {
  encoder->len = 0;
  encoder->capacity = capacity == 0 ? kDefaultCapacity : capacity;
  encoder->data = malloc(encoder->len);
  if (encoder->data == NULL)
    return -1;
  return 0;
}


void mc_encoder_reset(mc_encoder_t* encoder) {
  encoder->len = 0;
}


void mc_encoder_destroy(mc_encoder_t* encoder) {
  free(encoder->data);
  encoder->data = NULL;
  encoder->capacity = 0;
  encoder->len = 0;
}


unsigned char* mc_encoder_data(mc_encoder_t* encoder) {
  return encoder->data;
}


int mc_encoder_len(mc_encoder_t* encoder) {
  return encoder->len;
}


int mc_encoder_write_u8(mc_encoder_t* encoder, uint8_t value) {
  GROW(encoder, 1);
  *PTR(encoder, uint8_t) = value;
  encoder->len += 1;
  return 0;
}


int mc_encoder_write_u16(mc_encoder_t* encoder, uint16_t value) {
  GROW(encoder, 2);
  *PTR(encoder, uint16_t) = htons(value);
  encoder->len += 2;
  return 0;
}


int mc_encoder_write_u32(mc_encoder_t* encoder, uint32_t value) {
  GROW(encoder, 4);
  *PTR(encoder, uint32_t) = htonl(value);
  encoder->len += 4;
  return 0;
}


int mc_encoder_write_u64(mc_encoder_t* encoder, uint64_t value) {
  GROW(encoder, 8);
  *PTR(encoder, uint32_t) = htonl((value >> 32) & 0xffffffff);
  encoder->len += 4;
  *PTR(encoder, uint32_t) = htonl(value & 0xffffffff);
  encoder->len += 4;
  return 0;
}


int mc_encoder_write_i8(mc_encoder_t* encoder, int8_t value) {
  return mc_encoder_write_u8(encoder, (uint8_t) value);
}


int mc_encoder_write_i16(mc_encoder_t* encoder, int16_t value) {
  return mc_encoder_write_u16(encoder, (uint16_t) value);
}


int mc_encoder_write_i32(mc_encoder_t* encoder, int32_t value) {
  return mc_encoder_write_u32(encoder, (uint32_t) value);
}


int mc_encoder_write_i64(mc_encoder_t* encoder, int64_t value) {
  return mc_encoder_write_u64(encoder, (uint64_t) value);
}


int mc_encoder_write_string(mc_encoder_t* encoder, mc_string_t* str) {
  uint16_t len;

  len = str->len;
  MC_ENCODER_WRITE(encoder, u16, len);
  MC_ENCODER_WRITE_DATA(encoder,
                        (const unsigned char*) str->data,
                        len * sizeof(*str->data));

  return 0;
}


int mc_encoder_write_data(mc_encoder_t* encoder,
                          const unsigned char* data,
                          int len) {
  GROW(encoder, len);
  memcpy(PTR(encoder, char), data, len);
  encoder->len += len;
  return 0;
}


int mc_encoder__check_grow(mc_encoder_t* encoder, int size) {
  int new_capacity;
  unsigned char* new_data;

  /* No need to grow */
  if (encoder->len + size <= encoder->capacity)
    return 0;

  /* Grow */
  new_capacity = encoder->capacity + kCapacityIncrement;
  new_data = malloc(new_capacity);
  if (new_data == NULL)
    return -1;

  /* Copy old data */
  memcpy(new_data, encoder->data, encoder->len);
  free(encoder->data);
  encoder->data = new_data;
  encoder->capacity = new_capacity;

  return 0;
}
