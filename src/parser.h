#ifndef SRC_PARSER_H_
#define SRC_PARSER_H_

#include <stdint.h>  /* uint8_t */
#include <sys/types.h>  /* ssize_t */

typedef enum mc_frame_type_e mc_frame_type_t;
typedef struct mc_frame_string_s mc_frame_string_t;
typedef struct mc_frame_s mc_frame_t;
typedef struct mc_parser_s mc_parser_t;

enum mc_frame_type_e {
  MC_KEEPALIVE = 0x00,
  MC_LOGIN_REQUEST = 0x01,
  MC_HANDSHAKE = 0x02,
  MC_ENCRYPTION_RES = 0xFC,
  MC_ENCRYPTION_REQ = 0xFD
};

struct mc_frame_string_s {
  uint8_t* data;
  int len;
};

struct mc_frame_s {
  mc_frame_type_t type;
  union {
    struct {
      uint8_t version;
      mc_frame_string_t username;
      mc_frame_string_t host;
      uint32_t port;
    } handshake;
  } body;
};

struct mc_parser_s {
  uint8_t* data;
  ssize_t offset;
  ssize_t len;
};

int mc_parser_execute(uint8_t* data, ssize_t len, mc_frame_t* frame);

#endif  /* SRC_PARSER_H_ */
