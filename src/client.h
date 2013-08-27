#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

#include <stdint.h>  /* uint8_t */

#include "uv.h"
#include "server.h"

#define MC_MAX_BUF_SIZE 4096

typedef struct mc_client_s mc_client_t;
typedef struct mc_client__buf_s mc_client__buf_t;

struct mc_client__buf_s {
  uint8_t data[MC_MAX_BUF_SIZE];
  ssize_t len;
};

struct mc_client_s {
  mc_server_t* server;
  uv_tcp_t tcp;
  mc_client__buf_t encrypted;
  mc_client__buf_t cleartext;
  int is_encrypted;
};

int mc_client_init(mc_server_t* server, mc_client_t* client);
void mc_client_destroy(mc_client_t* client);

#endif  /* SRC_CLIENT_H_ */
