#include <string.h>  /* memset */

#include "format/anvil.h"
#include "format/nbt.h"  /* mc_nbt_t */
#include "utils/buffer.h"  /* mc_buffer_t */
#include "utils/common.h"  /* mc_region_t */


static int mc_anvil__encode(mc_buffer_t* b, mc_region_t* reg);
static mc_nbt_t* mc_anvil__encode_column(mc_column_t* col);
static mc_nbt_t* mc_anvil__encode_chunk(mc_chunk_t* chunk,
                                        int chunk_y,
                                        int* tile_count);
static int mc_anvil__update_entity(mc_entity_t* entity);

int mc_anvil_encode(mc_region_t* reg, unsigned char** out) {
  int r;
  mc_buffer_t buf;

  r = mc_buffer_init(&buf, 0);
  if (r != 0)
    return r;

  r = mc_anvil__encode(&buf, reg);
  if (r != 0) {
    mc_buffer_destroy(&buf);
    return r;
  }

  *out = mc_buffer_data(&buf);
  return mc_buffer_len(&buf);
}


int mc_anvil__encode(mc_buffer_t* b, mc_region_t* reg) {
  int x;
  int z;
  mc_nbt_t* columns[MC_COLUMN_MAX_X][MC_COLUMN_MAX_Z];

  memset(columns, 0, MC_COLUMN_MAX_X * MC_COLUMN_MAX_Z * sizeof(**columns));

  /* Translate every column to NBT first */
  for (x = 0; x < kMCColumnMaxX; x++) {
    for (z = 0; z < kMCColumnMaxZ; z++) {
      if (!reg->columns[x][z].generated)
        continue;

      columns[x][z] = mc_anvil__encode_column(&reg->columns[x][z]);
      if (columns[x][z] == NULL)
        goto fatal;
    }
  }

  return 0;

fatal:
  for (x = 0; x < kMCColumnMaxX; x++)
    for (z = 0; z < kMCColumnMaxZ; z++)
      if (columns[x][z] != NULL)
        mc_nbt_destroy(columns[x][z]);

  return -1;
}


mc_nbt_t* mc_anvil__encode_column(mc_column_t* col) {
  int r;
  int i;
  int x;
  int y;
  int z;
  int off;
  int chunk_count;
  int tile_count;
  mc_nbt_t* res;
  mc_nbt_t* level;
  mc_nbt_t* biomes;
  mc_nbt_t* height_map;
  mc_nbt_t* chunks;
  mc_nbt_t* chunk;
  mc_nbt_t* entities;
  mc_nbt_t* tiles;

  NBT_CREATE(res, compound, "", 1);
  NBT_CREATE(level, compound, "Level", 10);
  res->value.values.list[0] = level;

  NBT_CREATE(level->value.values.list[0], i32, "xPos", col->world_x);
  NBT_CREATE(level->value.values.list[1], i32, "zPos", col->world_z);
  NBT_CREATE(level->value.values.list[2], i64, "LastUpdate", col->last_update);
  NBT_CREATE(level->value.values.list[3],
             i8,
             "TerrainPopulated",
             col->populated);
  NBT_CREATE(level->value.values.list[4],
             i64,
             "InhabitedTime",
             col->inhabited_time);
  NBT_CREATE(biomes, i8l, "Biomes", kMCChunkMaxX * kMCChunkMaxZ);
  level->value.values.list[5] = biomes;
  NBT_CREATE(height_map, i32l, "HeightMap", kMCChunkMaxX * kMCChunkMaxZ);
  level->value.values.list[6] = height_map;

  for (x = 0; x < kMCChunkMaxX; x++) {
    for (z = 0; z < kMCChunkMaxZ; z++) {
      off = x + z * kMCChunkMaxX;
      biomes->value.i8l.list[off] = (int8_t) col->biomes[x][z];
      height_map->value.i8l.list[off] = col->height_map[x][z];
    }
  }

  /* Count chunks */
  chunk_count = 0;
  for (y = 0; y < kMCChunkMaxY; y++)
    if (col->chunks[y] != NULL)
      chunk_count++;

  NBT_CREATE(chunks, compound, "Sections", chunk_count);
  level->value.values.list[7] = chunks;

  /* Put chunks and count tiles */
  tile_count = 0;
  chunk_count = 0;
  for (y = 0; y < kMCChunkMaxY; y++) {
    if (col->chunks[y] == NULL)
      continue;
    chunk = mc_anvil__encode_chunk(col->chunks[y], y, &tile_count);
    if (chunk == NULL)
      goto nbt_fatal;

    chunks->value.values.list[chunk_count++] = chunk;
  }

  /* Put entities */
  NBT_CREATE(entities, compound, "Entities", col->entity_count);
  level->value.values.list[8] = entities;
  for (i = 0; i < col->entity_count; i++) {
    r = mc_anvil__update_entity(&col->entities[i]);
    if (r != 0)
      goto nbt_fatal;

    entities->value.values.list[i] = col->entities[i].nbt;
  }

  /* Put tiles */
  NBT_CREATE(tiles, compound, "TileEntities", tile_count);
  level->value.values.list[9] = tiles;

  return res;

nbt_fatal:
  if (res != NULL)
    mc_nbt_destroy(res);
  return NULL;
}


mc_nbt_t* mc_anvil__encode_chunk(mc_chunk_t* chunk,
                                 int chunk_y,
                                 int* tile_count) {
  int size;
  int off;
  int x;
  int y;
  int z;
  mc_nbt_t* res;
  int8_t* blocks;
  int8_t* add;
  int8_t* data;
  int8_t* block_light;
  int8_t* sky_light;

  NBT_CREATE(res, compound, "", 6);
  NBT_CREATE(res->value.values.list[0], i8, "Y", chunk_y);

  size = kMCChunkMaxX * kMCChunkMaxZ * kMCChunkMaxY;
  NBT_CREATE(res->value.values.list[1], i8l, "Blocks", size);
  blocks = res->value.values.list[1]->value.i8l.list;
  NBT_CREATE(res->value.values.list[2], i8l, "Add", size / 2);
  add = res->value.values.list[2]->value.i8l.list;
  NBT_CREATE(res->value.values.list[3], i8l, "Data", size / 2);
  data = res->value.values.list[3]->value.i8l.list;
  NBT_CREATE(res->value.values.list[4], i8l, "BlockLight", size / 2);
  block_light = res->value.values.list[4]->value.i8l.list;
  NBT_CREATE(res->value.values.list[5], i8l, "SkyLight", size / 2);
  sky_light = res->value.values.list[5]->value.i8l.list;

  memset(add, 0, size / 2);
  memset(data, 0, size / 2);
  memset(block_light, 0, size / 2);
  memset(sky_light, 0, size / 2);

  /* Fill arrays */
  for (x = 0; x < kMCChunkMaxX; x++) {
    for (z = 0; z < kMCChunkMaxZ; z++) {
      for (y = 0; y < kMCChunkMaxY; y++) {
        off = x + z * kMCChunkMaxX  + y * kMCChunkMaxZ * kMCChunkMaxX;

        blocks[off] = chunk->blocks[x][z][y].id & 0xff;

        /* Put 4-bit values */
        add[off >> 1] <<= 4;
        data[off >> 1] <<= 4;
        block_light[off >> 1] <<= 4;
        sky_light[off >> 1] <<= 4;

        add[off >> 1] |= (chunk->blocks[x][z][y].id >> 8) & 0xf;
        data[off >> 1] |= chunk->blocks[x][z][y].metadata & 0xf;
        block_light[off >> 1] |= chunk->blocks[x][z][y].light & 0xf;
        sky_light[off >> 1] |= chunk->blocks[x][z][y].skylight & 0xf;

        if (chunk->blocks[x][z][y].tile_data != NULL)
          *tile_count = *tile_count + 1;
      }
    }
  }

  return res;

nbt_fatal:
  if (res != NULL)
    mc_nbt_destroy(res);
  return NULL;
}


static int mc_anvil__update_entity(mc_entity_t* entity) {
  if (entity->nbt == NULL)
    return -1;

  return 0;
}
