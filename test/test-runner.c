#include <stdio.h>
#include <stdlib.h>

#include "format/anvil.h"
#include "format/nbt.h"
#include "utils/common.h"
#include "world.h"

#define ASSERT(cond, str) \
    if (!(cond)) { \
      fprintf(stderr, "Assertion failed: " str "\n"); \
      abort(); \
    }

static unsigned char nbt_compressed_data[] = {
  31, 139, 8, 0, 0, 0, 0, 0, 0, 0, 237, 84, 207, 79, 26, 65,
  20, 126, 194, 2, 203, 150, 130, 177, 196, 16, 99, 204, 171, 181, 132, 165,
  219, 205, 66, 17, 137, 177, 136, 22, 44, 154, 13, 26, 216, 168, 49, 134,
  184, 43, 195, 130, 46, 187, 102, 119, 176, 241, 212, 75, 123, 108, 122, 235,
  63, 211, 35, 127, 67, 207, 189, 246, 191, 160, 195, 47, 123, 105, 207, 189,
  240, 50, 201, 247, 230, 189, 111, 230, 123, 111, 38, 121, 2, 4, 84, 114,
  79, 44, 14, 120, 203, 177, 77, 141, 120, 244, 227, 112, 98, 62, 8, 123,
  29, 199, 165, 147, 24, 15, 130, 71, 221, 238, 132, 2, 98, 181, 162, 170,
  199, 120, 118, 92, 87, 203, 168, 85, 15, 27, 200, 214, 30, 106, 149, 134,
  134, 13, 173, 126, 88, 123, 143, 131, 207, 131, 79, 131, 111, 207, 3, 16,
  110, 91, 142, 62, 190, 165, 56, 76, 100, 253, 16, 234, 218, 116, 166, 35,
  64, 220, 102, 46, 105, 225, 181, 211, 187, 115, 250, 118, 11, 41, 219, 11,
  224, 239, 232, 61, 30, 56, 91, 239, 17, 8, 86, 245, 222, 93, 223, 11,
  64, 224, 94, 183, 250, 100, 183, 4, 0, 140, 65, 76, 115, 198, 8, 85,
  76, 211, 32, 46, 125, 164, 192, 200, 194, 16, 179, 186, 222, 88, 11, 83,
  163, 238, 68, 142, 69, 3, 48, 177, 39, 83, 140, 76, 241, 233, 20, 163,
  83, 140, 133, 225, 217, 159, 227, 179, 242, 68, 129, 165, 124, 51, 221, 216,
  187, 199, 170, 117, 19, 95, 40, 28, 8, 215, 46, 209, 89, 63, 175, 29,
  27, 96, 33, 89, 223, 250, 241, 5, 254, 193, 206, 252, 157, 189, 0, 188,
  241, 64, 201, 248, 133, 66, 64, 70, 254, 158, 235, 234, 15, 147, 58, 104,
  135, 96, 187, 235, 50, 55, 163, 40, 10, 142, 187, 245, 208, 105, 99, 202,
  78, 219, 233, 236, 230, 230, 43, 59, 189, 37, 190, 100, 73, 9, 61, 170,
  187, 148, 253, 24, 126, 232, 210, 14, 218, 111, 21, 76, 177, 104, 62, 43,
  225, 155, 156, 132, 153, 188, 132, 5, 9, 101, 89, 22, 69, 0, 255, 47,
  40, 174, 47, 242, 194, 178, 164, 46, 29, 32, 119, 90, 59, 185, 140, 202,
  231, 41, 223, 81, 65, 201, 22, 181, 197, 109, 161, 42, 173, 44, 197, 49,
  127, 186, 122, 146, 142, 94, 157, 95, 248, 18, 5, 35, 27, 209, 246, 183,
  119, 170, 205, 149, 114, 188, 158, 223, 88, 93, 75, 151, 174, 146, 23, 185,
  68, 208, 128, 200, 250, 62, 191, 179, 220, 84, 203, 7, 117, 110, 163, 182,
  118, 89, 146, 147, 169, 220, 81, 80, 153, 107, 204, 53, 230, 26, 255, 87,
  35, 8, 66, 203, 233, 27, 214, 120, 194, 236, 254, 252, 122, 251, 125, 120,
  211, 132, 223, 212, 242, 164, 251, 8, 6, 0, 0
};


void test_nbt_predefined() {
  mc_nbt_t* val;

  val = mc_nbt_parse(nbt_compressed_data,
                     sizeof(nbt_compressed_data),
                     kNBTGZip);
  ASSERT(val != NULL, "NBT parse failed");
  ASSERT(val->type == kNBTCompound, "NBT's top level tag should be compound");
  mc_nbt_destroy(val);
}


void test_nbt_cycle() {
  int r;
  mc_nbt_t* res;
  mc_nbt_t* val;
  unsigned char* out;

  /* Generate data */
  res = mc_nbt_create_compound("mine.uv", 7, 3);
  ASSERT(res != NULL, "Create compound failed");
  val = mc_nbt_create_i16("year", 4, 2013);
  ASSERT(val != NULL, "Create i16 failed");
  res->value.values.list[0] = val;
  val = mc_nbt_create_i8("version", 7, 1);
  ASSERT(val != NULL, "Create i8 failed");
  res->value.values.list[1] = val;
  val = mc_nbt_create_str("license", 7, "MIT", 3);
  ASSERT(val != NULL, "Create str failed");
  res->value.values.list[2] = val;

  /* Encode it */
  r = mc_nbt_encode(res, kNBTGZip, &out);
  ASSERT(r > 0, "Encode failed");

  /* Destroy */
  mc_nbt_destroy(res);

  val = mc_nbt_parse(out, r, kNBTGZip);
  ASSERT(val != NULL, "Parse failed");
  ASSERT(val->type == kNBTCompound, "Not compound root");
  ASSERT(val->value.values.len == 3, "Not enough items");
  ASSERT(val->value.values.list[0]->type == kNBTShort, "Not short");
  ASSERT(val->value.values.list[1]->type == kNBTByte, "Not byte");
  ASSERT(val->value.values.list[2]->type == kNBTString, "Not string");
  mc_nbt_destroy(val);
  free(out);
}


void test_anvil() {
  int r;
  int len;
  unsigned char* out;
  mc_region_t* reg;

  len = mc_read_file("./test/anvil.mca", &out);
  ASSERT(len > 0, "Read file failed");

  r = mc_anvil_parse(out, len, &reg);
  ASSERT(r == 0, "Anvil parse failed");
  mc_region_destroy(reg);
}


int main() {
  fprintf(stdout, "Running tests...\n");
  test_nbt_predefined();
  test_nbt_cycle();
  test_anvil();
  fprintf(stdout, "Done!\n");

  return 0;
}
