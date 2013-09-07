#include <arpa/inet.h>  /* ntohs, ntohl */
#include <assert.h>  /* assert */
#include <stdint.h>  /* uint8_t */
#include <stdlib.h>  /* abort */

#include "protocol/parser.h"
#include "utils/buffer.h"  /* mc_buffer_t */
#include "utils/common.h"  /* mc_slot_t */
#include "utils/string.h"  /* mc_string_t */

static int mc_parser__parse_login_req(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_handshake(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_use_entity(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_pos(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_look(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_pos_and_look(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_digging(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_block_placement(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_steer_vehicle(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_click_window(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_confirm_transaction(mc_buffer_t* b,
                                                mc_frame_t* frame);
static int mc_parser__parse_update_sign(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_abilities(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_settings(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_enc_resp(mc_buffer_t* b, mc_frame_t* frame);
static int mc_parser__parse_plugin_msg(mc_buffer_t* b, mc_frame_t* frame);

int mc_parser_execute(uint8_t* data, int len, mc_frame_t* frame) {
  int r;
  uint8_t type;
  uint8_t tmp;
  mc_buffer_t buffer;

  mc_buffer_from_data(&buffer, data, len);

  MC_BUFFER_READ(&buffer, u8, &type);
  frame->type = (mc_frame_type_t) type;

  r = 0;
  switch (frame->type) {
    case kMCKeepAliveType:
      MC_BUFFER_READ(&buffer, u32, &frame->body.keepalive);
      break;
    case kMCLoginReqType:
      r = mc_parser__parse_login_req(&buffer, frame);
      break;
    case kMCHandshakeType:
      r = mc_parser__parse_handshake(&buffer, frame);
      break;
    case kMCChatMsgType:
      MC_BUFFER_READ(&buffer, string, &frame->body.chat_msg);
      break;
    case kMCUseEntityType:
      r = mc_parser__parse_use_entity(&buffer, frame);
      break;
    case kMCPlayerType:
      MC_BUFFER_READ(&buffer, u8, &frame->body.pos_and_look.on_ground);
      frame->body.pos_and_look.x = 0;
      frame->body.pos_and_look.y = 0;
      frame->body.pos_and_look.z = 0;
      frame->body.pos_and_look.stance = 0;
      frame->body.pos_and_look.yaw = 0;
      frame->body.pos_and_look.pitch = 0;
      break;
    case kMCPlayerPosType:
      r = mc_parser__parse_pos(&buffer, frame);
      break;
    case kMCPlayerLookType:
      r = mc_parser__parse_look(&buffer, frame);
      break;
    case kMCPosAndLookType:
      r = mc_parser__parse_pos_and_look(&buffer, frame);
      break;
    case kMCDiggingType:
      r = mc_parser__parse_digging(&buffer, frame);
      break;
    case kMCBlockPlacementType:
      r = mc_parser__parse_block_placement(&buffer, frame);
      break;
    case kMCHeldItemChangeType:
      MC_BUFFER_READ(&buffer, u16, &frame->body.held_item_change.slot_id);
      if (frame->body.held_item_change.slot_id > kMCMaxHeldSlot)
        return kMCBufferUnknown;
      break;
    case kMCAnimationType:
      MC_BUFFER_READ(&buffer, u32, &frame->body.animation.entity_id);
      MC_BUFFER_READ(&buffer, u8, &tmp);
      frame->body.animation.kind = (mc_animation_t) tmp;
      break;
    case kMCEntityActionType:
      MC_BUFFER_READ(&buffer, u32, &frame->body.entity_action.entity_id);
      MC_BUFFER_READ(&buffer, u8, &tmp);
      frame->body.entity_action.action = (mc_entity_action_t) tmp;
      MC_BUFFER_READ(&buffer, u32, &frame->body.entity_action.boost);
      if (frame->body.entity_action.boost > 100)
        return kMCBufferUnknown;
      break;
    case kMCSteerVehicleType:
      return mc_parser__parse_steer_vehicle(&buffer, frame);
    case kMCCloseWindowType:
      MC_BUFFER_READ(&buffer, u8, (uint8_t*) &frame->body.close_window);
      break;
    case kMCClickWindowType:
      r = mc_parser__parse_click_window(&buffer, frame);
      break;
    case kMCConfirmTransactionType:
      r = mc_parser__parse_confirm_transaction(&buffer, frame);
      break;
    case kMCCreativeInvActionType:
      MC_BUFFER_READ(&buffer, u8, (uint8_t*) &frame->body.creative_action.slot);
      MC_BUFFER_READ(&buffer, slot, &frame->body.creative_action.clicked_slot);
      if (frame->body.creative_action.slot > kMCMaxInventorySlot)
        return kMCBufferUnknown;
      break;
    case kMCEnchantItemType:
      MC_BUFFER_READ(&buffer, u8, (uint8_t*) &frame->body.enchant_item.window);
      MC_BUFFER_READ(&buffer, u8, (uint8_t*) &frame->body.enchant_item.enchantment);
      break;
    case kMCUpdateSignType:
      r = mc_parser__parse_update_sign(&buffer, frame);
      break;
    case kMCPlayerAbilitiesType:
      r = mc_parser__parse_abilities(&buffer, frame);
      break;
    case kMCTabCompleteType:
      MC_BUFFER_READ(&buffer, string, &frame->body.tab_complete);
      break;
    case kMCClientSettingsType:
      r = mc_parser__parse_settings(&buffer, frame);
      break;
    case kMCClientStatusType:
      MC_BUFFER_READ(&buffer, u8, &tmp);
      frame->body.client_status = (mc_client_status_t) tmp;
      break;
    case kMCEncryptionResType:
      r = mc_parser__parse_enc_resp(&buffer, frame);
      break;
    case kMCPluginMsgType:
      r = mc_parser__parse_plugin_msg(&buffer, frame);
      break;
    case kMCServerListPingType:
      MC_BUFFER_READ(&buffer, u8, (uint8_t*) &frame->body.server_list_ping);
      break;
    case kMCKickType:
      MC_BUFFER_READ(&buffer, string, &frame->body.kick);
      break;
    default:
      /* Unknown frame, or frame should be sent by server */
      return kMCBufferUnknown;
  }

  if (r == 0)
    return mc_buffer_offset(&buffer);
  else
    return r;
}


int mc_parser__parse_login_req(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 11)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u32, &frame->body.login_req.entity_id);
  MC_BUFFER_READ(b, string, &frame->body.login_req.level);
  MC_BUFFER_READ(b, u8, &frame->body.login_req.game_mode);
  MC_BUFFER_READ(b, u8, (uint8_t*) &frame->body.login_req.dimension);
  MC_BUFFER_READ(b, u8, &frame->body.login_req.difficulty);
  MC_BUFFER_READ(b, u8, &frame->body.login_req.max_players);

  return 0;
}


int mc_parser__parse_handshake(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 9)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u8, &frame->body.handshake.version);
  MC_BUFFER_READ(b, string, &frame->body.handshake.username);
  MC_BUFFER_READ(b, string, &frame->body.handshake.host);
  MC_BUFFER_READ(b, u32, &frame->body.handshake.port);

  return 0;
}


int mc_parser__parse_use_entity(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 9)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u32, &frame->body.use_entity.user);
  MC_BUFFER_READ(b, u32, &frame->body.use_entity.target);
  MC_BUFFER_READ(b, u8, &frame->body.use_entity.button);

  return 0;
}


int mc_parser__parse_pos(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 33)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, double, &frame->body.pos_and_look.x);
  MC_BUFFER_READ(b, double, &frame->body.pos_and_look.y);
  MC_BUFFER_READ(b, double, &frame->body.pos_and_look.stance);
  MC_BUFFER_READ(b, double, &frame->body.pos_and_look.z);
  MC_BUFFER_READ(b, u8, &frame->body.pos_and_look.on_ground);

  frame->body.pos_and_look.yaw = 0;
  frame->body.pos_and_look.pitch = 0;

  return 0;
}


int mc_parser__parse_look(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 9)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, float, &frame->body.pos_and_look.yaw);
  MC_BUFFER_READ(b, float, &frame->body.pos_and_look.pitch);
  MC_BUFFER_READ(b, u8, &frame->body.pos_and_look.on_ground);

  frame->body.pos_and_look.x = 0;
  frame->body.pos_and_look.y = 0;
  frame->body.pos_and_look.z = 0;
  frame->body.pos_and_look.stance = 0;

  return 0;
}


int mc_parser__parse_pos_and_look(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 41)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, double, &frame->body.pos_and_look.x);
  MC_BUFFER_READ(b, double, &frame->body.pos_and_look.y);
  MC_BUFFER_READ(b, double, &frame->body.pos_and_look.stance);
  MC_BUFFER_READ(b, double, &frame->body.pos_and_look.z);
  MC_BUFFER_READ(b, float, &frame->body.pos_and_look.yaw);
  MC_BUFFER_READ(b, float, &frame->body.pos_and_look.pitch);
  MC_BUFFER_READ(b, u8, &frame->body.pos_and_look.on_ground);

  return 0;
}


int mc_parser__parse_digging(mc_buffer_t* b, mc_frame_t* frame) {
  uint8_t tmp;

  if (b->len < 11)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u8, &tmp);
  if (tmp > 5)
    return -1;
  frame->body.digging.status = (mc_digging_status_t) tmp;
  MC_BUFFER_READ(b, u32, (uint32_t*) &frame->body.digging.x);
  MC_BUFFER_READ(b, u8, (uint8_t*) &frame->body.digging.y);
  MC_BUFFER_READ(b, u32, (uint32_t*) &frame->body.digging.z);
  MC_BUFFER_READ(b, u8, &tmp);
  if (tmp > 5)
    return -1;
  frame->body.digging.face = (mc_face_t) tmp;

  return 0;
}


int mc_parser__parse_block_placement(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 13)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u32, (uint32_t*) &frame->body.block_placement.x);
  MC_BUFFER_READ(b, u8, &frame->body.block_placement.y);
  MC_BUFFER_READ(b, u32, (uint32_t*) &frame->body.block_placement.z);
  MC_BUFFER_READ(b, u8, (uint8_t*) &frame->body.block_placement.direction);
  MC_BUFFER_READ(b, slot, &frame->body.block_placement.held_item);
  MC_BUFFER_READ(b, u8, (uint8_t*) &frame->body.block_placement.cursor_x);
  MC_BUFFER_READ(b, u8, (uint8_t*) &frame->body.block_placement.cursor_y);
  MC_BUFFER_READ(b, u8, (uint8_t*) &frame->body.block_placement.cursor_z);

  /* Validate cursor pos */
  if (frame->body.block_placement.cursor_x < 0 ||
      frame->body.block_placement.cursor_x > 16 ||
      frame->body.block_placement.cursor_y < 0 ||
      frame->body.block_placement.cursor_y > 16 ||
      frame->body.block_placement.cursor_z < 0 ||
      frame->body.block_placement.cursor_z > 16) {
    return -1;
  }

  return 0;
}


int mc_parser__parse_steer_vehicle(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 10)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, float, &frame->body.steer_vehicle.sideways);
  MC_BUFFER_READ(b, float, &frame->body.steer_vehicle.forward);
  MC_BUFFER_READ(b, u8, &frame->body.steer_vehicle.jump);
  MC_BUFFER_READ(b, u8, &frame->body.steer_vehicle.unmount);

  return 0;
}


int mc_parser__parse_click_window(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 7)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u8, (uint8_t*) &frame->body.click_window.window);
  MC_BUFFER_READ(b, u16, &frame->body.click_window.slot);
  MC_BUFFER_READ(b, u8, &frame->body.click_window.button);
  MC_BUFFER_READ(b, u16, &frame->body.click_window.action_number);
  MC_BUFFER_READ(b, u8, &frame->body.click_window.mode);
  MC_BUFFER_READ(b, slot, &frame->body.click_window.clicked_item);

  return 0;
}


int mc_parser__parse_confirm_transaction(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 4)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u8, (uint8_t*) &frame->body.confirm_transaction.window);
  MC_BUFFER_READ(b, u16, &frame->body.confirm_transaction.action_id);
  MC_BUFFER_READ(b, u8, &frame->body.confirm_transaction.accepted);

  return 0;
}


int mc_parser__parse_update_sign(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 10)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u32, (uint32_t*) &frame->body.update_sign.x);
  MC_BUFFER_READ(b, u16, (uint16_t*) &frame->body.update_sign.y);
  MC_BUFFER_READ(b, u32, (uint32_t*) &frame->body.update_sign.z);
  MC_BUFFER_READ(b, string, &frame->body.update_sign.lines[0]);
  MC_BUFFER_READ(b, string, &frame->body.update_sign.lines[1]);
  MC_BUFFER_READ(b, string, &frame->body.update_sign.lines[2]);
  MC_BUFFER_READ(b, string, &frame->body.update_sign.lines[3]);

  return 0;
}


int mc_parser__parse_abilities(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 9)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u8, &frame->body.player_abilities.flags);
  MC_BUFFER_READ(b, float, &frame->body.player_abilities.flying_speed);
  MC_BUFFER_READ(b, float, &frame->body.player_abilities.walking_speed);

  return 0;
}


int mc_parser__parse_settings(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 6)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, string, &frame->body.settings.locale);
  MC_BUFFER_READ(b, u8, &frame->body.settings.view_distance);
  MC_BUFFER_READ(b, u8, &frame->body.settings.chat_flags);
  MC_BUFFER_READ(b, u8, &frame->body.settings.difficulty);
  MC_BUFFER_READ(b, u8, &frame->body.settings.show_cape);

  return 0;
}


int mc_parser__parse_enc_resp(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 4)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, u16, &frame->body.enc_resp.secret_len);
  MC_BUFFER_READ_DATA(b,
                      &frame->body.enc_resp.secret,
                      frame->body.enc_resp.secret_len);
  MC_BUFFER_READ(b, u16, &frame->body.enc_resp.token_len);
  MC_BUFFER_READ_DATA(b,
                      &frame->body.enc_resp.token,
                      frame->body.enc_resp.token_len);

  return 0;
}


int mc_parser__parse_plugin_msg(mc_buffer_t* b, mc_frame_t* frame) {
  if (b->len < 4)
    return kMCBufferOOB;

  MC_BUFFER_READ(b, string, &frame->body.plugin_msg.channel);
  MC_BUFFER_READ(b, u16, &frame->body.plugin_msg.msg_len);
  MC_BUFFER_READ_DATA(b,
                      &frame->body.plugin_msg.msg,
                      frame->body.plugin_msg.msg_len);

  return 0;
}
