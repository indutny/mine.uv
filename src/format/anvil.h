#ifndef SRC_FORMAT_ANVIL_H_
#define SRC_FORMAT_ANVIL_H_

#include "utils/common.h"

int mc_anvil_parse(const unsigned char* data, int len, mc_region_t** out);
int mc_anvil_encode(mc_region_t* reg, unsigned char** out);

#endif  /* SRC_FORMAT_ANVIL_H_ */
