#include <arpa/inet.h>  /* ntohs, ntohl */
#include <assert.h>  /* assert */
#include <stdint.h>  /* uint8_t */
#include <stdlib.h>  /* abort */
#include <sys/types.h>  /* ssize_t */

#include "parser.h"

#define PARSE_READ(p, type, out) \
    do { \
      int r; \
      r = mc_parser__read_##type((p), (out)); \
      if (r <= 0) \
        return r; \
    } while (0)

#define PARSE_READ_RAW(p, out, size) \
    do { \
      int r; \
      r = mc_parser__read_raw((p), (out), (size)); \
      if (r <= 0) \
        return r; \
    } while (0)

typedef struct mc_parser_s mc_parser_t;

static int mc_parser__read_u8(mc_parser_t* p, uint8_t* out);
static int mc_parser__read_u16(mc_parser_t* p, uint16_t* out);
static int mc_parser__read_u32(mc_parser_t* p, uint32_t* out);
static int mc_parser__read_u64(mc_parser_t* p, uint64_t* out);
static int mc_parser__read_float(mc_parser_t* p, float* out);
static int mc_parser__read_double(mc_parser_t* p, double* out);
static int mc_parser__read_string(mc_parser_t* p, mc_string_t* str);
static int mc_parser__read_slot(mc_parser_t* p, mc_slot_t* slot);
static int mc_parser__read_raw(mc_parser_t* p,
                               unsigned char** out,
                               size_t size);

static int mc_parser__parse_login_req(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_handshake(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_use_entity(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_pos(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_look(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_pos_and_look(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_digging(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_block_placement(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_steer_vehicle(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_click_window(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_confirm_transaction(mc_parser_t* p,
                                                mc_frame_t* frame);
static int mc_parser__parse_update_sign(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_abilities(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_settings(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_enc_resp(mc_parser_t* p, mc_frame_t* frame);
static int mc_parser__parse_plugin_msg(mc_parser_t* p, mc_frame_t* frame);

struct mc_parser_s {
  const unsigned char* data;
  size_t offset;
  size_t len;
};


int mc_parser_execute(uint8_t* data, ssize_t len, mc_frame_t* frame) {
  uint8_t type;
  uint8_t tmp;
  mc_parser_t parser;

  parser.data = data;
  parser.offset = 0;
  parser.len = len;

  PARSE_READ(&parser, u8, &type);
  frame->type = (mc_frame_type_t) type;

  switch (frame->type) {
    case kMCKeepAliveType:
      PARSE_READ(&parser, u32, &frame->body.keepalive);
      break;
    case kMCLoginReqType:
      return mc_parser__parse_login_req(&parser, frame);
    case kMCHandshakeType:
      return mc_parser__parse_handshake(&parser, frame);
    case kMCChatMsgType:
      PARSE_READ(&parser, string, &frame->body.chat_msg);
      return parser.offset;
    case kMCUseEntityType:
      return mc_parser__parse_use_entity(&parser, frame);
    case kMCPlayerType:
      PARSE_READ(&parser, u8, &frame->body.pos_and_look.on_ground);
      frame->body.pos_and_look.x = 0;
      frame->body.pos_and_look.y = 0;
      frame->body.pos_and_look.z = 0;
      frame->body.pos_and_look.stance = 0;
      frame->body.pos_and_look.yaw = 0;
      frame->body.pos_and_look.pitch = 0;
      break;
    case kMCPlayerPosType:
      return mc_parser__parse_pos(&parser, frame);
    case kMCPlayerLookType:
      return mc_parser__parse_look(&parser, frame);
    case kMCPosAndLookType:
      return mc_parser__parse_pos_and_look(&parser, frame);
    case kMCDiggingType:
      return mc_parser__parse_digging(&parser, frame);
    case kMCBlockPlacementType:
      return mc_parser__parse_block_placement(&parser, frame);
    case kMCHeldItemChangeType:
      PARSE_READ(&parser, u16, &frame->body.held_item_change.slot_id);
      if (frame->body.held_item_change.slot_id > 8)
        return -1;
      break;
    case kMCAnimationType:
      PARSE_READ(&parser, u32, &frame->body.animation.entity_id);
      PARSE_READ(&parser, u8, &tmp);
      frame->body.animation.kind = (mc_animation_t) tmp;
      break;
    case kMCEntityActionType:
      PARSE_READ(&parser, u32, &frame->body.entity_action.entity_id);
      PARSE_READ(&parser, u8, &tmp);
      frame->body.entity_action.action = (mc_entity_action_t) tmp;
      PARSE_READ(&parser, u32, &frame->body.entity_action.boost);
      if (frame->body.entity_action.boost > 100)
        return -1;
      break;
    case kMCSteerVehicleType:
      return mc_parser__parse_steer_vehicle(&parser, frame);
    case kMCCloseWindowType:
      PARSE_READ(&parser, u8, (uint8_t*) &frame->body.close_window);
      break;
    case kMCClickWindowType:
      return mc_parser__parse_click_window(&parser, frame);
    case kMCConfirmTransactionType:
      return mc_parser__parse_confirm_transaction(&parser, frame);
    case kMCCreativeInvActionType:
      PARSE_READ(&parser, u8, (uint8_t*) &frame->body.creative_action.slot);
      PARSE_READ(&parser, slot, &frame->body.creative_action.clicked_slot);
      if (frame->body.creative_action.slot > 8)
        return -1;
      break;
    case kMCEnchantItemType:
      PARSE_READ(&parser, u8, (uint8_t*) &frame->body.enchant_item.window);
      PARSE_READ(&parser, u8, (uint8_t*) &frame->body.enchant_item.enchantment);
      break;
    case kMCUpdateSignType:
      return mc_parser__parse_update_sign(&parser, frame);
    case kMCPlayerAbilitiesType:
      return mc_parser__parse_abilities(&parser, frame);
    case kMCTabCompleteType:
      PARSE_READ(&parser, string, &frame->body.tab_complete);
      break;
    case kMCClientSettingsType:
      return mc_parser__parse_settings(&parser, frame);
    case kMCClientStatusType:
      PARSE_READ(&parser, u8, &tmp);
      frame->body.client_status = (mc_client_status_t) tmp;
      break;
    case kMCEncryptionResType:
      return mc_parser__parse_enc_resp(&parser, frame);
    case kMCPluginMsgType:
      return mc_parser__parse_plugin_msg(&parser, frame);
    case kMCServerListPingType:
      PARSE_READ(&parser, u8, (uint8_t*) &frame->body.server_list_ping);
      break;
    case kMCKickType:
      PARSE_READ(&parser, string, &frame->body.kick);
      break;
    default:
      /* Unknown frame, or frame should be sent by server */
      return -1;
  }

  return parser.offset;
}


int mc_parser__read_u8(mc_parser_t* p, uint8_t* out) {
  if (p->len < 1)
    return 0;
  *out = p->data[0];
  p->data++;
  p->offset++;
  p->len--;

  return 1;
}


int mc_parser__read_u16(mc_parser_t* p, uint16_t* out) {
  if (p->len < 2)
    return 0;
  *out = ntohs(*(uint16_t*) p->data);
  p->data += 2;
  p->offset += 2;
  p->len -= 2;

  return 1;
}


int mc_parser__read_u32(mc_parser_t* p, uint32_t* out) {
  if (p->len < 4)
    return 0;
  *out = ntohl(*(uint32_t*) p->data);
  p->data += 4;
  p->offset += 4;
  p->len -= 4;

  return 1;
}


int mc_parser__read_u64(mc_parser_t* p, uint64_t* out) {
  uint32_t hi;
  uint32_t lo;
  if (p->len < 8)
    return 0;

  PARSE_READ(p, u32, &hi);
  PARSE_READ(p, u32, &lo);
  *out = ((uint64_t) hi << 32) | lo;

  return 1;
}


int mc_parser__read_float(mc_parser_t* p, float* out) {
  if (p->len < 4)
    return 0;

  *out = *(float*) p->data;
  p->data += 4;
  p->offset += 4;
  p->len -= 4;

  return 1;
}


int mc_parser__read_double(mc_parser_t* p, double* out) {
  if (p->len < 8)
    return 0;

  *out = *(double*) p->data;
  p->data += 8;
  p->offset += 8;
  p->len -= 8;

  return 1;
}


int mc_parser__read_string(mc_parser_t* p, mc_string_t* str) {
  uint16_t len;

  mc_string_init(str);

  PARSE_READ(p, u16, &len);
  if (p->len < (size_t) len * 2)
    return 0;

  str->data = (uint16_t*) p->data;
  str->len = len * 2;

  p->data += str->len;
  p->offset += str->len;
  p->len -= str->len;

  return 1;
}


int mc_parser__read_slot(mc_parser_t* p, mc_slot_t* slot) {
  mc_slot_init(slot);

  PARSE_READ(p, u16, &slot->id);
  if (slot->id == 0xffff)
    return 1;

  PARSE_READ(p, u16, &slot->count);
  PARSE_READ(p, u16, &slot->damage);
  PARSE_READ(p, u16, &slot->nbt_len);
  if (slot->nbt_len == 0xfff)
    return 1;

  PARSE_READ_RAW(p, &slot->nbt, slot->nbt_len);

  return 1;
}


int mc_parser__read_raw(mc_parser_t* p, unsigned char** out, size_t size) {
  if (p->len < size)
    return 0;

  *out = (unsigned char*) p->data;
  p->data += size;
  p->offset += size;
  p->len -= size;

  return 1;
}


int mc_parser__parse_login_req(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 11)
    return 0;

  PARSE_READ(p, u32, &frame->body.login_req.entity_id);
  PARSE_READ(p, string, &frame->body.login_req.level);
  PARSE_READ(p, u8, &frame->body.login_req.game_mode);
  PARSE_READ(p, u8, (uint8_t*) &frame->body.login_req.dimension);
  PARSE_READ(p, u8, &frame->body.login_req.difficulty);
  PARSE_READ(p, u8, &frame->body.login_req.max_players);

  return p->offset;
}


int mc_parser__parse_handshake(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 9)
    return 0;

  PARSE_READ(p, u8, &frame->body.handshake.version);
  PARSE_READ(p, string, &frame->body.handshake.username);
  PARSE_READ(p, string, &frame->body.handshake.host);
  PARSE_READ(p, u32, &frame->body.handshake.port);

  return p->offset;
}


int mc_parser__parse_use_entity(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 9)
    return 0;

  PARSE_READ(p, u32, &frame->body.use_entity.user);
  PARSE_READ(p, u32, &frame->body.use_entity.target);
  PARSE_READ(p, u8, &frame->body.use_entity.button);

  return p->offset;
}


int mc_parser__parse_pos(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 33)
    return 0;

  PARSE_READ(p, double, &frame->body.pos_and_look.x);
  PARSE_READ(p, double, &frame->body.pos_and_look.y);
  PARSE_READ(p, double, &frame->body.pos_and_look.stance);
  PARSE_READ(p, double, &frame->body.pos_and_look.z);
  PARSE_READ(p, u8, &frame->body.pos_and_look.on_ground);

  frame->body.pos_and_look.yaw = 0;
  frame->body.pos_and_look.pitch = 0;

  return p->offset;
}


int mc_parser__parse_look(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 9)
    return 0;

  PARSE_READ(p, float, &frame->body.pos_and_look.yaw);
  PARSE_READ(p, float, &frame->body.pos_and_look.pitch);
  PARSE_READ(p, u8, &frame->body.pos_and_look.on_ground);

  frame->body.pos_and_look.x = 0;
  frame->body.pos_and_look.y = 0;
  frame->body.pos_and_look.z = 0;
  frame->body.pos_and_look.stance = 0;

  return p->offset;
}


int mc_parser__parse_pos_and_look(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 41)
    return 0;

  PARSE_READ(p, double, &frame->body.pos_and_look.x);
  PARSE_READ(p, double, &frame->body.pos_and_look.y);
  PARSE_READ(p, double, &frame->body.pos_and_look.stance);
  PARSE_READ(p, double, &frame->body.pos_and_look.z);
  PARSE_READ(p, float, &frame->body.pos_and_look.yaw);
  PARSE_READ(p, float, &frame->body.pos_and_look.pitch);
  PARSE_READ(p, u8, &frame->body.pos_and_look.on_ground);

  return p->offset;
}


int mc_parser__parse_digging(mc_parser_t* p, mc_frame_t* frame) {
  uint8_t tmp;

  if (p->len < 11)
    return 0;

  PARSE_READ(p, u8, &tmp);
  if (tmp > 5)
    return -1;
  frame->body.digging.status = (mc_digging_status_t) tmp;
  PARSE_READ(p, u32, (uint32_t*) &frame->body.digging.x);
  PARSE_READ(p, u8, (uint8_t*) &frame->body.digging.y);
  PARSE_READ(p, u32, (uint32_t*) &frame->body.digging.z);
  PARSE_READ(p, u8, &tmp);
  if (tmp > 5)
    return -1;
  frame->body.digging.face = (mc_face_t) tmp;

  return p->offset;
}


int mc_parser__parse_block_placement(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 13)
    return 0;

  PARSE_READ(p, u32, (uint32_t*) &frame->body.block_placement.x);
  PARSE_READ(p, u8, &frame->body.block_placement.y);
  PARSE_READ(p, u32, (uint32_t*) &frame->body.block_placement.z);
  PARSE_READ(p, u8, (uint8_t*) &frame->body.block_placement.direction);
  PARSE_READ(p, slot, &frame->body.block_placement.held_item);
  PARSE_READ(p, u8, (uint8_t*) &frame->body.block_placement.cursor_x);
  PARSE_READ(p, u8, (uint8_t*) &frame->body.block_placement.cursor_y);
  PARSE_READ(p, u8, (uint8_t*) &frame->body.block_placement.cursor_z);

  /* Validate cursor pos */
  if (frame->body.block_placement.cursor_x < 0 ||
      frame->body.block_placement.cursor_x > 16 ||
      frame->body.block_placement.cursor_y < 0 ||
      frame->body.block_placement.cursor_y > 16 ||
      frame->body.block_placement.cursor_z < 0 ||
      frame->body.block_placement.cursor_z > 16) {
    return -1;
  }

  return p->offset;
}


int mc_parser__parse_steer_vehicle(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 10)
    return 0;

  PARSE_READ(p, float, &frame->body.steer_vehicle.sideways);
  PARSE_READ(p, float, &frame->body.steer_vehicle.forward);
  PARSE_READ(p, u8, &frame->body.steer_vehicle.jump);
  PARSE_READ(p, u8, &frame->body.steer_vehicle.unmount);

  return p->offset;
}


int mc_parser__parse_click_window(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 7)
    return 0;

  PARSE_READ(p, u8, (uint8_t*) &frame->body.click_window.window);
  PARSE_READ(p, u16, &frame->body.click_window.slot);
  PARSE_READ(p, u8, &frame->body.click_window.button);
  PARSE_READ(p, u16, &frame->body.click_window.action_number);
  PARSE_READ(p, u8, &frame->body.click_window.mode);
  PARSE_READ(p, slot, &frame->body.click_window.clicked_item);

  return p->offset;
}


int mc_parser__parse_confirm_transaction(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 4)
    return 0;

  PARSE_READ(p, u8, (uint8_t*) &frame->body.confirm_transaction.window);
  PARSE_READ(p, u16, &frame->body.confirm_transaction.action_id);
  PARSE_READ(p, u8, &frame->body.confirm_transaction.accepted);

  return p->offset;
}


int mc_parser__parse_update_sign(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 10)
    return 0;

  PARSE_READ(p, u32, (uint32_t*) &frame->body.update_sign.x);
  PARSE_READ(p, u16, (uint16_t*) &frame->body.update_sign.y);
  PARSE_READ(p, u32, (uint32_t*) &frame->body.update_sign.z);
  PARSE_READ(p, string, &frame->body.update_sign.lines[0]);
  PARSE_READ(p, string, &frame->body.update_sign.lines[1]);
  PARSE_READ(p, string, &frame->body.update_sign.lines[2]);
  PARSE_READ(p, string, &frame->body.update_sign.lines[3]);

  return p->offset;
}


int mc_parser__parse_abilities(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 9)
    return 0;

  PARSE_READ(p, u8, &frame->body.player_abilities.flags);
  PARSE_READ(p, float, &frame->body.player_abilities.flying_speed);
  PARSE_READ(p, float, &frame->body.player_abilities.walking_speed);

  return p->offset;
}


int mc_parser__parse_settings(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 6)
    return 0;

  PARSE_READ(p, string, &frame->body.settings.locale);
  PARSE_READ(p, u8, &frame->body.settings.view_distance);
  PARSE_READ(p, u8, &frame->body.settings.chat_flags);
  PARSE_READ(p, u8, &frame->body.settings.difficulty);
  PARSE_READ(p, u8, &frame->body.settings.show_cape);

  return p->offset;
}


int mc_parser__parse_enc_resp(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 4)
    return 0;

  PARSE_READ(p, u16, &frame->body.enc_resp.secret_len);
  PARSE_READ_RAW(p,
                 &frame->body.enc_resp.secret,
                 frame->body.enc_resp.secret_len);
  PARSE_READ(p, u16, &frame->body.enc_resp.token_len);
  PARSE_READ_RAW(p,
                 &frame->body.enc_resp.token,
                 frame->body.enc_resp.token_len);

  return p->offset;
}


int mc_parser__parse_plugin_msg(mc_parser_t* p, mc_frame_t* frame) {
  if (p->len < 4)
    return 0;

  PARSE_READ(p, string, &frame->body.plugin_msg.channel);
  PARSE_READ(p, u16, &frame->body.plugin_msg.msg_len);
  PARSE_READ_RAW(p,
                 &frame->body.plugin_msg.msg,
                 frame->body.plugin_msg.msg_len);

  return p->offset;
}
