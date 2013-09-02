#ifndef SRC_NBT_H_
#define SRC_NBT_H_

#include <stdint.h>  /* uint8_t, and friends */

typedef struct mc_nbt_s mc_nbt_t;
typedef enum mc_nbt_type_e mc_nbt_type_t;
typedef enum mc_nbt_comp_e mc_nbt_comp_t;

enum mc_nbt_comp_e {
  kNBTUncompressed,
  kNBTDeflate,
  kNBTGZip
};

enum mc_nbt_type_e {
  /* Primitives */
  kNBTByte,
  kNBTShort,
  kNBTInt,
  kNBTLong,
  kNBTFloat,
  kNBTDouble,

  /* Arrays */
  kNBTByteArray,
  kNBTIntArray,
  kNBTString,

  /* High-order structures */
  kNBTList,
  kNBTCompound
};

struct mc_nbt_s {
  mc_nbt_type_t type;
  struct {
    const char* value;
    int16_t len;
  } name;
  union {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    /*
     * NOTE: Its important to have similar length type in all lists,
     * as we might use it for interoperation
     */
    struct {
      int32_t len;
      int8_t list[1];
    } i8_list;
    struct {
      int32_t len;
      int32_t list[1];
    } i32_list;
    struct {
      int32_t len;
      char value[1];
    } str;
    struct {
      int32_t len;
      mc_nbt_t* list[1];
    } values;
  } value;
};

/* Parser API */
mc_nbt_t* mc_nbt_parse(const unsigned char* data,
                             int len,
                             mc_nbt_comp_t comp);

/* Encoder API */
int mc_nbt_encode(mc_nbt_t* val, mc_nbt_comp_t comp, unsigned char** out);

/* Utils API */
mc_nbt_t** mc_nbt_get(mc_nbt_t* val, const char* prop, int len);

/* Value API */
mc_nbt_t* mc_nbt_create_i8(const char* name, int name_len, int8_t val);
mc_nbt_t* mc_nbt_create_i16(const char* name, int name_len, int16_t val);
mc_nbt_t* mc_nbt_create_i32(const char* name, int name_len, int32_t val);
mc_nbt_t* mc_nbt_create_i64(const char* name, int name_len, int64_t val);
mc_nbt_t* mc_nbt_create_f32(const char* name, int name_len, float val);
mc_nbt_t* mc_nbt_create_f64(const char* name, int name_len, double val);
mc_nbt_t* mc_nbt_create_str(const char* name,
                            int name_len,
                            const char* value,
                            int value_len);
mc_nbt_t* mc_nbt_create_i8l(const char* name, int name_len, int len);
mc_nbt_t* mc_nbt_create_i32l(const char* name, int name_len, int len);
mc_nbt_t* mc_nbt_create_list(const char* name, int name_len, int len);
mc_nbt_t* mc_nbt_create_compound(const char* name, int name_len, int len);

void mc_nbt_destroy(mc_nbt_t* val);

#endif  /* SRC_NBT_H_ */
