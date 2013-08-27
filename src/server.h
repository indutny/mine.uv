#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

#include "openssl/rsa.h"  /* RSA */
#include "uv.h"

typedef struct mc_config_s mc_config_t;
typedef struct mc_server_s mc_server_t;

struct mc_config_s {
  int port;
};

struct mc_server_s {
  uv_loop_t* loop;
  uv_tcp_t tcp;
  RSA* rsa;
  char rsa_asn1[4096];
  int rsa_asn1_len;
};

int mc_server_init(mc_server_t* server, mc_config_t* config);
void mc_server_run(mc_server_t* server);
void mc_server_destroy(mc_server_t* server);

#endif  /* SRC_SERVER_H_ */
