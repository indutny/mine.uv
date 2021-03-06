#include <arpa/inet.h>  /* ntohs */
#include <assert.h>  /* assert */
#include <stdlib.h>  /* malloc, free, NULL */
#include <string.h>  /* memcpy, strlen */

#include "utils/string.h"


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
