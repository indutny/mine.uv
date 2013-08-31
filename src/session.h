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
  uv_getaddrinfo_t dns;
  int close_await;

  struct mc_client_s* client;

  /* Parametrized url */
  char* url;

  /* Completion callback */
  mc_session_verify_cb cb;
};

enum mc_session_verify_status_e {
  kMCVerifyOk,
  kMCVerifyErrNoMem,
  kMCVerifyErrTimeout,
  kMCVerifyErrConnect,
  kMCVerifyErrReject
};

mc_session_verify_t* mc_session_verify_new(struct mc_client_s* client);
void mc_session_verify(mc_session_verify_t* verify, mc_session_verify_cb cb);
void mc_session_verify_destroy(mc_session_verify_t* verify);

#endif  /* SRC_SESSION_H_ */
