#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

#include <stdint.h>  /* uint8_t */

/* Forward declarations */
struct uv_loop_s;
struct uv_tcp_s;
struct rsa_st;

typedef struct mc_config_s mc_config_t;
typedef struct mc_server_s mc_server_t;

struct mc_config_s {
  int port;  /* TCP port on which it should listen */
  int max_clients;
  int client_timeout;  /* in milliseconds, activates if no packets has arrived
                        * in specified amount of time
                        */
};

struct mc_server_s {
  struct uv_loop_s* loop;
  struct uv_tcp_s* tcp;

  int version;
  int clients;
  mc_config_t config;

  /* RSA key */
  struct rsa_st* rsa;
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
