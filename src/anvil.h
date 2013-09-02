#ifndef SRC_ANVIL_H_
#define SRC_ANVIL_H_

#include "nbt.h"

int mc_anvil_parse(const unsigned char* data, int len, mc_nbt_value_t** out);

#endif  /* SRC_ANVIL_H_ */
