#include <fcntl.h>  /* open, close */
#include <stdio.h>  /* rename */
#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* strdup */
#include <sys/stat.h>  /* stat */
#include <unistd.h>  /* read, write */
#include "world.h"
#include "nbt.h"

static int mc_world__read_file(const char* path, char** out);
static int mc_world__write_file(const char* path, char* out, int len);

static const char kBackupSuffix[] = ".backup";
static const char kTmpSuffix[] = ".tmp";


mc_world_t* mc_world_new(const char* path) {
  mc_world_t* res;

  res = malloc(sizeof(*res));
  if (res == NULL)
    goto malloc_failed;

  res->path = strdup(path);
  if (res->path == NULL)
    goto strdup_failed;

  char* out;
  mc_world__read_file("/tmp/a.c", &out);
  mc_world__write_file("/tmp/a.c", out, strlen(out));

strdup_failed:
  free(res);

malloc_failed:
  return NULL;
}


void mc_world_destroy(mc_world_t* world) {
  free(world->path);
  free(world);
}


int mc_world__read_file(const char* path, char** out) {
  int r;
  int off;
  int fd;
  int len;
  char* res;
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


int mc_world__write_file(const char* path, char* out, int len) {
  int r;
  int fd;
  int off;
  int path_len;
  char* backup_path;
  char* tmp_path;

  path_len = strlen(path);

  /* Prepare filenames */
  backup_path = malloc(path_len + sizeof(kBackupSuffix));
  tmp_path = malloc(path_len + sizeof(kTmpSuffix));
  if (backup_path == NULL || tmp_path == NULL)
    goto concat_failed;
  memcpy(backup_path, path, path_len);
  memcpy(tmp_path, path, path_len);
  memcpy(backup_path + path_len, kBackupSuffix, sizeof(kBackupSuffix));
  memcpy(tmp_path + path_len, kTmpSuffix, sizeof(kTmpSuffix));

  /* First: write data to temporary location */
  fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0775);
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
