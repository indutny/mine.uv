#ifndef SRC_WORLD_H_
#define SRC_WORLD_H_

#include "common.h" /* mc_block_id_t, mc_biome_t */

typedef struct mc_world_s mc_world_t;
typedef struct mc_region_s mc_region_t;
typedef struct mc_column_s mc_column_t;
typedef struct mc_chunk_s mc_chunk_t;
typedef struct mc_block_s mc_block_t;

struct mc_world_s {
  char* path;
  int ref_count;
};

struct mc_block_s {
  mc_block_id_t block_id;
  uint8_t block_metadata;
  uint8_t block_light;
  uint8_t block_skylight;
};

struct mc_chunk_s {
  uint8_t y;
  mc_block_t block[16][16][16];
};

struct mc_column_s {
  mc_biome_t biomes[16][16];
  mc_chunk_t* chunks[16];
};

struct mc_region_s {
  mc_column_t column[32][32];
};


mc_world_t* mc_world_new(const char* path);
void mc_world_retain(mc_world_t* world);
void mc_world_release(mc_world_t* world);

#endif  /* SRC_WORLD_H_ */
