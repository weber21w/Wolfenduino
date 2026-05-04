#include "Engine.h"
#include "Map.h"
#include "TileTypes.h"
#include "Sounds.h"


#ifdef PROGMEM_MAP_STREAMING
#include "../DataHeaders/Data_Maps.h"
#endif
#ifdef PETIT_FATFS_FILE_STREAMING
#include <petit_fatfs.h>
#endif

const int8_t PushWallDirections[] PROGMEM =
{
	0, -1,		// North
	1, 0,		// East
	0, 1,		// South
	-1, 0		// West
};

#ifdef PROGMEM_MAP_STREAMING
static uint8_t Map_readRawTileProgmem(Map* self, int8_t x, int8_t z)
{
	if(x < 0 || z < 0 || x >= MAP_SIZE || z >= MAP_SIZE)
	{
		return MAP_OUT_OF_BOUNDS;
	}

#if WOLF3D_COMPRESSED_MAP_DATA
	uint16_t rowIndex = ((uint16_t)(uint8_t)self->currentLevel * MAP_SIZE) + (uint8_t)z;
	uint16_t entry = pgm_read_word(&mapDataRowOffsets[rowIndex]);
	uint16_t end = pgm_read_word(&mapDataRowOffsets[rowIndex + 1]);
	uint8_t remaining = (uint8_t)x;

	while(entry < end)
	{
		uint16_t offset = entry << 1;
		uint8_t run = pgm_read_byte(&mapDataRle[offset]);
		if(remaining < run)
		{
			return pgm_read_byte(&mapDataRle[offset + 1]);
		}
		remaining -= run;
		entry++;
	}

	return MAP_OUT_OF_BOUNDS;
#else
	return pgm_read_byte(&mapData[(uint16_t)(uint8_t)z * MAP_SIZE + (uint8_t)x]);
#endif
}

static uint8_t Map_readRawMeta(Map* self, int8_t x, int8_t z)
{
	if(x < 0 || z < 0 || x >= MAP_SIZE || z >= MAP_SIZE)
	{
		return 0;
	}

#if WOLF3D_COMPRESSED_MAP_META
	uint16_t rowIndex = ((uint16_t)(uint8_t)self->currentLevel * MAP_SIZE) + (uint8_t)z;
	uint16_t start = pgm_read_word(&mapMetaRowOffsets[rowIndex]);
	uint16_t end = pgm_read_word(&mapMetaRowOffsets[rowIndex + 1]);

	while(start < end)
	{
		uint16_t entryOffset = (uint16_t)start << 1;
		uint8_t entryX = pgm_read_byte(&mapMetaSparse[entryOffset]);

		if(entryX == (uint8_t)x)
		{
			return pgm_read_byte(&mapMetaSparse[entryOffset + 1]);
		}

		start++;
	}

	return 0;
#else
	return pgm_read_byte(&mapMeta[(uint16_t)(uint8_t)z * MAP_SIZE + (uint8_t)x]);
#endif
}
#endif
bool Map_isValid(Map* self, int8_t x, int8_t z){
	if(x < self->bufferX || z < self->bufferZ || x >= self->bufferX + MAP_BUFFER_SIZE || z >= self->bufferZ + MAP_BUFFER_SIZE)
	{
		return false;
	}

	return true;
/*  if (cellX < 0)
    return false;
  if (cellX >= MAP_SIZE)
    return false;
  if (cellZ < 0)
    return false;
  if (cellZ >= MAP_SIZE)
    return false;
  return true;*/
}
void Map_init(Map* self){
#ifdef PROGMEM_MAP_STREAMING
	self->m_mapLoaded = true;
#endif

#ifdef STANDARD_FILE_STREAMING
	while(!self->m_mapLoaded)
	{
		fopen_s(&m_mapStream, "wolf3d.dat", "rb");
		self->m_mapLoaded = true;
	}
#endif
#ifdef PETIT_FATFS_FILE_STREAMING
	while(!self->m_mapLoaded)
	{
		if(pf_mount(&m_fileSystem) == FR_OK)
		{
			if(pf_open("WOLF3D.DAT") == FR_OK)
			{
				self->m_mapLoaded = true;
			}
			else ERROR(PSTR("NO WOLF3D.DAT FOUND!"));
		}
		else ERROR(PSTR("SD CARD MOUNT ERROR"));
	}
#endif

	for(int n = 0; n < 256 / 8; n++)
	{
		self->m_itemState[n] = 0;
		self->m_actorState[n] = 0;
	}

	for(int n = 0; n < MAX_DOORS; n++)
	{
		self->doors[n].type = DoorType_None;
	}

	for(int n = 0; n < MAX_ACTIVE_ITEMS; n++)
	{
		self->items[n].type = 0;
	}

#if WOLF3D_SEEN_WALL_BITSET
	Map_clearSeenWalls(self);
#endif
#if WOLF3D_FULL_MAP_RAM_CACHE
	Map_buildFullMapCache(self);
#endif
}
void Map_initStreaming(Map* self){
	self->m_mapLoaded = false;
	self->bufferX = 0;
	self->bufferZ = 0;
}
void Map_update(Map* self){
	Map_updateDoors(self);
}
bool Map_isDoor(Map* self, int8_t cellX, int8_t cellZ){
	return Map_tileIsDoor(Map_getTile(self, cellX, cellZ));
}
bool Map_isBlocked(Map* self, int8_t cellX, int8_t cellZ){
	uint8_t tile = Map_getTile(self, cellX, cellZ);

	// Check if self is a wall
	if((tile >= Tile_FirstWall && tile <= Tile_LastWall))
		return true;

	// Check if self is a blocking decoration
	if((tile >= Tile_FirstBlockingDecoration && tile <= Tile_LastBlockingDecoration))
		return true;

	// Check if the door is closed
	for(int8_t n = 0; n < MAX_DOORS; n++)
	{
		if(self->doors[n].type != DoorType_None && self->doors[n].x == cellX && self->doors[n].z == cellZ && (self->doors[n].type == DoorType_SecretPushWall || self->doors[n].open < 16))
		{
			return true;
		}
	}

	return false;
}
bool Map_isSolid(Map* self, int8_t cellX, int8_t cellZ){
	return Map_tileIsWall(Map_getTile(self, cellX, cellZ));
}
uint8_t Map_getTextureId(Map* self, int8_t cellX, int8_t cellZ){
	uint8_t tile = Map_getTile(self, cellX, cellZ);
	if(tile == MAP_OUT_OF_BOUNDS)
		return 0;
	return tile - 1;
}
uint8_t Map_getTile(Map* self, int8_t x, int8_t z){
	if(x < self->bufferX || z < self->bufferZ || x >= self->bufferX + MAP_BUFFER_SIZE || z >= self->bufferZ + MAP_BUFFER_SIZE)
	{
		return MAP_OUT_OF_BOUNDS;
	}
	
	return Map_getTileFast(self, x, z);
}
void Map_streamData(Map* self, uint8_t* buffer, uint8_t orientation, int8_t x, int8_t z, int8_t length){
#ifdef STANDARD_FILE_STREAMING
	if(m_mapStream)
	{
		int32_t offset = orientation == MapRead_Horizontal ? (z * MAP_SIZE + x) * 2 : (MAP_SIZE * MAP_SIZE * 2) + (x * MAP_SIZE + z) * 2;
		offset += self->currentLevel * MAP_SIZE * MAP_SIZE * 4;
		fseek(m_mapStream, offset, SEEK_SET);
		fread(buffer, 1, length * 2, m_mapStream);
		return;
	}
#endif
#ifdef PETIT_FATFS_FILE_STREAMING
	if(self->m_mapLoaded)
	{
		int16_t _x = x;
		int16_t _z = z;
		int32_t offset = orientation == MapRead_Horizontal ? (_z * MAP_SIZE + _x) * 2 : (MAP_SIZE * MAP_SIZE * 2) + (_x * MAP_SIZE + _z) * 2;
		WORD bytesRead;
		int errorCount = 0;

		do
		{
			if(pf_lseek(offset) == FR_OK)
			{
				if(pf_read(buffer, length * 2, &bytesRead) != FR_OK)
				{
					bytesRead = 0;
				}
			}
			errorCount ++;
			if(errorCount > 3)
			{
				ERROR(PSTR("ERROR READING SD CARD"));
			}
		}
		while(bytesRead < length * 2);
	}
#endif
	//printf("Streaming %s, %d, %d\n", orientation == MapRead_Horizontal ? "Horizontal" : "Vertical", x, z, length);
	// TODO: make self stream from SD card or decompress from huffman stream in progmem

#ifdef PROGMEM_MAP_STREAMING
	(void)self;
	if(orientation == MapRead_Horizontal)
	{
		for(int8_t n = 0; n < length; n++)
		{
			buffer[n * 2] = Map_readRawTileProgmem(self, (int8_t)(x + n), z);
			buffer[n * 2 + 1] = Map_readRawMeta(self, (int8_t)(x + n), z);
		}
	}
	else
	{
		for(int8_t n = 0; n < length; n++)
		{
			buffer[n * 2] = Map_readRawTileProgmem(self, x, (int8_t)(z + n));
			buffer[n * 2 + 1] = Map_readRawMeta(self, x, (int8_t)(z + n));
		}
	}
#endif
}

uint8_t Map_readRawTile(Map* self, int8_t x, int8_t z){
	if(x < 0 || z < 0 || x >= MAP_SIZE || z >= MAP_SIZE)
	{
		return MAP_OUT_OF_BOUNDS;
	}
#if WOLF3D_FULL_MAP_RAM_CACHE
	return self->m_fullMap[(uint16_t)(uint8_t)z * MAP_SIZE + (uint8_t)x];
#else
#ifdef PROGMEM_MAP_STREAMING
	return Map_readRawTileProgmem(self, x, z);
#else
	uint8_t pair[2];
	Map_streamData(self, pair, MapRead_Horizontal, x, z, 1);
	return pair[0];
#endif
#endif
}

#if WOLF3D_SEEN_WALL_BITSET
static uint16_t Map_seenWallBitIndex(int8_t x, int8_t z)
{
	uint8_t sx = (uint8_t)x / WOLF3D_SEEN_WALL_GRANULARITY;
	uint8_t sz = (uint8_t)z / WOLF3D_SEEN_WALL_GRANULARITY;
	return (uint16_t)sz * WOLF3D_SEEN_WALL_DIM + sx;
}

void Map_clearSeenWalls(Map* self)
{
	memset(self->m_seenWalls, 0, sizeof(self->m_seenWalls));
}

void Map_markSeenWall(Map* self, int8_t x, int8_t z)
{
	if(x < 0 || z < 0 || x >= MAP_SIZE || z >= MAP_SIZE)
	{
		return;
	}
	uint16_t bitIndex = Map_seenWallBitIndex(x, z);
	uint16_t byteIndex = bitIndex >> 3;
	uint8_t mask = (uint8_t)(1u << (bitIndex & 7));
	self->m_seenWalls[byteIndex] |= mask;
}

bool Map_isWallSeen(Map* self, int8_t x, int8_t z)
{
	if(x < 0 || z < 0 || x >= MAP_SIZE || z >= MAP_SIZE)
	{
		return false;
	}
	uint16_t bitIndex = Map_seenWallBitIndex(x, z);
	uint16_t byteIndex = bitIndex >> 3;
	uint8_t mask = (uint8_t)(1u << (bitIndex & 7));
	return (self->m_seenWalls[byteIndex] & mask) != 0;
}
#endif

#if WOLF3D_FULL_MAP_RAM_CACHE
void Map_buildFullMapCache(Map* self)
{
	for(uint8_t z = 0; z < MAP_SIZE; z++)
	{
		for(uint8_t x = 0; x < MAP_SIZE; x++)
		{
#ifdef PROGMEM_MAP_STREAMING
			self->m_fullMap[(uint16_t)z * MAP_SIZE + x] = Map_readRawTileProgmem(self, (int8_t)x, (int8_t)z);
#else
			uint8_t pair[2];
			Map_streamData(self, pair, MapRead_Horizontal, x, z, 1);
			self->m_fullMap[(uint16_t)z * MAP_SIZE + x] = pair[0];
#endif
		}
	}
}
#endif
uint8_t Map_streamIn(Map* self, uint8_t tile, uint8_t metadata, int8_t x, int8_t z){
	if(tile >= Tile_FirstDoor && tile <= Tile_LastDoor)
	{
		uint8_t textureId = 18;
		if(tile == Tile_Door_Elevator_Horizontal || tile == Tile_Door_Elevator_Vertical)
			textureId = 12;
		Map_streamInDoor(self, tile - Tile_FirstDoor + 1, textureId, x, z);
	}
	else if(tile >= Tile_FirstItem && tile <= Tile_LastItem)
	{
		if(!Map_isItemCollected(self, metadata))
		{
			Map_placeItem(self, tile, x, z, metadata);
		}
		return Tile_Empty;
	}
	else if(tile >= Tile_FirstActor && tile <= Tile_LastActor)
	{
		if(!Map_isActorKilled(self, metadata))
		{
			uint8_t actorType = ActorType_Empty;
			bool spawnActor = false;

			switch(tile)
			{
			case Tile_Actor_Guard_Easy:
				spawnActor = true;
				actorType = ActorType_Guard;
				break;
			case Tile_Actor_Guard_Medium:
				spawnActor = engine.difficulty >= Difficulty_Medium;
				actorType = ActorType_Guard;
				break;
			case Tile_Actor_Guard_Hard:
				spawnActor = engine.difficulty >= Difficulty_Hard;
				actorType = ActorType_Guard;
				break;
			case Tile_Actor_SS_Easy:
				spawnActor = true;
				actorType = ActorType_SS;
				break;
			case Tile_Actor_SS_Medium:
				spawnActor = engine.difficulty >= Difficulty_Medium;
				actorType = ActorType_SS;
				break;
			case Tile_Actor_SS_Hard:
				spawnActor = engine.difficulty >= Difficulty_Hard;
				actorType = ActorType_SS;
				break;
			case Tile_Actor_Dog_Easy:
				spawnActor = true;
				actorType = ActorType_Dog;
				break;
			case Tile_Actor_Dog_Medium:
				spawnActor = engine.difficulty >= Difficulty_Medium;
				actorType = ActorType_Dog;
				break;
			case Tile_Actor_Dog_Hard:
				spawnActor = engine.difficulty >= Difficulty_Hard;
				actorType = ActorType_Dog;
				break;
			case Tile_Actor_Boss:
#if WOLF3D_ENABLE_BOSS
				spawnActor = true;
				actorType = ActorType_Boss;
#endif
				break;
			default:
				break;
			}

			if(spawnActor)
			{
				Engine_spawnActor(metadata, actorType, x, z);
			}
		}
		return Tile_Empty;
	}
	else if(tile == Tile_SecretPushWall)
	{
		if(engine.gameState == GameState_Loading)
		{
			Map_streamInDoor(self, DoorType_SecretPushWall, metadata, x, z);
		}
		return Tile_Empty;
	}

	return tile;
}
void Map_updateHorizontalSlice(Map* self, int8_t offsetZ){
	Map_streamData(self, self->m_streamBuffer, MapRead_Horizontal, self->bufferX, self->bufferZ + offsetZ, MAP_BUFFER_SIZE);

	int8_t targetZ	= (self->bufferZ + offsetZ) & 0xf;

	for(int8_t x = 0; x < MAP_BUFFER_SIZE; x++)
	{
		int8_t targetX = (self->bufferX + x) & 0xf;
		uint8_t read = Map_streamIn(self, self->m_streamBuffer[x * 2], self->m_streamBuffer[x * 2 + 1], self->bufferX + x, self->bufferZ + offsetZ);

		self->m_mapBuffer[targetZ * MAP_BUFFER_SIZE + targetX] = read;
	}
}
void Map_updateVerticalSlice(Map* self, int8_t offsetX){
	Map_streamData(self, self->m_streamBuffer, MapRead_Vertical, self->bufferX + offsetX, self->bufferZ, MAP_BUFFER_SIZE);

	int8_t targetX = (self->bufferX + offsetX) & 0xf;

	for(int8_t z = 0; z < MAP_BUFFER_SIZE; z++)
	{
		int8_t targetZ = (self->bufferZ + z) & 0xf;
		uint8_t read = Map_streamIn(self, self->m_streamBuffer[z * 2], self->m_streamBuffer[z * 2 + 1], self->bufferX + offsetX, self->bufferZ + z);

		self->m_mapBuffer[targetZ * MAP_BUFFER_SIZE + targetX] = read;
	}
}
void Map_updateEntireBuffer(Map* self){
	for(int8_t n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		if(engine.actors[n].type != ActorType_Empty)
		{
			Actor_updateFrozenState(&engine.actors[n]);
		}
	}

	for(int8_t n = 0; n < MAP_BUFFER_SIZE; n++)
	{
		Map_updateHorizontalSlice(self, n);
	}
}
void Map_updateBufferPosition(Map* self, int8_t newX, int8_t newZ){
	if(newX < 0)
		newX = 0;
	if(newZ < 0)
		newZ = 0;
	if(newX > MAP_SIZE - MAP_BUFFER_SIZE)
		newX = MAP_SIZE - MAP_BUFFER_SIZE;
	if(newZ > MAP_SIZE - MAP_BUFFER_SIZE)
		newZ = MAP_SIZE - MAP_BUFFER_SIZE;
	
	if(engine.gameState == GameState_Loading || newX <= self->bufferX - MAP_BUFFER_SIZE || newX >= self->bufferX + MAP_BUFFER_SIZE
	|| newZ <= self->bufferZ - MAP_BUFFER_SIZE || newZ >= self->bufferZ + MAP_BUFFER_SIZE)
	{
		self->bufferX = newX;
		self->bufferZ = newZ;
		WARNING("Updating entire buffer at %d %d\n", self->bufferX, self->bufferZ);
		Map_updateEntireBuffer(self);
		return;
	}

	if(self->bufferX == newX && self->bufferZ == newZ)
		return;

	while(self->bufferX < newX)
	{
		self->bufferX ++;
		Map_updateVerticalSlice(self, MAP_BUFFER_SIZE - 1);
	}
	while(self->bufferX > newX)
	{
		self->bufferX --;
		Map_updateVerticalSlice(self, 0);
	}
	while(self->bufferZ < newZ)
	{
		self->bufferZ ++;
		Map_updateHorizontalSlice(self, MAP_BUFFER_SIZE - 1);
	}
	while(self->bufferZ > newZ)
	{
		self->bufferZ --;
		Map_updateHorizontalSlice(self, 0);
	}
}
void Map_updateDoors(Map* self){
	for(int8_t n = 0; n < MAX_DOORS; n++)
	{
		if(self->doors[n].type != DoorType_None)
		{
			Door_update(&self->doors[n]);
		}
	}
}
void Map_streamInDoor(Map* self, uint8_t type, uint8_t metadata, int8_t x, int8_t z){
	int8_t freeIndex = -1;

	if(type == DoorType_SecretPushWall)
		WARNING("Creating secret push wall at %d %d!\n", x, z);

	for(int8_t n = 0; n < MAX_DOORS; n++)
	{
		if(freeIndex == -1 && self->doors[n].type == DoorType_None)
		{
			freeIndex = n;
		}
		if(self->doors[n].type != DoorType_None && self->doors[n].x == x && self->doors[n].z == z)
		{
			// Already streamed in
			return;
		}
	}

	if(freeIndex == -1)
	{
		for(int8_t n = 0; n < MAX_DOORS; n++)
		{
			if(self->doors[n].type != DoorType_SecretPushWall && !Map_isValid(self, self->doors[n].x, self->doors[n].z))
			{
				freeIndex = n;
				break;
			}
		}
	}

	if(freeIndex == -1)
	{
		for(int8_t n = 0; n < MAX_DOORS; n++)
		{
			if(self->doors[n].type != DoorType_SecretPushWall && Renderer_isFrustrumClipped(&engine.renderer, self->doors[n].x, self->doors[n].z))
			{
				freeIndex = n;
				break;
			}
		}
	}

	if(freeIndex == -1)
	{
		WARNING("No room to spawn door!\n");
		return;
	}

	self->doors[freeIndex].x = x;
	self->doors[freeIndex].z = z;
	self->doors[freeIndex].open = 0;
	self->doors[freeIndex].state = DoorState_Idle;
	self->doors[freeIndex].type = type;
	self->doors[freeIndex].texture = metadata;
}
void Map_openDoorsAt(Map* self, int8_t x, int8_t z, int8_t direction){
//	if(!Map_isDoor(self, x, z))
		//return;
	if(direction != Direction_None)
	{
		if(Map_getTile(self, x, z) == Tile_ExitSwitchWall)
		{
			Platform_stopMusic();
			engine.gameState = GameState_FinishedLevel;
		}
	}

	for(int8_t n = 0; n < MAX_DOORS; n++)
	{
		if(self->doors[n].type != DoorType_None && self->doors[n].x == x && self->doors[n].z == z)
		{
			if(self->doors[n].type == DoorType_SecretPushWall)
			{
				if(direction != Direction_None)
				{
					int8_t offX = pgm_read_byte(&PushWallDirections[(direction) * 2]);
					int8_t offZ = pgm_read_byte(&PushWallDirections[(direction) * 2 + 1]);

					if(Map_isValid(&engine.map, x + offX, z + offZ) && !Map_isSolid(&engine.map, x + offX, z + offZ))
					{
						Platform_playSoundCell(Sound_PushWall, x, z);
						self->doors[n].state = DoorState_FirstPushWallState + direction;
					}
				}
			}
			else
			{
				if(self->doors[n].state != DoorState_Opening && self->doors[n].open == 0)
				{
					Platform_playSoundCell(Sound_OpenDoor, x, z);
				}
				self->doors[n].state = DoorState_Opening;
			}
			return;
		}
	}
}
void Door_update(Door* self){
	switch(self->state)
	{
	case DoorState_PushNorth:
	case DoorState_PushEast:
	case DoorState_PushSouth:
	case DoorState_PushWest:
		self->open++;
		if(self->open == CELL_SIZE)
		{
			self->open = 0;
			int8_t offX = pgm_read_byte(&PushWallDirections[(self->state - DoorState_FirstPushWallState) * 2]);
			int8_t offZ = pgm_read_byte(&PushWallDirections[(self->state - DoorState_FirstPushWallState) * 2 + 1]);

			self->x += offX;
			self->z += offZ;
			if(!Map_isValid(&engine.map, self->x + offX, self->z + offZ) || Map_isSolid(&engine.map, self->x + offX, self->z + offZ))
			{
				self->state = DoorState_Idle;
			}
		}
		break;
	case DoorState_Opening:
		if(self->open < DOOR_MAX_OPEN)
		{
			self->open ++;
		}
		else self->state = DoorState_Closing;
		break;
	case DoorState_Closing:
		if(self->open > 0)
		{
			self->open --;
			if(self->open == 16)
			{
				Platform_playSoundCell(Sound_CloseDoor, self->x, self->z);
			}
		}
		else self->state = DoorState_Idle;
		break;
	}
}

bool Map_isClearLine(Map* self, int16_t x1, int16_t z1, int16_t x2, int16_t z2)
/*{
	int cellX = x1 / CELL_SIZE;
	int cellZ = z1 / CELL_SIZE;
	int targetCellX = x2 / CELL_SIZE;
	int targetCellZ = z2 / CELL_SIZE;
	int16_t x = x1;
	int16_t z = z1;
	int16_t dx = x2 - x1;
	int16_t dz = z2 - z1;
	int16_t tx, tz;

	while(1)
	{
		if(cellX == targetCellX && cellZ == targetCellZ)
			return true;

		if(dx < 0)
		{
			tx = (x - cellX * CELL_SIZE) / -dx;
		}
		else if(dx > 0)
		{
			tx = ((cellX + 1) * CELL_SIZE - x) / dx;
		}
		else tx = 0;

		if(dz < 0)
		{
			tz = (z - cellZ * CELL_SIZE) / -dz;
		}
		else if(dz > 0)
		{
			tz = ((cellZ + 1) * CELL_SIZE - z) / dz;
		}
		else tz = 0;

	}

	return true;
}
*/
{
/*    int         x1,y1,xt1,yt1,x2,y2,xt2,yt2;
    int         x,y;
    int         xdist,ydist,xstep,ystep;
    int         partial,delta;
    int32_t     ltemp;
    int         xfrac,yfrac,deltafrac;
    unsigned    value,intercept;
	*/
	int cellX1 = WORLD_TO_CELL(x1);
	int cellX2 = WORLD_TO_CELL(x2);
	int cellZ1 = WORLD_TO_CELL(z1);
	int cellZ2 = WORLD_TO_CELL(z2);

    int xdist = mabs(cellX2 - cellX1);

	int partial, delta;
	int deltafrac;
	int xfrac, zfrac;
	int xstep, zstep;
	int32_t ltemp;
	int x, z;

    if (xdist > 0)
    {
        if (cellX2 > cellX1)
        {
            partial = (CELL_TO_WORLD(cellX1 + 1) - x1);
            xstep = 1;
        }
        else
        {
            partial = (x1 - CELL_TO_WORLD(cellX1));
            xstep = -1;
        }

        deltafrac = mabs(x2 - x1);
        delta = z2 - z1;
        ltemp = ((int32_t)delta * CELL_SIZE) / deltafrac;
        if (ltemp > 0x7fffl)
            zstep = 0x7fff;
        else if (ltemp < -0x7fffl)
            zstep = -0x7fff;
        else
            zstep = ltemp;
        zfrac = z1 + (((int32_t)zstep*partial) / CELL_SIZE);

        x = cellX1 + xstep;
        cellX2 += xstep;
        do
        {
            z = WORLD_TO_CELL(zfrac);
            zfrac += zstep;

            uint8_t tile = Map_getTile(self, x, z);
            x += xstep;

            if (!tile)
                continue;

            if (tile >= Tile_FirstWall && tile <= Tile_LastWall)
                return false;

            //
            // see if the door is open enough
            //
            /*value &= ~0x80;
            intercept = yfrac-ystep/2;

            if (intercept>doorposition[value])
                return false;*/

        } while (x != cellX2);
    }

    int zdist = mabs(cellZ2 - cellZ1);

    if (zdist > 0)
    {
        if (cellZ2 > cellZ1)
        {
            partial = (CELL_TO_WORLD(cellZ1 + 1) - z1);
            zstep = 1;
        }
        else
        {
            partial = (z1 - CELL_TO_WORLD(cellZ1));
            zstep = -1;
        }

        deltafrac = mabs(z2 - z1);
        delta = x2 - x1;
        ltemp = ((int32_t)delta * CELL_SIZE)/deltafrac;
        if (ltemp > 0x7fffl)
            xstep = 0x7fff;
        else if (ltemp < -0x7fffl)
            xstep = -0x7fff;
        else
            xstep = ltemp;
        xfrac = x1 + (((int32_t)xstep*partial) / CELL_SIZE);

        z = cellZ1 + zstep;
        cellZ2 += zstep;
        do
        {
            x = WORLD_TO_CELL(xfrac);
            xfrac += xstep;

            uint8_t tile = Map_getTile(self, x, z);
            z += zstep;

            if (!tile)
                continue;

            if (tile >= Tile_FirstWall && tile <= Tile_LastWall)
                return false;

            //
            // see if the door is open enough
            //
            /*value &= ~0x80;
            intercept = xfrac-xstep/2;

            if (intercept>doorposition[value])
                return false;*/
        } while (z != cellZ2);
    }

    return true;
}
bool Map_placeItem(Map* self, uint8_t type, int8_t x, int8_t z, uint8_t spawnId){
	int8_t slot = -1;

	for(int8_t n = 0; n < MAX_ACTIVE_ITEMS; n++)
	{
		if(self->items[n].type == 0)
		{
			slot = n;
		}
		else if(spawnId != DYNAMIC_ITEM_ID && self->items[n].spawnId == spawnId)
		{
			return false;
		}
	}

	if(slot == -1)
	{
		for(int8_t n = 0; n < MAX_ACTIVE_ITEMS; n++)
		{
			if(!Map_isValid(self, self->items[n].x, self->items[n].z))
			{
				slot = n;
				break;
			}
		}
	}

	if(slot == -1)
	{
		WARNING("No room to spawn item!\n");
		return false;
	}

	self->items[slot].type = type;
	self->items[slot].spawnId = spawnId;
	self->items[slot].x = x;
	self->items[slot].z = z;

	return true;
}
