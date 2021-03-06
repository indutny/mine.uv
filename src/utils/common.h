#ifndef SRC_UTILS_COMMON_H_
#define SRC_UTILS_COMMON_H_

#include <stdint.h>  /* uint8_t */

#include "utils/string.h"  /* mc_string_t */

/* Forward-declarations */
struct mc_nbt_s;

#define MC_ENTITY_LIST(ENTITY_DECL) \
    ENTITY_DECL(DroppedItem, 0x1, "Item") \
    ENTITY_DECL(ExperienceOrb, 0x2, "XPOrb") \
    ENTITY_DECL(LeadKnot, 0x8, "LeashKnot") \
    ENTITY_DECL(Painting, 0x9, "Painting") \
    ENTITY_DECL(ItemFrame, 0x12, "ItemFrame") \
    ENTITY_DECL(ShotArrow, 0xA, "Arrow") \
    ENTITY_DECL(ThrownSnowball, 0xB, "Snowball") \
    ENTITY_DECL(GhastFireball, 0xC, "Fireball") \
    ENTITY_DECL(BlazeFireball, 0xD, "SmallFireball") \
    ENTITY_DECL(ThrownEnderPearl, 0xE, "ThrownEnderpearl") \
    ENTITY_DECL(ThrownEyeOfEnder, 0xF, "EyeOfEnderSignal") \
    ENTITY_DECL(ThrownSplashPotion, 0x10, "ThrownPotion") \
    ENTITY_DECL(ThrownBottleEnchanting, 0x11, "ThrownExpBottle") \
    ENTITY_DECL(WitherSkull, 0x13, "WitherSkull") \
    ENTITY_DECL(FireworkRocket, 0x16, "FireworksRocketEntity") \
    ENTITY_DECL(PrimedTNT, 0x14, "PrimedTnt") \
    ENTITY_DECL(FallingBlock, 0x15, "FallingSand") \
    ENTITY_DECL(Boat, 0x29, "Boat") \
    ENTITY_DECL(Minecart, 0x2A, "MinecartRideable") \
    ENTITY_DECL(MinecartWithChest, 0x2B, "MinecartChest") \
    ENTITY_DECL(MinecartWithFurnace, 0x2C, "MinecartFurnace") \
    ENTITY_DECL(MinecartWithTNT, 0x2D, "MinecartTNT") \
    ENTITY_DECL(MinecartWithHopper, 0x2E, "MinecartHopper") \
    ENTITY_DECL(MinecartWithSpawner, 0x2F, "MinecartSpawner") \
    ENTITY_DECL(Mob, 0x30, "Mob") \
    ENTITY_DECL(Monster, 0x31, "Monster") \
    ENTITY_DECL(Creeper, 0x32, "Creeper") \
    ENTITY_DECL(Skeleton, 0x33, "Skeleton") \
    ENTITY_DECL(Spider, 0x34, "Spider") \
    ENTITY_DECL(Giant, 0x35, "Giant") \
    ENTITY_DECL(Zombie, 0x36, "Zombie") \
    ENTITY_DECL(Slime, 0x37, "Slime") \
    ENTITY_DECL(Ghast, 0x38, "Ghast") \
    ENTITY_DECL(ZombiePigman, 0x39, "PigZombie") \
    ENTITY_DECL(Enderman, 0x3A, "Enderman") \
    ENTITY_DECL(CaveSpider, 0x3B, "CaveSpider") \
    ENTITY_DECL(Silverfish, 0x3C, "Silverfish") \
    ENTITY_DECL(Blaze, 0x3D, "Blaze") \
    ENTITY_DECL(MagmaCube, 0x3E, "LavaSlime") \
    ENTITY_DECL(EnderDragon, 0x3F, "EnderDragon") \
    ENTITY_DECL(Wither, 0x40, "WitherBoss") \
    ENTITY_DECL(Witch, 0x42, "Witch") \
    ENTITY_DECL(Bat, 0x41, "Bat") \
    ENTITY_DECL(Pig, 0x5A, "Pig") \
    ENTITY_DECL(Sheep, 0x5B, "Sheep") \
    ENTITY_DECL(Cow, 0x5C, "Cow") \
    ENTITY_DECL(Chicken, 0x5D, "Chicken") \
    ENTITY_DECL(Squid, 0x5E, "Squid") \
    ENTITY_DECL(Wolf, 0x5F, "Wolf") \
    ENTITY_DECL(Mooshroom, 0x60, "MushroomCow") \
    ENTITY_DECL(SnowGolem, 0x61, "SnowMan") \
    ENTITY_DECL(Ocelot, 0x62, "Ozelot") \
    ENTITY_DECL(IronGolem, 0x63, "VillagerGolem") \
    ENTITY_DECL(Horse, 0x64, "EntityHorse") \
    ENTITY_DECL(Villager, 0x78, "Villager") \
    ENTITY_DECL(EnderCrystal, 0xC8, "EnderCrystal")

typedef enum mc_frame_type_e mc_frame_type_t;
typedef enum mc_client_status_e mc_client_status_t;
typedef enum mc_digging_status_e mc_digging_status_t;
typedef enum mc_face_e mc_face_t;
typedef enum mc_animation_e mc_animation_t;
typedef enum mc_entity_action_e mc_entity_action_t;
typedef enum mc_biome_e mc_biome_t;
typedef enum mc_block_id_e mc_block_id_t;
typedef enum mc_entity_id_e mc_entity_id_t;
typedef struct mc_frame_s mc_frame_t;
typedef struct mc_slot_s mc_slot_t;
typedef struct mc_region_s mc_region_t;
typedef struct mc_column_s mc_column_t;
typedef struct mc_chunk_s mc_chunk_t;
typedef union mc_uuid_u mc_uuid_t;
typedef struct mc_block_s mc_block_t;
typedef struct mc_entity_s mc_entity_t;
typedef struct mc_mob_s mc_mob_t;
typedef struct mc_player_s mc_player_t;

#define MC_COLUMN_MAX_X 32
#define MC_COLUMN_MAX_Z 32
#define MC_COLUMN_MAX_Y 16
#define MC_CHUNK_MAX_X 16
#define MC_CHUNK_MAX_Z 16
#define MC_CHUNK_MAX_Y 16

static const int kMCColumnMaxX = MC_COLUMN_MAX_X;
static const int kMCColumnMaxZ = MC_COLUMN_MAX_Z;
static const int kMCColumnMaxY = MC_COLUMN_MAX_Y;
static const int kMCChunkMaxX = MC_CHUNK_MAX_X;
static const int kMCChunkMaxZ = MC_CHUNK_MAX_Z;
static const int kMCChunkMaxY = MC_CHUNK_MAX_Y;
static const int kMCMaxHeldSlot = 8;
static const int kMCMaxInventorySlot = 44;

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

#define ENTITY_DECL(id, value, str) kMCEntity##id = value,

enum mc_entity_id_e {
  MC_ENTITY_LIST(ENTITY_DECL)
  kMCEntityPlayer = 0xfe,
  kMCEntityUnknown = 0xff
};

#undef ENTITY_DECL

struct mc_slot_s {
  uint16_t id;
  uint16_t count;
  uint16_t damage;
  uint16_t nbt_len;
  unsigned char* nbt;
  int allocated;
};

union mc_uuid_u {
  unsigned char raw[16];
  struct {
    uint64_t high;
    uint64_t low;
  } obj;
};

struct mc_block_s {
  mc_block_id_t id;
  uint8_t metadata;
  uint8_t light;
  uint8_t skylight;
  struct mc_nbt_s* tile_data;
};

struct mc_entity_s {
  mc_entity_id_t id;
  int8_t on_ground;
  int8_t invulnerable;
  int16_t air;
  int16_t fire;
  float fall_distance;
  double motion_x;
  double motion_y;
  double motion_z;
  double pos_x;
  double pos_y;
  double pos_z;
  float yaw;
  float pitch;
  mc_uuid_t uuid;

  struct mc_nbt_s* nbt;
};

struct mc_mob_s {
  int16_t health;
  float health_f;
  float absorption_amount;
  int16_t attack_time;
  int16_t hurt_time;
  int16_t death_time;
  int8_t can_pick_up_loot;
  int8_t persistence_required;
  char* custom_name;
  int custom_name_len;
  int8_t custom_name_visible;
  int8_t leashed;
  int8_t leash_is_uuid;
  union {
    mc_uuid_t uuid;
    struct {
      int32_t x;
      int32_t y;
      int32_t z;
    } pos;
  } leash;
};

struct mc_player_s {
  mc_mob_t common;
};

struct mc_chunk_s {
  mc_block_t blocks[MC_CHUNK_MAX_X][MC_CHUNK_MAX_Z][MC_CHUNK_MAX_Y];
};

struct mc_column_s {
  int generated;
  int8_t populated;
  int32_t world_x;
  int32_t world_z;
  int64_t last_update;
  int64_t inhabited_time;

  mc_biome_t biomes[MC_CHUNK_MAX_X][MC_CHUNK_MAX_Z];
  int32_t height_map[MC_CHUNK_MAX_X][MC_CHUNK_MAX_Z];
  mc_chunk_t* chunks[MC_COLUMN_MAX_Y];
  mc_entity_t* entities;
  int entity_count;
};

struct mc_region_s {
  mc_column_t columns[MC_COLUMN_MAX_X][MC_COLUMN_MAX_Z];
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
mc_region_t* mc_region_new();
void mc_region_destroy(mc_region_t* region);

/* Entity utils */
mc_entity_id_t mc_entity_str_to_id(const char* val, int len);

/* Slot utils */
void mc_slot_init(mc_slot_t* slot);
void mc_slot_destroy(mc_slot_t* slot);

/* File utilities, should be called from worker threads */
int mc_read_file(const char* path, unsigned char** out);
int mc_write_file(const char* path,
                  const unsigned char* out,
                  int len,
                  int update);

#endif  /* SRC_UTILS_COMMON_H_ */
