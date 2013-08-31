#include <arpa/inet.h>  /* htons, htonl */

#include "nbt.h"
#include "nbt-private.h"

static const int kEncodeIncrement = 1024;


int mc_nbt_encode(mc_nbt_value_t* val,
                  mc_nbt_comp_t comp,
                  unsigned char** out) {
  return -1;
}
