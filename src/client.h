#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

#include <stdint.h>  /* uint8_t, uint16_t */

#include "uv.h"
#include "common.h"  /* mc_string_t */
#include "framer.h"  /* mc_farmer_t */
#include "openssl/evp.h"  /* EVP_CIPHER_CTX, EVP_MAX_MD_SIZE */
#include "server.h"

#define MC_MAX_ENC_BUF_SIZE 2048
#define MC_MAX_CLEAR_BUF_SIZE (MC_MAX_ENC_BUF_SIZE + 512)

typedef struct mc_client_s mc_client_t;
typedef struct mc_client__enc_buf_s mc_client__enc_buf_t;
typedef struct mc_client__clear_buf_s mc_client__clear_buf_t;
typedef enum mc_client__state_e mc_client__state_t;

enum mc_client__state_e {
  kMCInitialState,
  kMCInHandshakeState,
  kMCLoginState,
  kMCReadyState
};

struct mc_client__enc_buf_s {
  uint8_t data[MC_MAX_ENC_BUF_SIZE];
  ssize_t len;
};

struct mc_client__clear_buf_s {
  uint8_t data[MC_MAX_CLEAR_BUF_SIZE];
  ssize_t len;
};

struct mc_client_s {
  mc_server_t* server;
  uv_tcp_t tcp;
  mc_framer_t framer;

  mc_client__state_t state;
  mc_client__enc_buf_t encrypted;
  mc_client__clear_buf_t cleartext;

  /* User identification */
  mc_string_t username;
  char* ascii_username;
  char api_hash[EVP_MAX_MD_SIZE * 2 + 2];

  /* Verify token */
  unsigned char verify_token[20];

  /* Shared secret and encryption stuff */
  unsigned char* secret;
  int secret_len;
  EVP_CIPHER_CTX aes_in;
  EVP_CIPHER_CTX aes_out;
};

int mc_client_init(mc_server_t* server, mc_client_t* client);
void mc_client_destroy(mc_client_t* client);

#undef MC_MAX_BUF_SIZE

#endif  /* SRC_CLIENT_H_ */
