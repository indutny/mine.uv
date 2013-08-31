#include <arpa/inet.h>  /* htons */
#include <assert.h>  /* assert */
#include <stdlib.h>  /* malloc, NULL */
#include <string.h>  /* memset */

#include "server.h"
#include "client.h"
#include "common-private.h"  /* ARRAY_SIZE */
#include "openssl/bio.h"  /* BIO, BIO_new, ... */
#include "openssl/pem.h"  /* PEM_write_bio_RSA_PUBKEY */
#include "openssl/rand.h"  /* RAND_bytes */
#include "openssl/rsa.h"  /* RSA_generate_key, RSA_free */
#include "uv.h"

static int mc_server__generate_rsa(mc_server_t* server);
static int mc_server__generate_id(mc_server_t* server);
static void mc_server__on_connection(uv_stream_t* stream, int status);
static void mc_server__on_close(uv_handle_t* handle);

int mc_server_init(mc_server_t* server, mc_config_t* config) {
  int r;
  struct sockaddr_in addr;

  /* Initialize OpenSSL */
  OPENSSL_init();

  server->loop = uv_loop_new();
  if (server->loop == NULL)
    return 1;

  server->tcp = malloc(sizeof(*server->tcp));
  if (server->tcp == NULL) {
    r = 1;
    goto failed_alloc_tcp;
  }
  server->tcp->data = server;

  r = mc_server__generate_rsa(server);
  if (r != 0)
    goto failed_generate_key;

  r = mc_server__generate_id(server);
  if (r != 0)
    goto fatal;

  /* Initialize and start TCP server */
  r = uv_tcp_init(server->loop, server->tcp);
  if (r != 0)
    goto fatal;

  memset(&addr, 0, sizeof(addr));
  addr.sin_len = sizeof(addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(config->port);
  addr.sin_addr.s_addr = INADDR_ANY;

  r = uv_tcp_bind(server->tcp, addr);
  if (r != 0)
    goto fatal;

  r = uv_listen((uv_stream_t*) server->tcp, 256, mc_server__on_connection);
  if (r != 0)
    goto fatal;

  server->version = 74;  /* 1.6.2 */
  server->clients = 0;

  /* Copy config and set defaults */
  memcpy(&server->config, config, sizeof(*config));
  if (server->config.port == 0)
    server->config.port = 25565;
  if (server->config.session_url == NULL) {
    server->config.session_url = "http://session.minecraft.net/"
                                 "game/checkserver.jsp?user=%s&serverId=%s";
  }

  return 0;

fatal:
  free(server->rsa_pub_asn1);
  RSA_free(server->rsa);
  server->rsa_pub_asn1 = NULL;
  server->rsa = NULL;

failed_generate_key:
  free(server->tcp);

failed_alloc_tcp:
  uv_loop_delete(server->loop);
  server->loop = NULL;
  return r;
}


int mc_server__generate_rsa(mc_server_t* server) {
  int r;

  server->rsa = RSA_generate_key(1024, 65537, NULL, NULL);
  if (server->rsa == NULL)
    return -1;

  /* Cache ASN1 encoding of public key */
  server->rsa_pub_asn1 = NULL;
  r = i2d_RSA_PUBKEY(server->rsa, &server->rsa_pub_asn1);
  if (r <= 0) {
    RSA_free(server->rsa);
    server->rsa = NULL;
    return -1;
  }

  /* Cache ASN1 encoding length */
  server->rsa_pub_asn1_len = r;

  return 0;
}


int mc_server__generate_id(mc_server_t* server) {
  int r;
  unsigned char code;
  size_t i;

  /* Generate binary server id */
  r = RAND_bytes((unsigned char*) server->server_id, sizeof(server->server_id));
  if (r != 1)
    return -1;

  /* Translate it into readable character set: U+0021 - U+007E */
  assert(ARRAY_SIZE(server->server_id) == ARRAY_SIZE(server->ascii_server_id));
  for (i = 0; i < ARRAY_SIZE(server->server_id); i++) {
    code = 0x0021 + (server->server_id[i] % 0x005f);
    server->server_id[i] = htons((uint16_t) code);
    server->ascii_server_id[i] = code;
  }

  return 0;
}


void mc_server_run(mc_server_t* server) {
  uv_run(server->loop, UV_RUN_DEFAULT);
}


void mc_server__on_close(uv_handle_t* handle) {
  uv_loop_delete(handle->loop);
}


void mc_server_destroy(mc_server_t* server) {
  uv_close((uv_handle_t*) server->tcp, mc_server__on_close);
  server->loop = NULL;
  RSA_free(server->rsa);
  server->rsa = NULL;
  free(server->rsa_pub_asn1);
  server->rsa_pub_asn1 = NULL;
}


void mc_server__on_connection(uv_stream_t* stream, int status) {
  mc_server_t* server;

  server = stream->data;

  if (status == 0)
    mc_client_new(server);
}
