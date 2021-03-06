#ifndef SRC_PROTOCOL_PARSER_H_
#define SRC_PROTOCOL_PARSER_H_

#include "utils/common.h"  /* mc_frame_t and friends */

int mc_parser_execute(uint8_t* data, int len, mc_frame_t* frame);

#endif  /* SRC_PROTOCOL_PARSER_H_ */
