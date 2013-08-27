#include <arpa/inet.h>  /* htons */
#include <assert.h>  /* assert */
#include <stdlib.h>  /* malloc, NULL */
#include <string.h>  /* memset */

#include "server.h"
#include "openssl/bio.h"  /* BIO, BIO_new, ... */
#include "openssl/pem.h"  /* PEM_write_bio_RSA_PUBKEY */
#include "openssl/rsa.h"  /* RSA_generate_key, RSA_free */
#include "uv.h"
#include "client.h"

static void mc_server__on_connection(uv_stream_t* stream, int status);
static void mc_server__on_close(uv_handle_t* handle);

int mc_server_init(mc_server_t* server, mc_config_t* config) {
  int r;
  struct sockaddr_in addr;
  BIO* bio;

  server->loop = uv_loop_new();
  if (server->loop == NULL)
    return 1;

  /* Generate public key */
  server->rsa = RSA_generate_key(1024, 65537, NULL, NULL);
  if (server->rsa == NULL)
    goto failed_generate_key;

  /* Cache ASN1 encoding of public key */
  bio = BIO_new(BIO_s_mem());
  if (bio == NULL)
    goto fatal;
  r = PEM_write_bio_RSA_PUBKEY(bio, server->rsa);
  if (r == 1) {
    r = BIO_read(bio, server->rsa_asn1, sizeof(server->rsa_asn1));
    r = (r == sizeof(server->rsa_asn1)) ? -1 : 0;
  } else {
    r = -1;
  }

  BIO_free_all(bio);
  bio = NULL;

  if (r != 0)
    goto fatal;

  /* Cache ASN1 encoding length */
  server->rsa_asn1_len = strlen(server->rsa_asn1);

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

  server->tcp.data = server;

  return 0;

fatal:
  RSA_free(server->rsa);
  server->rsa = NULL;

failed_generate_key:
  uv_loop_delete(server->loop);
  server->loop = NULL;
  return r;
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
