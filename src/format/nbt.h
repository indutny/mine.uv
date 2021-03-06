#ifndef SRC_FORMAT_NBT_H_
#define SRC_FORMAT_NBT_H_

#include <stdint.h>  /* uint8_t, and friends */

#define NBT_GET(obj, prop, type) \
    mc_nbt_get((obj), (prop), sizeof((prop)) - 1, (type))

#define NBT_READ(obj, prop, type, to) \
    do { \
      int r; \
      r = mc_nbt_read((obj), (prop), sizeof((prop)) - 1, (type), (to)); \
      if (r != 0) \
        return r; \
    } while (0)

#define NBT_OPT_READ(obj, prop, type, to, def) \
    do { \
      int r; \
      r = mc_nbt_read((obj), (prop), sizeof((prop)) - 1, (type), (to)); \
      if (r != 0) \
        *(to) = def; \
    } while (0)

#define NBT_SET(obj, prop, type, value) \
    do { \
      int r; \
      r = mc_nbt_set((obj), (prop), sizeof((prop)) - 1, (type), (value)); \
      if (r != 0) \
        return r; \
    } while (0)

#define NBT_OPT_SET(obj, prop, type, value) \
    mc_nbt_set((obj), (prop), sizeof((prop)) - 1, (type), (value));

#define NBT_CREATE(out, type, name, arg) \
    do { \
      (out) = mc_nbt_create_##type((name), sizeof((name)) - 1, (arg)); \
      if ((out) == NULL) \
        goto nbt_fatal; \
    } while (0)

typedef struct mc_nbt_s mc_nbt_t;
typedef struct mc_nbt_parser_s mc_nbt_parser_t;
typedef enum mc_nbt_type_e mc_nbt_type_t;
typedef enum mc_nbt_comp_e mc_nbt_comp_t;
typedef enum mc_nbt_lifetime_e mc_nbt_lifetime_t;

enum mc_nbt_comp_e {
  kNBTUncompressed,
  kNBTDeflate,
  kNBTGZip
};

enum mc_nbt_lifetime_e {
  kSameLifetime,
  kIndependentLifetime
};

struct mc_nbt_parser_s {
  unsigned char* data;
  int len;
  int depth;
  int name_len;

  /* NBT tree has the same lifetime as input data */
  mc_nbt_lifetime_t lifetime;
  unsigned char* uncompressed;
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
      int8_t* list;
    } i8l;
    struct {
      int32_t len;
      int32_t* list;
    } i32l;
    struct {
      int32_t len;
      char* value;
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
mc_nbt_t* mc_nbt_preparse(mc_nbt_parser_t* parser,
                          const unsigned char* data,
                          int len,
                          mc_nbt_comp_t comp,
                          mc_nbt_lifetime_t lifetime);
void mc_nbt_postparse(mc_nbt_parser_t* parser);

/* Encoder API */
int mc_nbt_encode(mc_nbt_t* val, mc_nbt_comp_t comp, unsigned char** out);

/* Utils API */
mc_nbt_t** mc_nbt_find(mc_nbt_t* obj,
                       const char* prop,
                       int len,
                       mc_nbt_type_t type);
mc_nbt_t* mc_nbt_get(mc_nbt_t* obj,
                     const char* prop,
                     int len,
                     mc_nbt_type_t type);
int mc_nbt_set(mc_nbt_t* obj,
               const char* prop,
               int len,
               mc_nbt_type_t type,
               void* val);
int mc_nbt_read(mc_nbt_t* obj,
                const char* prop,
                int len,
                mc_nbt_type_t type,
                void* to);
mc_nbt_t* mc_nbt_clone(const mc_nbt_t* val);

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

#endif  /* SRC_FORMAT_NBT_H_ */
