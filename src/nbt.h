#ifndef SRC_NBT_H_
#define SRC_NBT_H_

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
  kNBTByteArray,
  kNBTIntArray,
  kNBTString,

  /* High-order structures */
  kNBTList,
  kNBTCompound
};

struct mc_nbt_value_s {
  mc_nbt_value_type_t type;
  union {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f32;
    double d64;
    int8_t i8_list[1];
    int32_t i32_list[1];
    mc_nbt_value_t* list[1];
    struct {
      mc_nbt_value_t* keys[1];
      mc_nbt_value_t* values[1];
    } compound;
  } val;
};

mc_nbt_value_t* mc_nbt_parse(unsigned char* data, int len, mc_nbt_comp_t comp);
void mc_nbt_destroy(mc_nbt_value_t* val);

#endif  /* SRC_NBT_H_ */
