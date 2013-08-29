#ifndef SRC_PARSER_H_
#define SRC_PARSER_H_

#include "common.h"  /* mc_frame_t and friends */
#include <sys/types.h>  /* ssize_t */

typedef struct mc_parser_s mc_parser_t;

int mc_parser_execute(uint8_t* data, ssize_t len, mc_frame_t* frame);

#endif  /* SRC_PARSER_H_ */
