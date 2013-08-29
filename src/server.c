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

  server->loop = uv_loop_new();
  if (server->loop == NULL)
    return 1;

  r = mc_server__generate_rsa(server);
  if (r != 0)
    goto failed_generate_key;

  r = mc_server__generate_id(server);
  if (r != 0)
    goto fatal;

  /* Initialize and start TCP server */
  r = uv_tcp_init(server->loop, &server->tcp);
  if (r != 0)
    goto fatal;

  memset(&addr, 0, sizeof(addr));
  addr.sin_len = sizeof(addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(config->port);
  addr.sin_addr.s_addr = INADDR_ANY;

  r = uv_tcp_bind(&server->tcp, addr);
  if (r != 0)
    goto fatal;

  r = uv_listen((uv_stream_t*) &server->tcp, 256, mc_server__on_connection);
  if (r != 0)
    goto fatal;

  server->version = 74;  /* 1.6.2 */
  server->tcp.data = server;

  return 0;

fatal:
  free(server->rsa_pub_asn1);
  RSA_free(server->rsa);
  server->rsa_pub_asn1 = NULL;
  server->rsa = NULL;

failed_generate_key:
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
  size_t i;

  /* Generate binary server id */
  r = RAND_bytes((unsigned char*) server->server_id, sizeof(server->server_id));
  if (r != 1)
    return -1;

  /* Translate it into readable character set: U+0021 - U+007E */
  for (i = 0; i < ARRAY_SIZE(server->server_id); i++)
    server->server_id[i] = htons(0x0021 + (server->server_id[i] % 0x005f));

  return 0;
}


void mc_server_run(mc_server_t* server) {
  uv_run(server->loop, UV_RUN_DEFAULT);
}


void mc_server__on_close(uv_handle_t* handle) {
  uv_loop_delete(handle->loop);
}


void mc_server_destroy(mc_server_t* server) {
  uv_close((uv_handle_t*) &server->tcp, mc_server__on_close);
  server->loop = NULL;
  RSA_free(server->rsa);
  server->rsa = NULL;
  free(server->rsa_pub_asn1);
  server->rsa_pub_asn1 = NULL;
}


void mc_server__on_connection(uv_stream_t* stream, int status) {
  int r;
  mc_server_t* server;
  mc_client_t* client;

  server = stream->data;

  client = malloc(sizeof(*client));
  if (client == NULL)
    return;

  r = uv_tcp_init(server->loop, &client->tcp);
  if (r != 0)
    goto fatal;

  /* Do not use Nagle algorithm */
  r = uv_tcp_nodelay(&client->tcp, 1);
  if (r != 0)
    goto fatal;

  r = uv_accept(stream, (uv_stream_t*) &client->tcp);
  if (r != 0)
    goto fatal;

  r = mc_client_init(server, client);
  if (r != 0)
    goto client_init_failed;

  return;

client_init_failed:
  mc_client_destroy(client);
  return;

fatal:
  free(client);
  return;
}
