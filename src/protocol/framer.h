#ifndef SRC_PROTOCOL_FRAMER_H_
#define SRC_PROTOCOL_FRAMER_H_

#include <stdint.h>  /* uint8_t */

#include "uv.h"  /* uv_stream_t */
#include "utils/string.h"  /* mc_string_t */
#include "utils/encoder.h"  /* mc_encoder_t */
#include "openssl/evp.h"  /* EVP_CIPHER_CTX */

typedef struct mc_framer_s mc_framer_t;
typedef void (*mc_framer_send_cb_t)(mc_framer_t*, int status);

struct mc_framer_s {
  mc_encoder_t encoder;
  EVP_CIPHER_CTX* aes;
};

int mc_framer_init(mc_framer_t* framer);
void mc_framer_destroy(mc_framer_t* framer);

/* Start using encryption */
void mc_framer_use_aes(mc_framer_t* framer, EVP_CIPHER_CTX* aes);

/* Send all accumulated data */
int mc_framer_send(mc_framer_t* framer,
                   uv_stream_t* stream,
                   mc_framer_send_cb_t cb);

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
int mc_framer_kick(mc_framer_t* framer, mc_string_t* reason);

#endif  /* SRC_PROTOCOL_FRAMER_H_ */
