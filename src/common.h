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
  kMCLoginReqType = 0x01,
  kMCHandshakeType = 0x02,
  kMCPlayerPosType = 0x0B,
  kMCPlayerLookType= 0x0C,
  kMCPosAndLookType = 0x0D,
  kMCClientSettingsType = 0xCC,
  kMCClientStatusType = 0xCD,
  kMCPluginMsgType = 0xFA,
  kMCEncryptionResType = 0xFC,
  kMCEncryptionReqType = 0xFD,
  kMCKickType = 0xFF
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
    uint32_t keepalive;
    mc_client_status_t client_status;
    struct {
      uint32_t entity_id;
      mc_string_t level;
      uint8_t game_mode;
      int8_t dimension;
      uint8_t difficulty;
      uint8_t max_players;
    } login_req;
    struct {
      uint8_t version;
      mc_string_t username;
      mc_string_t host;
      uint32_t port;
    } handshake;
    struct {
      double x;
      double y;
      double stance;
      double z;
      float yaw;
      float pitch;
      uint8_t on_ground;
    } pos_and_look;
    struct {
      mc_string_t locale;
      uint8_t view_distance;
      uint8_t chat_flags;
      uint8_t difficulty;
      uint8_t show_cape;
    } settings;
    struct {
      uint16_t secret_len;
      unsigned char* secret;
      uint16_t token_len;
      unsigned char* token;
    } enc_resp;
    struct {
      mc_string_t channel;
      uint16_t msg_len;
      unsigned char* msg;
    } plugin_msg;
  } body;
};

void mc_string_init(mc_string_t* str);
void mc_string_destroy(mc_string_t* str);

void mc_string_set(mc_string_t* str, const uint16_t* data, int len);
int mc_string_copy(mc_string_t* to, mc_string_t* from);
char* mc_string_to_ascii(mc_string_t* str);
int mc_string_from_ascii(mc_string_t* to, const char* from);

#endif  /* SRC_COMMON_H_ */
