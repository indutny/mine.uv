#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_

#include <stdint.h>  /* uint8_t */
#include <sys/types.h>  /* ssize_t */

typedef enum mc_frame_type_e mc_frame_type_t;
typedef enum mc_client_status_e mc_client_status_t;
typedef enum mc_digging_status_e mc_digging_status_t;
typedef enum mc_face_e mc_face_t;
typedef enum mc_animation_e mc_animation_t;
typedef enum mc_entity_action_e mc_entity_action_t;
typedef struct mc_frame_s mc_frame_t;
typedef struct mc_string_s mc_string_t;
typedef struct mc_slot_s mc_slot_t;

enum mc_frame_type_e {
  /* Sent by both */
  kMCKeepAliveType = 0x00,
  kMCLoginReqType = 0x01,
  kMCChatMsgType = 0x03,
  kMCPosAndLookType = 0x0D,
  kMCDiggingType = 0x0E,
  kMCBlockPlacementType = 0x0F,
  kMCHeldItemChangeType = 0x10,
  kMCAnimationType = 0x12,
  kMCCloseWindowType = 0x65,
  kMCConfirmTransactionType = 0x6A,
  kMCCreativeInvActionType = 0x6B,
  kMCUpdateSignType = 0x82,
  kMCPlayerAbilitiesType = 0xCA,
  kMCTabCompleteType = 0xCB,
  kMCPluginMsgType = 0xFA,
  kMCEncryptionResType = 0xFC,
  kMCKickType = 0xFF,

  /* Sent by client */
  kMCHandshakeType = 0x02,
  kMCUseEntityType = 0x07,
  kMCPlayerType = 0x0A,
  kMCPlayerPosType = 0x0B,
  kMCPlayerLookType= 0x0C,
  kMCEntityActionType = 0x13,
  kMCSteerVehicleType = 0x1B,
  kMCClickWindowType = 0x66,
  kMCEnchantItemType = 0x6C,
  kMCClientSettingsType = 0xCC,
  kMCClientStatusType = 0xCD,
  kMCServerListPingType = 0xFE,

  /* Sent by server */
  kMCTimeUpdateType = 0x04,
  kMCEntityEquipmentType = 0x05,
  kMCSpawnPositionType = 0x06,
  kMCUpdateHealthType = 0x08,
  kMCRespawnType = 0x09,
  kMCUseBedType = 0x11,
  kMCSpawnNamedEntityType = 0x14,
  kMCCollectItemType = 0x16,
  kMCSpawnObjectType = 0x17,
  kMCSpawnMobType = 0x18,
  kMCSpawnPaintingType = 0x19,
  kMCSpawnExpOrbType = 0x1A,
  kMCEntityVelocityType = 0x1C,
  kMCDestroyEntityType = 0x1D,
  kMCEntityType = 0x1E,
  kMCEntityRelMoveType = 0x1F,
  kMCEntityLookType = 0x20,
  kMCEntityLookAndRelMoveType = 0x21,
  kMCEntityTeleportType = 0x22,
  kMCEntityHeadLookType = 0x23,
  kMCEntityStatusType = 0x26,
  kMCAttachEntityType = 0x27,
  kMCEntityMetadataType = 0x28,
  kMCEntityEffectType = 0x29,
  kMCRemoveEntityEffectType = 0x2A,
  kMCSetExpType = 0x2B,
  kMCEntityPropsType = 0x2C,
  kMCChunkDataType = 0x33,
  kMCMultiBlockChangeType = 0x34,
  kMCBlockChangeType = 0x35,
  kMCBlockActionType = 0x36,
  kMCBlockBreakAnimationType = 0x37,
  kMCMapChunkBulkType = 0x38,
  kMCExplosionType = 0x3C,
  kMCSoundType = 0x3D,
  kMCNamedSoundType = 0x3E,
  kMCParticleType = 0x3F,
  kMCChangeGameType = 0x46,
  kMCSpawnGlobalEntityType = 0x47,
  kMCOpenWindowType = 0x64,
  kMCSetSlotType = 0x67,
  kMCSetWindowItemsType = 0x68,
  kMCUpdateWindowType = 0x69,
  kMCItemDataType = 0x83,
  kMCUpdateTileEntityType = 0x84,
  kMCTileEditorOpenType = 0x85,
  kMCIncrementStatType = 0xC8,
  kMCPlayerListItemType = 0xC9,
  kMCDisplayScoreType = 0xD0,
  kMCTeamType = 0xD1,
  kMCEncryptionReqType = 0xFD
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

enum mc_animation_e {
  kMCNoAnimation = 0,
  kMCSwingArmAnimation = 1
};

enum mc_entity_action_e {
  kMCCrounchAction = 1,
  kMCUncrounchAction = 2,
  kMCLeaveBedAction = 3,
  kMCStartSprintingAction = 4,
  kMCStopSprintingAction = 5
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
  unsigned char* nbt;
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
      uint16_t slot_id;
    } held_item_change;

    struct {
      uint32_t entity_id;
      mc_animation_t kind;
    } animation;

    struct {
      uint32_t entity_id;
      mc_entity_action_t action;
      uint32_t boost;
    } entity_action;

    struct {
      float sideways;
      float forward;
      uint8_t jump;
      uint8_t unmount;
    } steer_vehicle;

    int8_t close_window;

    struct {
      int8_t window;
      uint16_t slot;
      uint8_t button;
      uint16_t action_number;
      uint8_t mode;
      mc_slot_t clicked_item;
    } click_window;

    struct {
      int8_t window;
      uint16_t action_id;
      uint8_t accepted;
    } confirm_transaction;

    struct {
      int8_t slot;
      mc_slot_t clicked_slot;
    } creative_action;

    struct {
      int8_t window;
      int8_t enchantment;
    } enchant_item;

    struct {
      int32_t x;
      int16_t y;
      int32_t z;
      mc_string_t lines[4];
    } update_sign;

    struct {
      uint8_t flags;
      float flying_speed;
      float walking_speed;
    } player_abilities;

    mc_string_t tab_complete;

    int8_t server_list_ping;

    mc_string_t kick;

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
