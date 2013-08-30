#include <stdio.h>  /* fprintf */
#include <string.h>  /* memset */

#include "openssl/crypto.h"
#include "server.h"
#include "nbt.h"

int main() {
  int r;
  mc_server_t server;
  mc_config_t config;

  /* Start server */
  memset(&config, 0, sizeof(config));
  config.port = 25565;
  config.max_connections = 1000;

  r = mc_server_init(&server, &config);
  if (r != 0) {
    fprintf(stderr, "Failed to start mine.uv: %d\n", r);
    return -1;
  }

  fprintf(stdout, "Mine.uv running\n");
  mc_server_run(&server);

  fprintf(stdout, "Exiting...\n");
  mc_server_destroy(&server);

  return 0;
}
