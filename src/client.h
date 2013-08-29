#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

#include <stdint.h>  /* uint8_t, uint16_t */

#include "uv.h"
#include "common.h"  /* mc_string_t */
#include "framer.h"  /* mc_farmer_t */
#include "server.h"

#define MC_MAX_BUF_SIZE 4096

typedef struct mc_client_s mc_client_t;
typedef struct mc_client__buf_s mc_client__buf_t;
typedef enum mc_client__state_e mc_client__state_t;

enum mc_client__state_e {
  kMCInitialState,
  kMCInHandshakeState,
  kMCLoginState,
  kMCReadyState
};

struct mc_client__buf_s {
  uint8_t data[MC_MAX_BUF_SIZE];
  ssize_t len;
};

struct mc_client_s {
  mc_server_t* server;
  uv_tcp_t tcp;
  mc_framer_t framer;

  mc_client__state_t state;
  mc_client__buf_t encrypted;
  mc_client__buf_t cleartext;

  mc_string_t username;
  unsigned char verify_token[20];
};

int mc_client_init(mc_server_t* server, mc_client_t* client);
void mc_client_destroy(mc_client_t* client);

#endif  /* SRC_CLIENT_H_ */
