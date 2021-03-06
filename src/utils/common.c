#include <arpa/inet.h>  /* ntohs */
#include <assert.h>  /* assert */
#include <fcntl.h>  /* open, close */
#include <stdio.h>  /* rename */
#include <stdlib.h>  /* malloc */
#include <string.h>  /* memcpy, strncmp */
#include <sys/stat.h>  /* stat */
#include <unistd.h>  /* read, write */

#include "utils/common.h"
#include "format/nbt.h"  /* mc_nbt_destroy */

static void mc_chunk__destroy(mc_chunk_t* block);
static void mc_block__destroy(mc_block_t* block);

static const char kBackupSuffix[] = ".backup";
static const char kTmpSuffix[] = ".tmp";


void mc_region_destroy(mc_region_t* region) {
  int x;
  int y;
  int z;
  int i;
  mc_column_t* col;

  for (x = 0; x < kMCColumnMaxX; x++) {
    for (z = 0; z < kMCColumnMaxZ; z++) {
      col = &region->columns[x][z];

      /* Free chunks */
      for (y = 0; y < kMCColumnMaxY; y++) {
        if (col->chunks[y] == NULL)
          continue;
        mc_chunk__destroy(col->chunks[y]);
        col->chunks[y] = NULL;
      }

      /* Free entities */
      for (i = 0; i < col->entity_count; i++) {
        mc_nbt_destroy(col->entities[i].nbt);
        col->entities[i].nbt = NULL;
      }
      free(col->entities);
      col->entities = NULL;
      col->entity_count = 0;
    }
  }
  free(region);
}


void mc_chunk__destroy(mc_chunk_t* chunk) {
  int x;
  int y;
  int z;

  for (x = 0; x < kMCChunkMaxX; x++)
    for (z = 0; z < kMCChunkMaxZ; z++)
      for (y = 0; y < kMCChunkMaxY; y++)
        mc_block__destroy(&chunk->blocks[x][z][y]);
  free(chunk);
}


void mc_block__destroy(mc_block_t* block) {
  if (block->tile_data != NULL) {
    mc_nbt_destroy(block->tile_data);
    block->tile_data = NULL;
  }
}

#define ENTITY_TO_STR_DECL(id, value, str) \
    if (len == (sizeof(str) - 1) && strncmp(val, str, len) == 0) { \
      return kMCEntity##id; \
    } else

mc_entity_id_t mc_entity_str_to_id(const char* val, int len) {
  MC_ENTITY_LIST(ENTITY_TO_STR_DECL) {
    return kMCEntityUnknown;
  }
}


void mc_slot_init(mc_slot_t* slot) {
  slot->id = 0;
  slot->count = 0;
  slot->damage = 0;
  slot->nbt_len = 0;
  slot->nbt = NULL;
  slot->allocated = 0;
}

void mc_slot_destroy(mc_slot_t* slot) {
  if (slot->allocated) {
    free(slot->nbt);
    slot->nbt = NULL;
  }
}


int mc_read_file(const char* path, unsigned char** out) {
  int r;
  int off;
  int fd;
  int len;
  unsigned char* res;
  struct stat s;

  fd = open(path, O_RDONLY);
  if (fd == -1)
    goto open_failed;

  r = fstat(fd, &s);
  if (r != 0)
    goto fstat_failed;

  len = s.st_size;
  res = malloc(len);
  if (res == NULL)
    goto fstat_failed;

  off = 0;
  while (off < len) {
    r = read(fd, res + off, len - off);
    if (r <= 0)
      goto read_failed;
    off += r;
  }

  *out = res;
  close(fd);
  return len;

read_failed:
  free(res);

fstat_failed:
  close(fd);

open_failed:
  return -1;
}


int mc_write_file(const char* path,
                  const unsigned char* out,
                  int len,
                  int update) {
  int r;
  int fd;
  int off;
  int path_len;
  char* backup_path;
  char* tmp_path;

  path_len = strlen(path);

  /* Prepare filenames */
  if (update) {
    backup_path = malloc(path_len + sizeof(kBackupSuffix));
    tmp_path = malloc(path_len + sizeof(kTmpSuffix));
    if (backup_path == NULL || tmp_path == NULL)
      goto concat_failed;
    memcpy(backup_path, path, path_len);
    memcpy(tmp_path, path, path_len);
    memcpy(backup_path + path_len, kBackupSuffix, sizeof(kBackupSuffix));
    memcpy(tmp_path + path_len, kTmpSuffix, sizeof(kTmpSuffix));
  } else {
    backup_path = NULL;
    tmp_path = NULL;
  }

  /* First: write data to temporary location */
  fd = open(update ? tmp_path : path, O_WRONLY | O_CREAT | O_TRUNC, 0775);
  if (fd == -1)
    goto concat_failed;

  off = 0;
  while (off < len) {
    r = write(fd, out + off, len - off);
    if (r <= 0)
      goto concat_failed;
    off += r;
  }

  close(fd);

  if (update) {
    /* Second: replace original file */
    r = rename(path, backup_path);
    if (r != 0)
      goto rename_backup_failed;
    r = rename(tmp_path, path);
    if (r != 0)
      goto rename_tmp_failed;
    r = unlink(backup_path);
    if (r != 0)
      goto unlink_failed;
  }

  free(backup_path);
  free(tmp_path);
  return 0;

unlink_failed:
  unlink(path);

rename_tmp_failed:
  /* Try to revert changes */
  rename(backup_path, path);

rename_backup_failed:
  unlink(tmp_path);

concat_failed:
  free(backup_path);
  free(tmp_path);

  return -1;
}
