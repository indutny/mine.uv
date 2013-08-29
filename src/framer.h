#ifndef SRC_FRAMER_H_
#define SRC_FRAMER_H_

#include "uv.h"  /* uv_stream_t */

#include <stdint.h>  /* uint8_t */
#include "common.h"  /* mc_string_t */

typedef struct mc_framer_s mc_framer_t;

struct mc_framer_s {
  unsigned char* data;
  ssize_t offset;
  ssize_t len;
};

int mc_framer_init(mc_framer_t* framer);
void mc_framer_destroy(mc_framer_t* framer);

/* Send all accumulated data */
int mc_framer_send(mc_framer_t* framer, uv_stream_t* stream);

/* Generate various frames */
int mc_framer_enc_key_req(mc_framer_t* framer,
                          mc_string_t* server_id,
                          const unsigned char* public_key,
                          uint16_t public_key_len,
                          const unsigned char* token,
                          uint16_t token_len);
int mc_framer_enc_key_res(mc_framer_t* framer,
                          const unsigned char* secret,
                          uint16_t secret_len,
                          const unsigned char* token,
                          uint16_t token_len);
int mc_framer_login_req(mc_framer_t* framer,
                        uint32_t entity_id,
                        mc_string_t* level_type,
                        uint8_t mode,
                        int8_t dimension,
                        uint8_t difficulty,
                        uint8_t max_players);

#endif  /* SRC_FRAMER_H_ */
