#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_

#include <stdint.h>  /* uint8_t */
#include <sys/types.h>  /* ssize_t */

typedef enum mc_frame_type_e mc_frame_type_t;
typedef enum mc_client_status_e mc_client_status_t;
typedef enum mc_digging_status_e mc_digging_status_t;
typedef enum mc_face_e mc_face_t;
typedef struct mc_frame_s mc_frame_t;
typedef struct mc_string_s mc_string_t;
typedef struct mc_slot_s mc_slot_t;

enum mc_frame_type_e {
  kMCKeepAliveType = 0x00,
  kMCLoginReqType = 0x01,
  kMCHandshakeType = 0x02,
  kMCChatMsgType = 0x03,
  kMCTimeUpdateType = 0x04,
  kMCEntityEquipmentType = 0x05,
  kMCSpawnPositionType = 0x06,
  kMCUseEntityType = 0x07,
  kMCUpdateHealthType = 0x08,
  kMCRespawnType = 0x09,
  kMCPlayerType = 0x0A,
  kMCPlayerPosType = 0x0B,
  kMCPlayerLookType= 0x0C,
  kMCPosAndLookType = 0x0D,
  kMCDiggingType = 0x0E,
  kMCBlockPlacementType = 0x0F,
  kMCClientSettingsType = 0xCC,
  kMCClientStatusType = 0xCD,
  kMCPluginMsgType = 0xFA,
  kMCEncryptionResType = 0xFC,
  kMCEncryptionReqType = 0xFD,
  kMCKickType = 0xFF
};

enum mc_client_status_e {
  kMCInitialSpawnStatus = 0,
  kMCRespawnStatus = 1
};

enum mc_digging_status_e {
  kMCStartedDigging = 0,
  kMCCancelledDigging = 1,
  kMCFinishedDigging = 2,
  kMCDropItemStack = 3,
  kMCDropItem = 4,
  kMCShootArrow = 5
};

enum mc_face_e {
  kMCFaceNY = 0,
  kMCFacePY = 1,
  kMCFaceNZ = 2,
  kMCFacePZ = 3,
  kMCFaceNX = 4,
  kMCFacePX = 5
};

struct mc_string_s {
  const uint16_t* data;
  uint16_t len;
  int allocated;
};

struct mc_slot_s {
  uint16_t id;
  uint16_t count;
  uint16_t damage;
  uint16_t nbt_len;
  const unsigned char* nbt;
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
    mc_string_t chat_msg;
    struct {
      uint32_t user;
      uint32_t target;
      uint8_t button;
    } use_entity;
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
      mc_digging_status_t status;
      int32_t x;
      int8_t y;
      int32_t z;
      mc_face_t face;
    } digging;
    struct {
      int32_t x;
      uint8_t y;
      int32_t z;
      int8_t direction;
      mc_slot_t held_item;
      int8_t cursor_x;
      int8_t cursor_y;
      int8_t cursor_z;
    } block_placement;
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

/* String utils */
void mc_string_init(mc_string_t* str);
void mc_string_destroy(mc_string_t* str);

void mc_string_set(mc_string_t* str, const uint16_t* data, int len);
int mc_string_copy(mc_string_t* to, mc_string_t* from);
char* mc_string_to_ascii(mc_string_t* str);
int mc_string_from_ascii(mc_string_t* to, const char* from);

/* Slot utils */
void mc_slot_init(mc_slot_t* slot);
void mc_slot_destroy(mc_slot_t* slot);

#endif  /* SRC_COMMON_H_ */
