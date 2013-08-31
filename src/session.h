#ifndef SRC_SESSION_H_
#define SRC_SESSION_H_

#include "uv.h"

/* Forward declarations */
struct mc_client_s;

typedef struct mc_session_verify_s mc_session_verify_t;
typedef enum mc_session_verify_status_e mc_session_verify_status_t;
typedef void (*mc_session_verify_cb)(struct mc_client_s* client,
                                     mc_session_verify_status_t status);

struct mc_session_verify_s {
  uv_tcp_t tcp;
  uv_timer_t timer;
  int dns_active;
  uv_getaddrinfo_t dns_req;
  int connect_active;
  uv_connect_t connect_req;
  int write_active;
  uv_write_t write_req;

  /* Number of bytes from CRLFCRLF sequence that was received */
  int crlf;

  /* Number of handles waiting for close event */
  int close_await;

  /* Number of asynchronous requests that we should await for */
  int busy_await;

  struct mc_client_s* client;

  /* Parametrized url */
  char hostname[256];
  char* url;

  /* Completion callback */
  mc_session_verify_cb cb;

  char buf[256];
};

enum mc_session_verify_status_e {
  kMCVerifyOk,
  kMCVerifyRejected,
  kMCVerifyErrNoMem,
  kMCVerifyErrDNS,
  kMCVerifyErrNoIPv4,
  kMCVerifyErrTimeout,
  kMCVerifyErrConnect,
  kMCVerifyErrWrite,
  kMCVerifyErrRead
};

mc_session_verify_t* mc_session_verify_new(struct mc_client_s* client);
void mc_session_verify(mc_session_verify_t* verify, mc_session_verify_cb cb);
void mc_session_verify_destroy(mc_session_verify_t* verify);

#endif  /* SRC_SESSION_H_ */
