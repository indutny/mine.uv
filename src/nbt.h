#ifndef SRC_NBT_H_
#define SRC_NBT_H_

#include <stdint.h>  /* uint8_t, and friends */

typedef struct mc_nbt_value_s mc_nbt_value_t;
typedef enum mc_nbt_value_type_e mc_nbt_value_type_t;
typedef enum mc_nbt_comp_e mc_nbt_comp_t;

enum mc_nbt_comp_e {
  kNBTUncompressed,
  kNBTDeflate,
  kNBTZip
};

enum mc_nbt_value_type_e {
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

struct mc_nbt_value_s {
  mc_nbt_value_type_t type;
  struct {
    const char* value;
    int32_t len;
  } name;
  union {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
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
      mc_nbt_value_t* list[1];
    } values;
  } value;
};

mc_nbt_value_t* mc_nbt_parse(unsigned char* data, int len, mc_nbt_comp_t comp);
void mc_nbt_destroy(mc_nbt_value_t* val);

#endif  /* SRC_NBT_H_ */
