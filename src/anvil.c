#include <arpa/inet.h>  /* ntohl */
#include <stdint.h>  /* uint32_t, uint8_t */
#include <stdlib.h>

#include "nbt.h"

static const int kHeaderSize = 1024;  /* 32 * 32 */
static const int kSectorSize = 4096;

int mc_anvil_parse(const unsigned char* data, int len, mc_nbt_value_t** out) {
  int sector_offset;
  int sector_count;
  int32_t body_len;
  uint8_t comp;

  if (kHeaderSize > len)
    return -1;

  sector_offset = (ntohl(*(uint32_t*) data) >> 8) * kSectorSize;
  sector_count = data[3];
  if (sector_offset + sector_count > len)
    return -1;

  if (sector_offset + 5 > len)
    return -1;

  body_len = ntohl(*(int32_t*) (data + sector_offset));
  comp = data[sector_offset + 4];

  /* Not generated yet */
  if (body_len == -1) {
    *out = NULL;
    return 0;
  }

  if (body_len + sector_offset + 5> len)
    return -1;

  *out = mc_nbt_parse(data + sector_offset + 5,
                      body_len,
                      comp == 1 ? kNBTGZip : kNBTDeflate);
  if (*out == NULL)
    return -1;

  return 0;
}
