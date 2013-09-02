#include <arpa/inet.h>  /* ntohs */
#include <assert.h>  /* assert */
#include <fcntl.h>  /* open, close */
#include <stdio.h>  /* rename */
#include <stdlib.h>  /* malloc */
#include <string.h>  /* memcpy */
#include <sys/stat.h>  /* stat */
#include <unistd.h>  /* read, write */

#include "common.h"
#include "common-private.h"  /* ARRAY_SIZE */

static const char kBackupSuffix[] = ".backup";
static const char kTmpSuffix[] = ".tmp";


void mc_region_destroy(mc_region_t* region) {
  unsigned int x;
  unsigned int y;
  unsigned int z;

  for (x = 0; x < ARRAY_SIZE(region->column); x++) {
    for (z = 0; z < ARRAY_SIZE(region->column[x]); z++) {
      for (y = 0; y < ARRAY_SIZE(region->column[x][z].chunks); y++) {
        free(region->column[x][z].chunks[y]);
        region->column[x][z].chunks[y] = NULL;
      }
      free(region->column[x][z].compressed);
      region->column[x][z].compressed = NULL;
    }
  }
  free(region);
}


void mc_string_init(mc_string_t* str) {
  str->data = NULL;
  str->len = 0;
  str->allocated = 0;
}


void mc_string_destroy(mc_string_t* str) {
  if (str->allocated)
    free((char*) str->data);
  str->data = NULL;
  str->len = 0;
  str->allocated = 0;
}


void mc_string_set(mc_string_t* str, const uint16_t* data, int len) {
  str->data = data;
  str->len = len;
  str->allocated = 0;
}


int mc_string_copy(mc_string_t* to, mc_string_t* from) {
  assert(to->data == NULL);

  to->data = malloc(from->len * sizeof(*to->data));
  if (to->data == NULL)
    return -1;

  memcpy((char*) to->data, from->data, from->len * sizeof(*to->data));
  to->allocated = 1;
  to->len = from->len;

  return 0;
}


char* mc_string_to_ascii(mc_string_t* str) {
  unsigned char* result;
  int i;
  int j;
  uint16_t c;

  result = malloc(str->len + 1);
  if (result == NULL)
    return NULL;

  for (i = 0, j = 0; i < str->len; i++) {
    c = ntohs(str->data[i]);

    /* Skip two units */
    if (c == 0x0001) {
      i++;
      continue;
    }

    /* Check if character can be converted to ASCII */
    if ((c & 0xFF80) != 0)
      continue;
    result[j++] = (unsigned char) c;
  }
  result[j] = 0;
  assert(j <= str->len);

  return (char*) result;
}


int mc_string_from_ascii(mc_string_t* to, const char* from) {
  int i;
  int len;
  uint16_t* data;

  assert(to->data == NULL);

  len = strlen(from);
  data = malloc(len * sizeof(*to->data));
  if (data == NULL)
    return -1;

  for (i = 0; i < len; i++)
    data[i] = htons((unsigned char) from[i]);
  to->data = data;
  to->allocated = 1;
  to->len = len;

  return 0;
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
