#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* strdup */
#include "world.h"
#include "nbt.h"


mc_world_t* mc_world_new(const char* path) {
  mc_world_t* res;

  res = malloc(sizeof(*res));
  if (res == NULL)
    goto malloc_failed;

  res->path = strdup(path);
  if (res->path == NULL)
    goto strdup_failed;

strdup_failed:
  free(res);

malloc_failed:
  return NULL;
}


void mc_world_destroy(mc_world_t* world) {
  free(world->path);
  free(world);
}
