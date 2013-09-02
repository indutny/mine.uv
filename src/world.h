#ifndef SRC_WORLD_H_
#define SRC_WORLD_H_

#include "common.h" /* mc_block_id_t, mc_biome_t */

typedef struct mc_world_s mc_world_t;


struct mc_world_s {
  char* path;
  int ref_count;
};


mc_world_t* mc_world_new(const char* path);
void mc_world_retain(mc_world_t* world);
void mc_world_release(mc_world_t* world);

#endif  /* SRC_WORLD_H_ */
