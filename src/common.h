#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_

#include <stdint.h>  /* uint8_t */

typedef enum mc_frame_type_e mc_frame_type_t;
typedef enum mc_client_status_e mc_client_status_t;
typedef enum mc_digging_status_e mc_digging_status_t;
typedef enum mc_face_e mc_face_t;
typedef enum mc_animation_e mc_animation_t;
typedef enum mc_entity_action_e mc_entity_action_t;
typedef enum mc_biome_e mc_biome_t;
typedef enum mc_block_id_e mc_block_id_t;
typedef struct mc_frame_s mc_frame_t;
typedef struct mc_string_s mc_string_t;
typedef struct mc_slot_s mc_slot_t;
typedef struct mc_region_s mc_region_t;
typedef struct mc_column_s mc_column_t;
typedef struct mc_chunk_s mc_chunk_t;
typedef struct mc_block_s mc_block_t;

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

enum mc_biome_e {
  kMCBiomeOcean = 0,
  kMCBiomePlains = 1,
  kMCBiomeDesert = 2,
  kMCBiomeExtremeHills = 3,
  kMCBiomeForest = 4,
  kMCBiomeTaiga = 5,
  kMCBiomeSwampland = 6,
  kMCBiomeRiver = 7,
  kMCBiomeHell = 8,
  kMCBiomeSky = 9,
  kMCBiomeFrozenOcean = 10,
  kMCBiomeFrozenRiver = 11,
  kMCBiomeIcePlains = 12,
  kMCBiomeIceMountains = 13,
  kMCBiomeMushroomIsland = 14,
  kMCBiomeMushroomIslandShore = 15,
  kMCBiomeBeach = 16,
  kMCBiomeDesertHills = 17,
  kMCBiomeForestHills = 18,
  kMCBiomeTaigaHills = 19,
  kMCBiomeExtremeHillsEdge = 20,
  kMCBiomeJungle = 21,
  kMCBiomeJungleHills = 22,

  kMCBiomeNotGenerated = 0xff
};

enum mc_block_id_e {
  kMCBlockAir = 0x0,
  kMCBlockStone = 0x1,
  kMCBlockGrassBlock = 0x2,
  kMCBlockDirt = 0x3,
  kMCBlockCobblestone = 0x4,
  kMCBlockWoodPlanks = 0x5,
  kMCBlockSaplings = 0x6,
  kMCBlockBedrock = 0x7,
  kMCBlockWater = 0x8,
  kMCBlockStationaryWater = 0x9,
  kMCBlockLava = 0xA,
  kMCBlockStationaryLava = 0xB,
  kMCBlockSand = 0xC,
  kMCBlockGravel = 0xD,
  kMCBlockGoldOre = 0xE,
  kMCBlockIronOre = 0xF,
  kMCBlockCoalOre = 0x10,
  kMCBlockWood = 0x11,
  kMCBlockLeaves = 0x12,
  kMCBlockSponge = 0x13,
  kMCBlockGlass = 0x14,
  kMCBlockLapisLazuliOre = 0x15,
  kMCBlockLapisLazuliBlock = 0x16,
  kMCBlockDispenser = 0x17,
  kMCBlockSandstone = 0x18,
  kMCBlockNoteBlock = 0x19,
  kMCBlockBed = 0x1A,
  kMCBlockPoweredRail = 0x1B,
  kMCBlockDetectorRail = 0x1C,
  kMCBlockStickyPiston = 0x1D,
  kMCBlockCobweb = 0x1E,
  kMCBlockGrass = 0x1F,
  kMCBlockDeadBush = 0x20,
  kMCBlockPiston = 0x21,
  kMCBlockPistonExtension = 0x22,
  kMCBlockWool = 0x23,
  kMCBlockMovedByPiston = 0x24,
  kMCBlockDandelion = 0x25,
  kMCBlockRose = 0x26,
  kMCBlockBrownMushroom = 0x27,
  kMCBlockRedMushroom = 0x28,
  kMCBlockOfGold = 0x29,
  kMCBlockOfIron = 0x2A,
  kMCBlockDoubleStoneSlab = 0x2B,
  kMCBlockStoneSlab = 0x2C,
  kMCBlockBricks = 0x2D,
  kMCBlockTNT = 0x2E,
  kMCBlockBookshelf = 0x2F,
  kMCBlockMossStone = 0x30,
  kMCBlockObsidian = 0x31,
  kMCBlockTorch = 0x32,
  kMCBlockFire = 0x33,
  kMCBlockMonsterSpawner = 0x34,
  kMCBlockOakWoodStairs = 0x35,
  kMCBlockChest = 0x36,
  kMCBlockRedstoneWire = 0x37,
  kMCBlockDiamondOre = 0x38,
  kMCBlockOfDiamond = 0x39,
  kMCBlockCraftingTable = 0x3A,
  kMCBlockWheat = 0x3B,
  kMCBlockFarmland = 0x3C,
  kMCBlockFurnace = 0x3D,
  kMCBlockBurningFurnace = 0x3E,
  kMCBlockSignPost = 0x3F,
  kMCBlockWoodenDoor = 0x40,
  kMCBlockLadders = 0x41,
  kMCBlockRail = 0x42,
  kMCBlockCobblestoneStairs = 0x43,
  kMCBlockWallSign = 0x44,
  kMCBlockLever = 0x45,
  kMCBlockStonePressurePlate = 0x46,
  kMCBlockIronDoor = 0x47,
  kMCBlockWoodenPressurePlate = 0x48,
  kMCBlockRedstoneOre = 0x49,
  kMCBlockGlowingRedstoneOre = 0x4A,
  kMCBlockRedstoneTorchInactive = 0x4B,
  kMCBlockRedstoneTorchActive = 0x4C,
  kMCBlockStoneButton = 0x4D,
  kMCBlockSnow = 0x4E,
  kMCBlockIce = 0x4F,
  kMCBlockSnowBlock = 0x50,
  kMCBlockCactus = 0x51,
  kMCBlockClay = 0x52,
  kMCBlockSugarCane = 0x53,
  kMCBlockJukebox = 0x54,
  kMCBlockFence = 0x55,
  kMCBlockPumpkin = 0x56,
  kMCBlockNetherrack = 0x57,
  kMCBlockSoulSand = 0x58,
  kMCBlockGlowstone = 0x59,
  kMCBlockNetherPortal = 0x5A,
  kMCBlockJackOLantern = 0x5B,
  kMCBlockCakeBlock = 0x5C,
  kMCBlockRedstoneRepeaterInactive = 0x5D,
  kMCBlockRedstoneRepeaterActive = 0x5E,
  kMCBlockLockedChest = 0x5F,
  kMCBlockTrapdoor = 0x60,
  kMCBlockMonsterEgg = 0x61,
  kMCBlockStoneBricks = 0x62,
  kMCBlockHugeBrownMushroom = 0x63,
  kMCBlockHugeRedMushroom = 0x64,
  kMCBlockIronBars = 0x65,
  kMCBlockGlassPane = 0x66,
  kMCBlockMelon = 0x67,
  kMCBlockPumpkinStem = 0x68,
  kMCBlockMelonStem = 0x69,
  kMCBlockVines = 0x6A,
  kMCBlockFenceGate = 0x6B,
  kMCBlockBrickStairs = 0x6C,
  kMCBlockStoneBrickStairs = 0x6D,
  kMCBlockMycelium = 0x6E,
  kMCBlockLilyPad = 0x6F,
  kMCBlockNetherBrick = 0x70,
  kMCBlockNetherBrickFence = 0x71,
  kMCBlockNetherBrickStairs = 0x72,
  kMCBlockNetherWart = 0x73,
  kMCBlockEnchantmentTable = 0x74,
  kMCBlockBrewingStand = 0x75,
  kMCBlockCauldron = 0x76,
  kMCBlockEndPortal = 0x77,
  kMCBlockEndPortalBlock = 0x78,
  kMCBlockEndStone = 0x79,
  kMCBlockDragonEgg = 0x7A,
  kMCBlockRedstoneLampInactive = 0x7B,
  kMCBlockRedstoneLampActive = 0x7C,
  kMCBlockWoodenDoubleSlab = 0x7D,
  kMCBlockWoodenSlab = 0x7E,
  kMCBlockCocoa = 0x7F,
  kMCBlockSandstoneStairs = 0x80,
  kMCBlockEmeraldOre = 0x81,
  kMCBlockEnderChest = 0x82,
  kMCBlockTripwireHook = 0x83,
  kMCBlockTripwire = 0x84,
  kMCBlockofEmerald = 0x85,
  kMCBlockSpruceWoodStairs = 0x86,
  kMCBlockBirchWoodStairs = 0x87,
  kMCBlockJungleWoodStairs = 0x88,
  kMCBlockCommandBlock = 0x89,
  kMCBlockBeacon = 0x8A,
  kMCBlockCobblestoneWall = 0x8B,
  kMCBlockFlowerPot = 0x8C,
  kMCBlockCarrots = 0x8D,
  kMCBlockPotatoes = 0x8E,
  kMCBlockWoodenButton = 0x8F,
  kMCBlockMobHead = 0x90,
  kMCBlockAnvil = 0x91,
  kMCBlockTrappedChest = 0x92,
  kMCBlockWeightedPressurePlateLight = 0x93,
  kMCBlockWeightedPressurePlateHeavy = 0x94,
  kMCBlockRedstoneComparatorInactive = 0x95,
  kMCBlockRedstoneComparatorActive = 0x96,
  kMCBlockDaylightSensor = 0x97,
  kMCBlockOfRedstone = 0x98,
  kMCBlockNetherQuartzOre = 0x99,
  kMCBlockHopper = 0x9A,
  kMCBlockOfQuartz = 0x9B,
  kMCBlockQuartzStairs = 0x9C,
  kMCBlockActivatorRail = 0x9D,
  kMCBlockDropper = 0x9E,
  kMCBlockStainedClay = 0x9F,
  kMCBlockHayBlock = 0xAA,
  kMCBlockCarpet = 0xAB,
  kMCBlockHardenedClay = 0xAC,
  kMCBlockOfCoal = 0xAD
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

struct mc_block_s {
  mc_block_id_t block_id;
  uint8_t block_metadata;
  uint8_t block_light;
  uint8_t block_skylight;
};

struct mc_chunk_s {
  mc_block_t block[16][16][16];
};

struct mc_column_s {
  int8_t populated;
  int32_t world_x;
  int32_t world_z;
  int64_t last_update;
  mc_biome_t biomes[16][16];
  mc_chunk_t* chunks[16];

  /* TODO(indutny): Store entities too */
  /* TODO(indutny): Store heightmap too */
};

struct mc_region_s {
  mc_column_t column[32][32];
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

/* Region utils */
void mc_region_destroy(mc_region_t* region);

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

/* File utilities, should be called from worker threads */
int mc_read_file(const char* path, unsigned char** out);
int mc_write_file(const char* path,
                  const unsigned char* out,
                  int len,
                  int update);

#endif  /* SRC_COMMON_H_ */
