#ifndef SRC_UTILS_BUFFER_H_
#define SRC_UTILS_BUFFER_H_

#include "utils/common.h"  /* mc_slot_t */
#include "utils/string.h"  /* mc_string_t */

#define MC_BUFFER_WRAP(expr) \
    do { \
      int r; \
      r = (expr); \
      if (r != 0) \
        return r; \
    } while (0)

#define MC_BUFFER_WRITE(buffer, type, value) \
    MC_BUFFER_WRAP(mc_buffer_write_##type((buffer), (value)))

#define MC_BUFFER_WRITE_DATA(buffer, data, len) \
    MC_BUFFER_WRAP(mc_buffer_write_data((buffer), (data), (len)))

#define MC_BUFFER_READ(buffer, type, value) \
    MC_BUFFER_WRAP(mc_buffer_read_##type((buffer), (value)))

#define MC_BUFFER_READ_DATA(buffer, data, len) \
    MC_BUFFER_WRAP(mc_buffer_read_data((buffer), (data), (len)))

typedef struct mc_buffer_s mc_buffer_t;
typedef enum mc_buffer_err_e mc_buffer_err_t;

struct mc_buffer_s {
  unsigned char* data;
  int offset;
  int len;
  int capacity;
};

enum mc_buffer_err_e {
  kMCBufferUnknown = -1,
  kMCBufferNoMem = -2,
  kMCBufferOOB = -3
};

int mc_buffer_init(mc_buffer_t* buffer, int capacity);
void mc_buffer_from_data(mc_buffer_t* buffer, unsigned char* data, int len);
void mc_buffer_reset(mc_buffer_t* buffer);
void mc_buffer_destroy(mc_buffer_t* buffer);

unsigned char* mc_buffer_data(mc_buffer_t* buffer);
int mc_buffer_offset(mc_buffer_t* buffer);
int mc_buffer_len(mc_buffer_t* buffer);
void mc_buffer_replace(mc_buffer_t* buffer, unsigned char* out, int len);
int mc_buffer_reserve(mc_buffer_t* buffer, int size);
unsigned char* mc_buffer_reserve_ptr(mc_buffer_t* buffer, int reserve_off);

/* Write interface */

int mc_buffer_write_u8(mc_buffer_t* buffer, uint8_t value);
int mc_buffer_write_u16(mc_buffer_t* buffer, uint16_t value);
int mc_buffer_write_u32(mc_buffer_t* buffer, uint32_t value);
int mc_buffer_write_u64(mc_buffer_t* buffer, uint64_t value);
int mc_buffer_write_i8(mc_buffer_t* buffer, int8_t value);
int mc_buffer_write_i16(mc_buffer_t* buffer, int16_t value);
int mc_buffer_write_i32(mc_buffer_t* buffer, int32_t value);
int mc_buffer_write_i64(mc_buffer_t* buffer, int64_t value);
int mc_buffer_write_string(mc_buffer_t* buffer, mc_string_t* str);
int mc_buffer_write_data(mc_buffer_t* buffer, const void* data, int len);

/* Read interface */

int mc_buffer_read_u8(mc_buffer_t* buffer, uint8_t* value);
int mc_buffer_read_u16(mc_buffer_t* buffer, uint16_t* value);
int mc_buffer_read_u32(mc_buffer_t* buffer, uint32_t* value);
int mc_buffer_read_u64(mc_buffer_t* buffer, uint64_t* value);
int mc_buffer_read_i8(mc_buffer_t* buffer, int8_t* value);
int mc_buffer_read_i16(mc_buffer_t* buffer, int16_t* value);
int mc_buffer_read_i32(mc_buffer_t* buffer, int32_t* value);
int mc_buffer_read_i64(mc_buffer_t* buffer, int64_t* value);
int mc_buffer_read_float(mc_buffer_t* buffer, float* value);
int mc_buffer_read_double(mc_buffer_t* buffer, double* value);
int mc_buffer_read_data(mc_buffer_t* buffer, unsigned char** data, int len);
int mc_buffer_read_string(mc_buffer_t* buffer, mc_string_t* str);
int mc_buffer_read_slot(mc_buffer_t* buffer, mc_slot_t* slot);

#endif  /* SRC_UTILS_BUFFER_H_ */
