#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_

#include <stdint.h>  /* uint8_t */
#include <sys/types.h>  /* ssize_t */

typedef enum mc_frame_type_e mc_frame_type_t;
typedef enum mc_client_status_e mc_client_status_t;
typedef struct mc_frame_s mc_frame_t;
typedef struct mc_string_s mc_string_t;

enum mc_frame_type_e {
  kMCKeepAliveType = 0x00,
  kMCLoginRequestType = 0x01,
  kMCHandshakeType = 0x02,
  kMCClientStatus = 0xCD,
  kMCEncryptionResType = 0xFC,
  kMCEncryptionReqType = 0xFD
};

enum mc_client_status_e {
  kMCInitialSpawnStatus,
  kMCRespawnStatus
};

struct mc_string_s {
  const uint16_t* data;
  uint16_t len;
  int allocated;
};

struct mc_frame_s {
  mc_frame_type_t type;
  union {
    uint16_t keepalive;
    mc_client_status_t client_status;
    struct {
      uint8_t version;
      mc_string_t username;
      mc_string_t host;
      uint32_t port;
    } handshake;
    struct {
      uint16_t secret_len;
      unsigned char* secret;
      uint16_t token_len;
      unsigned char* token;
    } enc_resp;
  } body;
};

void mc_string_init(mc_string_t* str);
void mc_string_destroy(mc_string_t* str);

void mc_string_set(mc_string_t* str, const uint16_t* data, int len);
int mc_string_copy(mc_string_t* to, mc_string_t* from);
char* mc_string_to_ascii(mc_string_t* str);

#endif  /* SRC_COMMON_H_ */
