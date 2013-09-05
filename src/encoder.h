#ifndef SRC_ENCODER_H_
#define SRC_ENCODER_H_

#include "common.h"  /* mc_string_t */

#define MC_ENCODER_WRITE(encoder, type, value) \
    do { \
      int r; \
      r = mc_encoder_write_##type((encoder), (value)); \
      if (r != 0) \
        return r; \
    } while (0);

#define MC_ENCODER_WRITE_DATA(encoder, data, len) \
    do { \
      int r; \
      r = mc_encoder_write_data((encoder), (data), (len)); \
      if (r != 0) \
        return r; \
    } while (0); \

typedef struct mc_encoder_s mc_encoder_t;

struct mc_encoder_s {
  unsigned char* data;
  int len;
  int capacity;
};

int mc_encoder_init(mc_encoder_t* encoder, int capacity);
void mc_encoder_reset(mc_encoder_t* encoder);
void mc_encoder_destroy(mc_encoder_t* encoder);

unsigned char* mc_encoder_data(mc_encoder_t* encoder);
int mc_encoder_len(mc_encoder_t* encoder);
void mc_encoder_replace(mc_encoder_t* encoder, unsigned char* out, int len);

int mc_encoder_write_u8(mc_encoder_t* encoder, uint8_t value);
int mc_encoder_write_u16(mc_encoder_t* encoder, uint16_t value);
int mc_encoder_write_u32(mc_encoder_t* encoder, uint32_t value);
int mc_encoder_write_u64(mc_encoder_t* encoder, uint64_t value);
int mc_encoder_write_i8(mc_encoder_t* encoder, int8_t value);
int mc_encoder_write_i16(mc_encoder_t* encoder, int16_t value);
int mc_encoder_write_i32(mc_encoder_t* encoder, int32_t value);
int mc_encoder_write_i64(mc_encoder_t* encoder, int64_t value);
int mc_encoder_write_string(mc_encoder_t* encoder, mc_string_t* str);
int mc_encoder_write_data(mc_encoder_t* encoder, const void* data, int len);

#endif  /* SRC_ENCODER_H_ */
