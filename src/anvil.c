#include <arpa/inet.h>  /* ntohl */
#include <stdint.h>  /* uint32_t, uint8_t */
#include <stdlib.h>  /* calloc, free, NULL */

#include "nbt.h"
#include "common.h"
#include "common-private.h"  /* ARRAY_SIZE */

static const int kHeaderSize = 1024;  /* 32 * 32 */
static const int kSectorSize = 4096;
static const char kLevel[] = "Level";
static const char kTerrainPopulated[] = "TerrainPopulated";
static const char kXPos[] = "xPos";
static const char kYPos[] = "yPos";
static const char kLastUpdate[] = "LastUpdate";

int mc_anvil_parse(const unsigned char* data, int len, mc_region_t** out) {
  mc_region_t* res;
  mc_column_t* col;
  mc_nbt_value_t* nbt;
  int offset;
  int32_t body_len;
  uint8_t comp;
  int max_x;
  int max_y;
  int max_z;
  int x;
  int y;
  int z;

  /* Read .mca headers first */
  if (kHeaderSize > len)
    return -1;

  /* Allocate space for the result */
  res = calloc(1, sizeof(*res));
  if (res == NULL)
    return -1;

  max_x = ARRAY_SIZE(res->column);
  max_z = ARRAY_SIZE(res->column[0]);
  max_y = ARRAY_SIZE(res->column[0][0].chunks);

  /* Read .mca column by column */
  for (x = 0; x < max_x; x++) {
    for (z = 0; z < max_z; z++) {
      offset = (ntohl(*(uint32_t*) (data + 4 * (x + z * max_x))) >> 8) *
               kSectorSize;
      if (offset + 5 > len)
        goto fatal;

      body_len = ntohl(*(int32_t*) (data + offset));
      comp = data[offset + 4];
      col = &res->column[x][z];

      /* Not generated yet */
      if (body_len < 0)
        continue;

      if (body_len + offset + 4 > len)
        goto fatal;

      nbt = mc_nbt_parse(data + offset + 5,
                         body_len - 1,
                         comp == 1 ? kNBTGZip : kNBTDeflate);
      if (nbt == NULL)
        goto fatal;

      mc_nbt_destroy(nbt);
    }
  }

  *out = res;

  return 0;

fatal:
  mc_region_destroy(res);

  return -1;
}
