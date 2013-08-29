#include <assert.h>  /* assert */
#include <stdlib.h>  /* malloc */
#include <string.h>  /* memcpy */

#include "common.h"

void mc_string_init(mc_string_t* str) {
  str->data = NULL;
  str->len = 0;
}


void mc_string_destroy(mc_string_t* str) {
  free(str->data);
  str->data = NULL;
  str->len = 0;
}


int mc_string_copy(mc_string_t* to, mc_string_t* from) {
  assert(to->data == NULL);

  to->data = malloc(from->len);
  if (to->data == NULL)
    return -1;

  memcpy(to->data, from->data, from->len);
  to->len = from->len;

  return 0;
}
