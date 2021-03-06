#include <arpa/inet.h>  /* ntohl */
#include <stdint.h>  /* uint32_t, uint8_t */
#include <stdlib.h>  /* calloc, free, NULL */
#include <string.h>  /* memcpy */

#include "format/anvil.h"
#include "format/nbt.h"
#include "utils/common.h"

static int mc_anvil__parse_column(mc_nbt_t* nbt, mc_column_t* col);
static int mc_anvil__parse_biomes(mc_nbt_t* level, mc_column_t* col);
static int mc_anvil__parse_chunks(mc_nbt_t* level, mc_column_t* col);
static mc_entity_t* mc_anvil__parse_entities(mc_nbt_t* level, int* count);
static int mc_anvil__parse_entity(mc_nbt_t* nbt, mc_entity_t* entity);
static int mc_anvil__parse_tile_entities(mc_nbt_t* nbt, mc_column_t* col);
static int mc_anvil__parse_height_map(mc_nbt_t* nbt, mc_column_t* col);

static const int kHeaderSize = 1024;  /* 32 * 32 */
static const int kSectorSize = 4096;

int mc_anvil_parse(const unsigned char* data, int len, mc_region_t** out) {
  int r;
  mc_nbt_parser_t nbt_parser;
  mc_region_t* res;
  mc_column_t* col;
  mc_nbt_t* nbt;
  int offset;
  int32_t body_len;
  uint8_t comp;
  int x;
  int z;

  /* Read .mca headers first */
  if (kHeaderSize > len)
    return -1;

  /* Allocate space for the result */
  res = calloc(1, sizeof(*res));
  if (res == NULL)
    return -1;

  /* Read .mca column by column */
  for (x = 0; x < kMCColumnMaxX; x++) {
    for (z = 0; z < kMCColumnMaxZ; z++) {
      offset = 4 * (x + z * kMCColumnMaxX);
      offset = (ntohl(*(uint32_t*) (data + offset)) >> 8) * kSectorSize;

      /* Not generated yet */
      if (offset == 0)
        continue;

      if (offset + 5 > len)
        goto fatal;

      body_len = ntohl(*(int32_t*) (data + offset));
      comp = data[offset + 4];
      col = &res->columns[x][z];

      /* Not generated yet */
      if (body_len < 0)
        continue;

      if (body_len + offset + 4 > len)
        goto fatal;

      nbt = mc_nbt_preparse(&nbt_parser,
                            data + offset + 5,
                            body_len,
                            comp == 1 ? kNBTGZip : kNBTDeflate,
                            kSameLifetime);
      if (nbt == NULL)
        goto fatal;

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
  NBT_READ(level, "InhabitedTime", kNBTLong, &col->inhabited_time);

  /* Parse biomes */
  r = mc_anvil__parse_biomes(level, col);
  if (r != 0)
    return r;

  /* Parse chunks */
  r = mc_anvil__parse_chunks(level, col);
  if (r != 0)
    return r;

  /* Parse entities */
  col->entities = mc_anvil__parse_entities(level, &col->entity_count);
  if (col->entities == NULL)
    return -1;

  /* Parse tile entities */
  r = mc_anvil__parse_tile_entities(level, col);
  if (r != 0)
    return -1;

  /* Parse height map */
  r = mc_anvil__parse_height_map(level, col);
  if (r != 0)
    return -1;

  col->generated = 1;

  return 0;
}


int mc_anvil__parse_biomes(mc_nbt_t* level, mc_column_t* col) {
  int x;
  int z;
  int off;
  int array_size;
  mc_nbt_t* biomes;

  array_size = kMCChunkMaxX * kMCChunkMaxZ;
  biomes = NBT_GET(level, "Biomes", kNBTByteArray);
  if (biomes == NULL || biomes->value.i8l.len != array_size)
    return -1;
  for (x = 0; x < kMCChunkMaxX; x++) {
    for (z = 0; z < kMCChunkMaxZ; z++) {
      off = x + z * kMCChunkMaxX;
      col->biomes[x][z] = (mc_biome_t) biomes->value.i8l.list[off];
    }
  }

  return 0;
}


int mc_anvil__parse_chunks(mc_nbt_t* level, mc_column_t* col) {
  int i;
  int x;
  int8_t y;
  int z;
  int off;
  int array_size;
  uint8_t block_light;
  uint8_t sky_light;
  uint8_t block_data;
  uint16_t block_add;
  mc_nbt_t* chunks;
  mc_nbt_t* chunk;
  mc_nbt_t* blocks;
  mc_nbt_t* block_lights;
  mc_nbt_t* sky_lights;
  mc_nbt_t* block_datas;
  mc_nbt_t* block_adds;
  mc_chunk_t* mchunk;
  mc_block_t* block;

  chunks = NBT_GET(level, "Sections", kNBTList);
  if (chunks == NULL)
    return -1;
  for (i = 0; i < chunks->value.values.len; i++) {
    chunk = chunks->value.values.list[i];

    NBT_READ(chunk, "Y", kNBTByte, &y);
    if (y < 0 || y >= kMCColumnMaxY)
      goto read_chunks_failed;

    mchunk = malloc(sizeof(*mchunk));
    if (mchunk == NULL)
      goto read_chunks_failed;
    col->chunks[y] = mchunk;

    /* Read block ids */
    array_size = kMCChunkMaxX * kMCChunkMaxZ * kMCChunkMaxY;
    blocks = NBT_GET(chunk, "Blocks", kNBTByteArray);
    block_lights = NBT_GET(chunk, "BlockLight", kNBTByteArray);
    sky_lights = NBT_GET(chunk, "SkyLight", kNBTByteArray);
    block_datas = NBT_GET(chunk, "Data", kNBTByteArray);
    block_adds = NBT_GET(chunk, "Add", kNBTByteArray);

    if (blocks == NULL || block_lights == NULL || sky_lights == NULL)
      goto read_chunks_failed;
    if (blocks->value.i8l.len != array_size ||
        block_lights->value.i8l.len != (array_size / 2) ||
        sky_lights->value.i8l.len != (array_size / 2) ||
        block_datas->value.i8l.len != (array_size / 2) ||
        (block_adds != NULL && block_adds->value.i8l.len != (array_size / 2))) {
      goto read_chunks_failed;
    }

    for (x = 0; x < kMCChunkMaxX; x++) {
      for (z = 0; z < kMCChunkMaxZ; z++) {
        for (y = 0; y < kMCChunkMaxY; y++) {
          off = x + z * kMCChunkMaxX + y * kMCChunkMaxX * kMCChunkMaxZ;

          block = &mchunk->blocks[x][z][y];
          if (block_adds == NULL) {
            block_add = 0;
          } else {
            block_add = block_adds->value.i8l.list[off >> 1];
            if (off % 2 == 0)
              block_add = block_add >> 4;
            else
              block_add = block_add & 0xf;
          }
          block_add = block_add << 8;
          block->id = (mc_block_id_t) (block_add | blocks->value.i8l.list[off]);

          block_light = (uint8_t) block_lights->value.i8l.list[off >> 1];
          sky_light = (uint8_t) sky_lights->value.i8l.list[off >> 1];
          block_data = (uint8_t) block_datas->value.i8l.list[off >> 1];
          if (off % 2 == 0) {
            block->light = block_light >> 4;
            block->skylight = sky_light >> 4;
            block->metadata = block_data >> 4;
          } else {
            block->light = block_light & 0xf;
            block->skylight = sky_light & 0xf;
            block->metadata = block_data & 0xf;
          }

          block->tile_data = NULL;
        }
      }
    }
  }

  return 0;

read_chunks_failed:
  /* Deallocate all read chunks */
  for (i = 0; i < kMCColumnMaxY; i++) {
    free(col->chunks[i]);
    col->chunks[i] = NULL;
  }

  return -1;
}


mc_entity_t* mc_anvil__parse_entities(mc_nbt_t* level, int* count) {
  mc_nbt_t* entities;
  mc_nbt_t* entity;
  mc_entity_t* res;
  int i;
  int r;

  entities = NBT_GET(level, "Entities", kNBTList);
  if (entities == NULL)
    goto get_entities_failed;

  res = malloc(sizeof(*res) * entities->value.values.len);
  if (res == NULL)
    goto get_entities_failed;

  for (i = 0; i < entities->value.values.len; i++) {
    entity = entities->value.values.list[i];
    r = mc_anvil__parse_entity(entity, &res[i]);
    if (r != 0)
      goto read_entities_failed;
  }
  *count = entities->value.values.len;

  return res;

read_entities_failed:
  while (--i >= 0) {
    mc_nbt_destroy(res[i].nbt);
    res[i].nbt = NULL;
  }
  free(res);

get_entities_failed:
  return NULL;
}


int mc_anvil__parse_entity(mc_nbt_t* nbt, mc_entity_t* entity) {
  mc_nbt_t* id;
  mc_nbt_t* list;

  id = NBT_GET(nbt, "id", kNBTString);
  if (id == NULL)
    entity->id = kMCEntityPlayer;
  else
    entity->id = mc_entity_str_to_id(id->value.str.value, id->value.str.len);

  NBT_OPT_READ(nbt, "OnGround", kNBTByte, &entity->on_ground, 0);
  NBT_OPT_READ(nbt, "Invulnerable", kNBTByte, &entity->invulnerable, 0);
  NBT_OPT_READ(nbt, "Air", kNBTShort, &entity->air, 0);
  NBT_OPT_READ(nbt, "Fire", kNBTShort, &entity->fire, 0);
  NBT_OPT_READ(nbt, "FallDistance", kNBTFloat, &entity->fall_distance, 0.0);
  NBT_READ(nbt, "UUIDMost", kNBTLong, &entity->uuid.obj.high);
  NBT_READ(nbt, "UUIDLeast", kNBTLong, &entity->uuid.obj.low);

  list = NBT_GET(nbt, "Pos", kNBTList);
  if (list == NULL ||
      list->value.values.len != 3 ||
      list->value.values.list[0]->type != kNBTDouble) {
    return -1;
  }
  entity->pos_x = list->value.values.list[0]->value.f64;
  entity->pos_y = list->value.values.list[1]->value.f64;
  entity->pos_z = list->value.values.list[2]->value.f64;

  list = NBT_GET(nbt, "Motion", kNBTList);
  if (list == NULL ||
      list->value.values.len != 3 ||
      list->value.values.list[0]->type != kNBTDouble) {
    return -1;
  }
  entity->motion_x = list->value.values.list[0]->value.f64;
  entity->motion_y = list->value.values.list[1]->value.f64;
  entity->motion_z = list->value.values.list[2]->value.f64;

  list = NBT_GET(nbt, "Rotation", kNBTList);
  if (list == NULL ||
      list->value.values.len != 2 ||
      list->value.values.list[0]->type != kNBTFloat) {
    return -1;
  }
  entity->yaw = list->value.values.list[0]->value.f32;
  entity->pitch = list->value.values.list[1]->value.f32;

  entity->nbt = mc_nbt_clone(nbt);
  if (entity->nbt == NULL)
    return -1;

  return 0;
}


int mc_anvil__parse_tile_entities(mc_nbt_t* nbt, mc_column_t* col) {
  int i;
  int32_t x;
  int32_t y;
  int32_t z;
  int chunk_y;
  int chunk_y_off;
  mc_nbt_t* list;
  mc_nbt_t* tile;
  mc_chunk_t* chunk;

  list = NBT_GET(nbt, "TileEntities", kNBTList);
  if (list == NULL || list->value.values.len == 0)
    return 0;

  if (list->value.values.list[0]->type != kNBTCompound)
    return -1;

  for (i = 0; i < list->value.values.len; i++) {
    tile = mc_nbt_clone(list->value.values.list[i]);
    if (tile == NULL)
      return -1;

    NBT_READ(tile, "x", kNBTInt, &x);
    NBT_READ(tile, "y", kNBTInt, &y);
    NBT_READ(tile, "z", kNBTInt, &z);

    chunk_y = y / kMCChunkMaxY;
    chunk_y_off = y % kMCChunkMaxY;
    if (chunk_y < 0 || chunk_y > kMCColumnMaxY)
      return -1;

    x = x % kMCChunkMaxX;
    z = z % kMCChunkMaxZ;

    chunk = col->chunks[chunk_y];
    if (chunk == NULL)
      return -1;
    chunk->blocks[x][z][chunk_y_off].tile_data = tile;
  }

  return 0;
}


int mc_anvil__parse_height_map(mc_nbt_t* nbt, mc_column_t* col) {
  mc_nbt_t* map;
  int x;
  int z;

  map = NBT_GET(nbt, "HeightMap", kNBTIntArray);
  if (map == NULL || map->value.i32l.len != kMCChunkMaxX * kMCChunkMaxZ)
    return -1;

  for (x = 0; x < kMCChunkMaxX; x++)
    for (z = 0; z < kMCChunkMaxZ; z++)
      col->height_map[x][z] = map->value.i32l.list[x + z * kMCChunkMaxX];

  return 0;
}
