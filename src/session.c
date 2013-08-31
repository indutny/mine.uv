#include <arpa/inet.h>  /* ntohs */
#include <assert.h>  /* assert */
#include <stdio.h>  /* snprintf */
#include <string.h>  /* strstr, strlen, memmove */

#include "uv.h"
#include "common-private.h"  /* container_of */
#include "session.h"
#include "client.h"  /* mc_client_t */

static void mc_session_verify__on_close(uv_handle_t* handle);
static void mc_session_verify__on_getaddrinfo(uv_getaddrinfo_t* req,
                                              int status,
                                              struct addrinfo* res);
static void mc_session_verify__on_connect(uv_connect_t* req, int status);
static void mc_session_verify__after_write(uv_write_t* req, int status);
static uv_buf_t mc_session_verify__on_alloc(uv_handle_t* handle, size_t size);
static void mc_session_verify__on_read(uv_stream_t* stream,
                                       ssize_t nread,
                                       uv_buf_t buf);
static void mc_session_verify__on_timeout(uv_timer_t* timer, int status);
static void mc_session_verify__cancel(mc_session_verify_t* verify);
static void mc_session_verify__parametrize(char* url,
                                           const char* match,
                                           const char* value,
                                           int value_len);

#define INVOKE_CB_ONCE(verify, status) \
    do { \
      mc_session_verify__cancel(verify); \
      (verify)->cb((verify)->client, (status)); \
      (verify)->cb = NULL; \
    } while (0)

static const char* request_template = "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n";

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
  verify->dns_active = 0;
  verify->connect_active = 0;
  verify->write_active = 0;
  verify->crlf = 0;

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
  assert(verify->client != NULL);
  verify->client = NULL;
  free(verify->url);
  verify->url = NULL;

  /* NOTE: we're heavily relying on order of callback execution here */
  mc_session_verify__cancel(verify);

  verify->close_await += 2;
  uv_close((uv_handle_t*) &verify->timer, mc_session_verify__on_close);
  uv_close((uv_handle_t*) &verify->tcp, mc_session_verify__on_close);
}


void mc_session_verify(mc_session_verify_t* verify, mc_session_verify_cb cb) {
  int r;
  int session_url_len;
  int url_len;
  char* url;
  const char* uri;

  assert(verify->cb == NULL && cb != NULL);

  /* Allocate enough space for new string */
  session_url_len = strlen(verify->client->server->config.session_url);
  url_len = session_url_len +
            /* Reserve 3x space for username, as it might need to be escaped */
            3 * verify->client->ascii_username_len +
            /* API hash contains only [a-z0-9\-]* characters */
            verify->client->api_hash_len;
  url = malloc(url_len + 1);
  if (url == NULL)
    return cb(verify->client, kMCVerifyErrNoMem);

  /* Copy template from server's config */
  memcpy(url, verify->client->server->config.session_url, session_url_len + 1);

  /* Parametrize */
  mc_session_verify__parametrize(url,
                                 "%uid%",
                                 verify->client->ascii_username,
                                 verify->client->ascii_username_len);
  mc_session_verify__parametrize(url,
                                 "%sid%",
                                 verify->client->api_hash,
                                 verify->client->api_hash_len);
  /* Split url into hostname and uri */
  uri = strstr(url, "/");
  assert(uri != NULL && (size_t) (uri - url) < sizeof(verify->hostname));
  memcpy(verify->hostname, url, uri - url);
  verify->hostname[uri - url] = 0;

  verify->url = url;
  verify->cb = cb;

  /* Perform DNS query */
  r = uv_getaddrinfo(verify->client->server->loop,
                     &verify->dns_req,
                     mc_session_verify__on_getaddrinfo,
                     verify->hostname,
                     "http",
                     NULL);
  if (r != 0)
    return cb(verify->client, kMCVerifyErrDNS);
  verify->dns_active = 1;
}


void mc_session_verify__on_close(uv_handle_t* handle) {
  mc_session_verify_t* verify;

  verify = handle->data;
  handle->data = NULL;

  if (--verify->close_await == 0)
    free(verify);
}


void mc_session_verify__on_getaddrinfo(uv_getaddrinfo_t* req,
                                       int status,
                                       struct addrinfo* res) {
  mc_session_verify_t* verify;
  int r;
  struct addrinfo* i;
  struct sockaddr_in* addr;

  if (status == UV_ECANCELED) {
    uv_freeaddrinfo(res);
    return;
  }

  verify = container_of(req, mc_session_verify_t, dns_req);
  verify->dns_active = 0;

  if (status != 0) {
    uv_freeaddrinfo(res);
    INVOKE_CB_ONCE(verify, kMCVerifyErrDNS);
    return;
  }

  /* Pick first IPv4 address and initiate tcp connection */
  for (i = res; i != NULL; i = i->ai_next) {
    if (i->ai_family == AF_INET)
      break;
  }
  if (i == NULL) {
    uv_freeaddrinfo(res);
    INVOKE_CB_ONCE(verify, kMCVerifyErrNoIPv4);
    return;
  }

  /* Connect to it */
  addr = (struct sockaddr_in*) i->ai_addr;
  addr->sin_port = ntohs(80);
  r = uv_tcp_connect(&verify->connect_req,
                     &verify->tcp,
                     *addr,
                     mc_session_verify__on_connect);

  /* Free addrinfo results */
  uv_freeaddrinfo(res);
  if (r != 0) {
    INVOKE_CB_ONCE(verify, kMCVerifyErrConnect);
    return;
  }
  verify->connect_active = 1;
}


void mc_session_verify__on_connect(uv_connect_t* req, int status) {
  int r;
  int len;
  mc_session_verify_t* verify;
  char* query;
  char* uri;
  uv_buf_t buf;

  if (status == UV_ECANCELED)
    return;

  verify = container_of(req, mc_session_verify_t, connect_req);
  verify->connect_active = 0;

  if (status != 0) {
    INVOKE_CB_ONCE(verify, kMCVerifyErrConnect);
    return;
  }

  /* Parametrize template */
  len = strlen(verify->url) + sizeof(request_template);
  query = malloc(len + 1);
  if (query == NULL) {
    INVOKE_CB_ONCE(verify, kMCVerifyErrNoMem);
    return;
  }

  uri = strstr(verify->url, "/");
  snprintf(query, len, request_template, uri, verify->hostname);
  buf = uv_buf_init(query, strlen(query));

  /* Store it inside structure to free it later */
  verify->write_req.data = query;

  /* Send write request */
  r = uv_write(&verify->write_req,
               (uv_stream_t*) &verify->tcp,
               &buf,
               1,
               mc_session_verify__after_write);
  if (r != 0) {
    INVOKE_CB_ONCE(verify, kMCVerifyErrWrite);
    return;
  }
  verify->write_active = 1;

  /* And start reading response */
  r = uv_read_start((uv_stream_t*) &verify->tcp,
                    mc_session_verify__on_alloc,
                    mc_session_verify__on_read);
  if (r != 0) {
    INVOKE_CB_ONCE(verify, kMCVerifyErrWrite);
    return;
  }

  if (verify->client->server->config.verify_timeout != 0) {
    /* Start timeout timer */
    r = uv_timer_start(&verify->timer,
                       mc_session_verify__on_timeout,
                       verify->client->server->config.verify_timeout,
                       0);
    if (r != 0) {
      /* Ignore result */
      INVOKE_CB_ONCE(verify, kMCVerifyErrWrite);
      return;
    }
  }
}


void mc_session_verify__after_write(uv_write_t* req, int status) {
  mc_session_verify_t* verify;

  verify = container_of(req, mc_session_verify_t, write_req);
  verify->write_active = 0;

  /* Just free the data */
  free(req->data);
}


uv_buf_t mc_session_verify__on_alloc(uv_handle_t* handle, size_t size) {
  mc_session_verify_t* verify;

  /* Fixed-size buffer operation mode */
  verify = container_of(handle, mc_session_verify_t, tcp);
  return uv_buf_init(verify->buf, sizeof(verify->buf));
}


void mc_session_verify__on_read(uv_stream_t* stream,
                                ssize_t nread,
                                uv_buf_t buf) {
  size_t i;
  mc_session_verify_t* verify;

  verify = container_of(stream, mc_session_verify_t, tcp);
  if (nread < 0) {
    /* Ignore result */
    INVOKE_CB_ONCE(verify, kMCVerifyErrRead);
    return;
  }

  for (i = 0; i < buf.len; i++) {
    if (verify->crlf == 4) {
      INVOKE_CB_ONCE(verify,
                     buf.base[i] == 'Y' ? kMCVerifyOk : kMCVerifyRejected);
      return;
    }

    if (buf.base[i] != ((verify->crlf % 2) ? '\n' : '\r'))
      verify->crlf = 0;
    else
      verify->crlf++;
  }
}


void mc_session_verify__on_timeout(uv_timer_t* timer, int status) {
  mc_session_verify_t* verify;

  if (status == UV_ECANCELED)
    return;
  verify = container_of(timer, mc_session_verify_t, timer);

  /* Ignore result */
  INVOKE_CB_ONCE(verify, kMCVerifyErrTimeout);
}


void mc_session_verify__cancel(mc_session_verify_t* verify) {
  int r;
  if (verify->dns_active) {
    r = uv_cancel((uv_req_t*) &verify->dns_req);
    assert(r == 0);
  }
  if (verify->connect_active) {
    r = uv_cancel((uv_req_t*) &verify->connect_req);
    assert(r == 0);
  }
  if (verify->write_active) {
    r = uv_cancel((uv_req_t*) &verify->write_req);
    assert(r == 0);
  }
  uv_read_stop((uv_stream_t*) &verify->tcp);
  uv_timer_stop(&verify->timer);

  verify->dns_active = 0;
  verify->connect_active = 0;
  verify->write_active = 0;
}


void mc_session_verify__parametrize(char* url,
                                    const char* match,
                                    const char* value,
                                    int value_len) {
  char* res;
  int i;
  unsigned char c;
  int match_len;
  int escaped_len;

  res = strstr(url, match);
  assert(res != NULL);

  match_len = strlen(match);

  /* Calculate escaped value's length */
  escaped_len = value_len;
  for (i = 0; i < value_len; i++) {
    c = value[i];

    /* Unreserved characters */
    if ((c >= 0x30 /* 0 */ && c <= 0x39 /* 9 */) ||
        (c >= 0x41 /* A */ && c <= 0x5a /* Z */) ||
        (c >= 0x61 /* a */ && c <= 0x7a /* z */) ||
        (c == '-' || c == '_' || c == '.' || c == '~')) {
      continue;
    }

    escaped_len += 2;
  }

  /* Shift tail of string forward */
  memmove(res + escaped_len, res + match_len, strlen(res) - match_len + 1);

  /* Insert value */
  for (i = 0; i < value_len; i++) {
    c = value[i];

    /* Unreserved characters */
    if ((c >= 0x30 /* 0 */ && c <= 0x39 /* 9 */) ||
        (c >= 0x41 /* A */ && c <= 0x5a /* Z */) ||
        (c >= 0x61 /* a */ && c <= 0x7a /* z */) ||
        (c == '-' || c == '_' || c == '.' || c == '~')) {
      /* Insert non-escaped char */
      res[0] = c;
      res++;
      continue;
    }

    /* Insert %dd hex representation */
    res[0] = '%';
    if ((c & 0x80) == 0)
      res[1] = '0' + ((c >> 4) & 0x7);
    else
      res[1] = 'A' + ((c >> 4) & 0x7);
    if ((c & 0x8) == 0)
      res[2] = '0' + (c & 0x7);
    else
      res[2] = 'A' + (c & 0x7);
    res += 3;
  }
}
