#ifndef SRC_NBT_PRIVATE_H_
#define SRC_NBT_PRIVATE_H_

#include "nbt.h"  /* mc_nbt_comp_t */

typedef enum mc_nbt__tag_e mc_nbt__tag_t;

enum mc_nbt__tag_e {
  kNBTTagEnd = 0,
  kNBTTagByte = 1,
  kNBTTagShort = 2,
  kNBTTagInt = 3,
  kNBTTagLong = 4,
  kNBTTagFloat = 5,
  kNBTTagDouble = 6,
  kNBTTagByteArray = 7,
  kNBTTagString = 8,
  kNBTTagList = 9,
  kNBTTagCompound = 10,
  kNBTTagIntArray = 11
};

static const int kGZipBits = 31;
static const int kDeflateBits = 15;

int mc_nbt__zlib(const unsigned char* data,
                 int len,
                 mc_nbt_comp_t comp,
                 int decompress,
                 unsigned char** out);

#endif  /* SRC_NBT_PRIVATE_H_ */
