#ifndef SRC_NBT_PRIVATE_H_
#define SRC_NBT_PRIVATE_H_

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

#endif  /* SRC_NBT_PRIVATE_H_ */
