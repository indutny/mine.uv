#ifndef SRC_UTILS_BUFFER_H_
#define SRC_UTILS_BUFFER_H_

#include "common.h"  /* mc_string_t */

#define MC_BUFFER_WRITE(encoder, type, value) \
    do { \
      int r; \
      r = mc_buffer_write_##type((encoder), (value)); \
      if (r != 0) \
        return r; \
    } while (0);

#define MC_BUFFER_WRITE_DATA(encoder, data, len) \
    do { \
      int r; \
      r = mc_buffer_write_data((encoder), (data), (len)); \
      if (r != 0) \
        return r; \
    } while (0); \

typedef struct mc_buffer_s mc_buffer_t;

struct mc_buffer_s {
  unsigned char* data;
  int len;
  int capacity;
};

int mc_buffer_init(mc_buffer_t* encoder, int capacity);
void mc_buffer_reset(mc_buffer_t* encoder);
void mc_buffer_destroy(mc_buffer_t* encoder);

unsigned char* mc_buffer_data(mc_buffer_t* encoder);
int mc_buffer_len(mc_buffer_t* encoder);
void mc_buffer_replace(mc_buffer_t* encoder, unsigned char* out, int len);

int mc_buffer_write_u8(mc_buffer_t* encoder, uint8_t value);
int mc_buffer_write_u16(mc_buffer_t* encoder, uint16_t value);
int mc_buffer_write_u32(mc_buffer_t* encoder, uint32_t value);
int mc_buffer_write_u64(mc_buffer_t* encoder, uint64_t value);
int mc_buffer_write_i8(mc_buffer_t* encoder, int8_t value);
int mc_buffer_write_i16(mc_buffer_t* encoder, int16_t value);
int mc_buffer_write_i32(mc_buffer_t* encoder, int32_t value);
int mc_buffer_write_i64(mc_buffer_t* encoder, int64_t value);
int mc_buffer_write_string(mc_buffer_t* encoder, mc_string_t* str);
int mc_buffer_write_data(mc_buffer_t* encoder, const void* data, int len);

#endif  /* SRC_UTILS_BUFFER_H_ */
