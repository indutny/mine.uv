#include <arpa/inet.h>  /* ntohl */
#include <stdint.h>  /* uint32_t, uint8_t */
#include <stdlib.h>  /* calloc, free, NULL */
#include <string.h>  /* memcpy */

#include "nbt.h"
#include "common.h"
#include "common-private.h"  /* ARRAY_SIZE */

static int mc_anvil__parse_column(mc_nbt_t* nbt, mc_column_t* col);
static int mc_anvil__parse_biomes(mc_nbt_t* level, mc_column_t* col);
static int mc_anvil__parse_chunks(mc_nbt_t* level, mc_column_t* col);

static const int kHeaderSize = 1024;  /* 32 * 32 */
static const int kSectorSize = 4096;

#define NBT_GET(obj, prop, type) \
    mc_nbt_get((obj), (prop), sizeof((prop)) - 1, (type))

#define NBT_READ(obj, prop, type, to) \
    do { \
      int r; \
      r = mc_nbt_read((obj), (prop), sizeof((prop)) - 1, (type), (to)); \
      if (r != 0) \
        return r; \
    } while (0)

int mc_anvil_parse(const unsigned char* data, int len, mc_region_t** out) {
  int r;
  mc_nbt_parser_t nbt_parser;
  mc_region_t* res;
  mc_column_t* col;
  mc_nbt_t* nbt;
  int offset;
  int32_t body_len;
  uint8_t comp;
  int max_x;
  int max_y;
  int max_z;
  int x;
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

      /* Not generated yet */
      if (offset == 0)
        continue;

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

      nbt = mc_nbt_preparse(&nbt_parser,
                            data + offset + 5,
                            body_len - 1,
                            comp == 1 ? kNBTGZip : kNBTDeflate,
                            kSameLifetime);
      if (nbt == NULL)
        goto fatal;

      /* Store cached deflated value */
      if (comp == 2) {
        col->compressed = malloc(body_len - 1);
        if (col->compressed != NULL) {
          col->compressed_len = body_len - 1;
          memcpy(col->compressed, data + offset + 5, col->compressed_len);
        }
      }

      /* Parse column's NBT */
      r = mc_anvil__parse_column(nbt, col);

      /* Destroy NBT data */
      mc_nbt_postparse(&nbt_parser);
      mc_nbt_destroy(nbt);
      if (r != 0)
        goto fatal;
    }
  }

  *out = res;

  return 0;

fatal:
  mc_region_destroy(res);

  return -1;
}


int mc_anvil_encode(mc_region_t* reg, unsigned char** out) {
  return -1;
}


int mc_anvil__parse_column(mc_nbt_t* nbt, mc_column_t* col) {
  int r;
  mc_nbt_t* level;

  level = NBT_GET(nbt, "Level", kNBTCompound);
  if (level == NULL)
    return -1;

  NBT_READ(level, "TerrainPopulated", kNBTByte, &col->populated);
  NBT_READ(level, "xPos", kNBTInt, &col->world_x);
  NBT_READ(level, "zPos", kNBTInt, &col->world_z);
  NBT_READ(level, "LastUpdate", kNBTLong, &col->last_update);

  /* Read biomes */
  r = mc_anvil__parse_biomes(level, col);
  if (r != 0)
    return r;

  /* Read chunks */
  r = mc_anvil__parse_chunks(level, col);
  if (r != 0)
    return r;

  return 0;
}


int mc_anvil__parse_biomes(mc_nbt_t* level, mc_column_t* col) {
  unsigned int x;
  unsigned int z;
  int off;
  int array_size;
  mc_nbt_t* biomes;

  array_size = ARRAY_SIZE(col->biomes) * ARRAY_SIZE(col->biomes[0]);
  biomes = NBT_GET(level, "Biomes", kNBTByteArray);
  if (biomes == NULL || biomes->value.i8l.len != array_size)
    return -1;
  for (x = 0; x < ARRAY_SIZE(col->biomes); x++) {
    for (z = 0; z < ARRAY_SIZE(col->biomes[0]); z++) {
      off = x + z * ARRAY_SIZE(col->biomes);
      col->biomes[x][z] = (mc_biome_t) biomes->value.i8l.list[off];
    }
  }

  return 0;
}


int mc_anvil__parse_chunks(mc_nbt_t* level, mc_column_t* col) {
  int i;
  unsigned x;
  int8_t y;
  unsigned z;
  int off;
  int array_size;
  mc_nbt_t* chunks;
  mc_nbt_t* chunk;
  mc_nbt_t* blocks;
  mc_nbt_t* block_lights;
  mc_nbt_t* sky_lights;
  mc_nbt_t* block_datas;
  mc_chunk_t* mchunk;
  mc_block_t* block;

  chunks = NBT_GET(level, "Sections", kNBTList);
  if (chunks == NULL)
    return -1;
  for (i = 0; i < chunks->value.values.len; i++) {
    chunk = chunks->value.values.list[i];

    NBT_READ(chunk, "Y", kNBTByte, &y);
    if (y < 0 || y >= (int8_t) ARRAY_SIZE(col->chunks))
      goto read_chunks_failed;

    mchunk = malloc(sizeof(*mchunk));
    if (mchunk == NULL)
      goto read_chunks_failed;
    col->chunks[y] = mchunk;

    /* Read block ids */
    array_size = ARRAY_SIZE(mchunk->blocks) *
                 ARRAY_SIZE(mchunk->blocks[0]) *
                 ARRAY_SIZE(mchunk->blocks[0][0]);
    blocks = NBT_GET(chunk, "Blocks", kNBTByteArray);
    block_lights = NBT_GET(chunk, "BlockLight", kNBTByteArray);
    sky_lights = NBT_GET(chunk, "SkyLight", kNBTByteArray);
    block_datas = NBT_GET(chunk, "Data", kNBTByteArray);

    if (blocks == NULL || block_lights == NULL || sky_lights == NULL)
      goto read_chunks_failed;
    if (blocks->value.i8l.len != array_size ||
        block_lights->value.i8l.len != (array_size / 2) ||
        sky_lights->value.i8l.len != (array_size / 2) ||
        block_datas->value.i8l.len != (array_size / 2)) {
      goto read_chunks_failed;
    }

    for (x = 0; x < ARRAY_SIZE(mchunk->blocks); x++) {
      for (z = 0; z < ARRAY_SIZE(mchunk->blocks[0]); z++) {
        for (y = 0; y < (int8_t) ARRAY_SIZE(mchunk->blocks[0][0]); y++) {
          off = x +
                z * ARRAY_SIZE(mchunk->blocks) +
                y * ARRAY_SIZE(mchunk->blocks) * ARRAY_SIZE(mchunk->blocks[0]);

          block = &mchunk->blocks[x][y][z];
          block->id = (mc_block_id_t) blocks->value.i8l.list[off];
          if (off % 2 == 0) {
            block->light =
                (uint8_t) block_lights->value.i8l.list[off >> 1] >> 4;
            block->skylight =
                (uint8_t) sky_lights->value.i8l.list[off >> 1] >> 4;
          } else {
            block->light =
                (uint8_t) block_lights->value.i8l.list[off >> 1] & 0xf;
            block->skylight =
                (uint8_t) sky_lights->value.i8l.list[off >> 1] & 0xf;
          }
        }
      }
    }
  }

  return 0;

read_chunks_failed:
  /* Deallocate all read chunks */
  for (i = 0; i < (int) ARRAY_SIZE(col->chunks); i++) {
    free(col->chunks[i]);
    col->chunks[i] = NULL;
  }

  return -1;
}
