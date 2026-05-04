#ifndef MAP_H_
#define MAP_H_

#include "Defines.h"
#include "TileTypes.h"

#define MAP_OUT_OF_BOUNDS 0xff

enum MapRead_Orientation
{
	MapRead_Horizontal,
	MapRead_Vertical
};

enum DoorType
{
	DoorType_None,
	DoorType_StandardHorizontal,
	DoorType_StandardVertical,
	DoorType_Locked1Horizontal,
	DoorType_Locked1Vertical,
	DoorType_Locked2Horizontal,
	DoorType_Locked2Vertical,
	DoorType_ExitHorizontal,
	DoorType_ExitVertical,
	DoorType_SecretPushWall
};

enum DoorState
{
	DoorState_Idle = 0,
	DoorState_Opening,
	DoorState_Closing,
	DoorState_PushNorth,
	DoorState_PushEast,
	DoorState_PushSouth,
	DoorState_PushWest,
	DoorState_FirstPushWallState = DoorState_PushNorth
};

enum Direction
{
	Direction_None = -1,
	Direction_North,
	Direction_East,
	Direction_South,
	Direction_West,
};

#define DOOR_MAX_OPEN 63

typedef struct Door
{
	uint8_t type;
	int8_t x, z;
	uint8_t open;
	uint8_t state;
	uint8_t texture;
} Door;

typedef struct Item
{
	uint8_t type;
	int8_t x, z;
	uint8_t spawnId;
} Item;

typedef struct Map
{
	int8_t bufferX;
	int8_t bufferZ;
	Door doors[MAX_DOORS];
	Item items[MAX_ACTIVE_ITEMS];
	int8_t currentLevel;
	uint8_t m_mapBuffer[MAP_BUFFER_SIZE * MAP_BUFFER_SIZE];
	uint8_t m_itemState[256 / 8];
	uint8_t m_actorState[256 / 8];
	uint8_t m_streamBuffer[MAP_BUFFER_SIZE * 2];
#if WOLF3D_SEEN_WALL_BITSET
	uint8_t m_seenWalls[WOLF3D_SEEN_WALL_BYTES];
#endif
#if WOLF3D_FULL_MAP_RAM_CACHE
	uint8_t m_fullMap[MAP_SIZE * MAP_SIZE];
#endif
	bool m_mapLoaded;
} Map;

void Door_init(Door* self);
void Door_update(Door* self);

void Map_initStreaming(Map* self);
void Map_init(Map* self);
bool Map_isValid(Map* self, int8_t cellX, int8_t cellZ);
bool Map_isBlocked(Map* self, int8_t cellX, int8_t cellZ);
bool Map_isSolid(Map* self, int8_t cellX, int8_t cellZ);
bool Map_isDoor(Map* self, int8_t cellX, int8_t cellZ);
static inline bool Map_tileIsWall(uint8_t tile)
{
	return tile >= Tile_FirstWall && tile <= Tile_LastWall && tile != MAP_OUT_OF_BOUNDS;
}
static inline bool Map_tileIsDoor(uint8_t tile)
{
	return tile >= Tile_FirstDoor && tile <= Tile_LastDoor;
}
uint8_t Map_getTextureId(Map* self, int8_t cellX, int8_t cellZ);
uint8_t Map_getTile(Map* self, int8_t cellX, int8_t cellZ);
static inline uint8_t Map_getTileFast(Map* self, int8_t cellX, int8_t cellZ)
{
	cellX &= 0xf;
	cellZ &= 0xf;
	return self->m_mapBuffer[cellZ * MAP_BUFFER_SIZE + cellX];
}
void Map_updateBufferPosition(Map* self, int8_t newX, int8_t newZ);
uint8_t Map_readRawTile(Map* self, int8_t x, int8_t z);
#if WOLF3D_SEEN_WALL_BITSET
void Map_clearSeenWalls(Map* self);
void Map_markSeenWall(Map* self, int8_t x, int8_t z);
bool Map_isWallSeen(Map* self, int8_t x, int8_t z);
#endif
#if WOLF3D_FULL_MAP_RAM_CACHE
void Map_buildFullMapCache(Map* self);
#endif
void Map_update(Map* self);
void Map_openDoorsAt(Map* self, int8_t x, int8_t z, int8_t direction);
bool Map_placeItem(Map* self, uint8_t type, int8_t x, int8_t z, uint8_t spawnId);
static inline bool Map_isItemCollected(Map* self, uint8_t spawnId)
{
	uint8_t index = spawnId / 8;
	uint8_t mask = 1 << (spawnId - (index * 8));
	return (self->m_itemState[index] & mask) != 0;
}
static inline void Map_markItemCollected(Map* self, uint8_t spawnId)
{
	uint8_t index = spawnId / 8;
	uint8_t mask = 1 << (spawnId - (index * 8));
	self->m_itemState[index] |= mask;
}
static inline bool Map_isActorKilled(Map* self, uint8_t spawnId)
{
	uint8_t index = spawnId / 8;
	uint8_t mask = 1 << (spawnId - (index * 8));
	return (self->m_actorState[index] & mask) != 0;
}
static inline void Map_markActorKilled(Map* self, uint8_t spawnId)
{
	uint8_t index = spawnId / 8;
	uint8_t mask = 1 << (spawnId - (index * 8));
	self->m_actorState[index] |= mask;
}
bool Map_isClearLine(Map* self, int16_t x1, int16_t z1, int16_t x2, int16_t z2);
void Map_streamData(Map* self, uint8_t* buffer, uint8_t orientation, int8_t x, int8_t z, int8_t length);
void Map_updateHorizontalSlice(Map* self, int8_t offsetZ);
void Map_updateVerticalSlice(Map* self, int8_t offsetX);
void Map_updateEntireBuffer(Map* self);
void Map_updateDoors(Map* self);
uint8_t Map_streamIn(Map* self, uint8_t tile, uint8_t metadata, int8_t x, int8_t z);
void Map_streamInDoor(Map* self, uint8_t type, uint8_t metadata, int8_t x, int8_t z);

#endif
