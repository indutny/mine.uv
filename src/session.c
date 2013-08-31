#include <assert.h>  /* assert */
#include <stdio.h>  /* snprintf */
#include <string.h>  /* strstr, strlen, memmove */

#include "uv.h"
#include "session.h"
#include "client.h"  /* mc_client_t */

static void mc_session_verify__on_close(uv_handle_t* handle);
static void mc_session_verify__parametrize(char* url,
                                           const char* match,
                                           const char* value,
                                           int value_len);


mc_session_verify_t* mc_session_verify_new(struct mc_client_s* client) {
  int r;
  mc_session_verify_t* verify;

  verify = malloc(sizeof(*verify));
  if (verify == NULL)
    goto malloc_failed;

  /* Number of handles waiting for close event */
  verify->close_await = 0;

  verify->client = client;
  verify->url = NULL;
  verify->cb = NULL;

  r = uv_tcp_init(client->server->loop, &verify->tcp);
  if (r != 0)
    goto tcp_init_failed;
  verify->tcp.data = verify;

  r = uv_timer_init(client->server->loop, &verify->timer);
  if (r != 0)
    goto timer_init_failed;
  verify->timer.data = verify;

  return verify;

timer_init_failed:
  verify->close_await++;
  uv_close((uv_handle_t*) &verify->tcp, mc_session_verify__on_close);

tcp_init_failed:
  free(verify);

malloc_failed:
  return verify;
}


void mc_session_verify_destroy(mc_session_verify_t* verify) {
  free(verify->url);
  verify->url = NULL;
  verify->client = NULL;

  /* TODO(indutny): destroy dns query, if its active */
  verify->close_await += 2;
  uv_close((uv_handle_t*) &verify->timer, mc_session_verify__on_close);
  uv_close((uv_handle_t*) &verify->tcp, mc_session_verify__on_close);
}


void mc_session_verify(mc_session_verify_t* verify, mc_session_verify_cb cb) {
  int session_url_len;
  int url_len;
  char* url;

  assert(verify->cb == NULL);

  /* Allocate enough space for new string */
  session_url_len = strlen(verify->client->server->config.session_url);
  url_len = session_url_len +
            verify->client->ascii_username_len +
            verify->client->api_hash_len;
  url = malloc(url_len + 1);
  if (url == NULL)
    return cb(verify->client, kMCVerifyErrNoMem);

  /* Copy template from server's config */
  memcpy(url, verify->client->server->config.session_url, session_url_len);

  /* Parametrize */
  mc_session_verify__parametrize(url,
                                 "%uid%",
                                 verify->client->ascii_username,
                                 verify->client->ascii_username_len);
  mc_session_verify__parametrize(url,
                                 "%sid%",
                                 verify->client->api_hash,
                                 verify->client->api_hash_len);
  verify->url = url;

  /* TODO(indutny) send request */
}


void mc_session_verify__on_close(uv_handle_t* handle) {
  mc_session_verify_t* verify;

  verify = handle->data;
  if (--verify->close_await == 0)
    free(verify);
}


void mc_session_verify__parametrize(char* url,
                                    const char* match,
                                    const char* value,
                                    int value_len) {
  char* res;
  int match_len;

  res = strstr(url, match);
  assert(res != NULL);

  match_len = strlen(match);

  /* Shift tail of string forward */
  memmove(res + value_len, res + match_len, strlen(res) - match_len);

  /* Insert value */
  /* TODO(indutny) escape value */
  memcpy(res, value, value_len);
}
