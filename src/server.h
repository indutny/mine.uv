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

  int version;

  /* RSA key */
  RSA* rsa;
  char rsa_pub_asn1[4096];
  int rsa_pub_asn1_len;

  /* Server id */
  uint16_t server_id[20];
};

int mc_server_init(mc_server_t* server, mc_config_t* config);
void mc_server_run(mc_server_t* server);
void mc_server_destroy(mc_server_t* server);

#endif  /* SRC_SERVER_H_ */
