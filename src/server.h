#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

#include "openssl/rsa.h"  /* RSA */
#include "uv.h"

typedef struct mc_config_s mc_config_t;
typedef struct mc_server_s mc_server_t;

struct mc_config_s {
  int port;
  int max_connections;
};

struct mc_server_s {
  uv_loop_t* loop;
  uv_tcp_t tcp;

  int version;
  int connections;
  int max_connections;

  /* RSA key */
  RSA* rsa;
  unsigned char* rsa_pub_asn1;
  int rsa_pub_asn1_len;

  /* Server id */
  uint16_t server_id[16];
  unsigned char ascii_server_id[16];
};

int mc_server_init(mc_server_t* server, mc_config_t* config);
void mc_server_run(mc_server_t* server);
void mc_server_destroy(mc_server_t* server);

#endif  /* SRC_SERVER_H_ */
