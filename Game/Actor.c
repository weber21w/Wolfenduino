#include "Engine.h"
#include "Actor.h"
#include "Sounds.h"

#include "../DataHeaders/Data_Guard.h"
#include "../DataHeaders/Data_Dog.h"
#include "../DataHeaders/Data_SS.h"
#if WOLF3D_ENABLE_BOSS
#include "../DataHeaders/Data_Boss.h"
#endif
#include "TileTypes.h"

#define WOLF3D_FULL_SIZE_SPRITE_SCALE_DIVISOR CELL_SIZE

static bool Actor_shouldRefreshLineOfSight(Actor* self)
{
#if WOLF3D_ACTOR_LOS_THROTTLE
	uint8_t slot = (uint8_t)(self - engine.actors);
	return ((engine.frameCount + slot) & WOLF3D_ACTOR_LOS_FRAME_MASK) == 0;
#else
	(void)self;
	return true;
#endif
}

static void Actor_refreshLineOfSight(Actor* self)
{
#if WOLF3D_ACTOR_LOS_THROTTLE
	if(Actor_shouldRefreshLineOfSight(self))
	{
		self->flags.seesPlayer = Map_isClearLine(&engine.map, self->x, self->z, engine.player.x, engine.player.z) ? 1 : 0;
	}
#else
	self->flags.seesPlayer = Map_isClearLine(&engine.map, self->x, self->z, engine.player.x, engine.player.z) ? 1 : 0;
#endif
}

static uint16_t Actor_bossHpForDifficulty(void)
{
	switch(engine.difficulty)
	{
	case Difficulty_Baby:
		return 850;
	case Difficulty_Easy:
		return 950;
	case Difficulty_Hard:
		return 1200;
	case Difficulty_Medium:
	default:
		return 1050;
	}
}


static uint8_t Actor_randomGuardDeathSound(void)
{
	switch(getRandomNumber() & 0x3)
	{
	case 0:
		return Sound_EnemyDeath2;
	case 1:
		return Sound_EnemyDeath3;
	default:
		return Sound_EnemyDeath;
	}
}
void Actor_init(Actor* self, uint8_t id, uint8_t actorType, int8_t cellX, int8_t cellZ){
	self->spawnId = id;
	self->type = actorType;
	self->state = ActorState_Idle;
	self->x = CELL_TO_WORLD(cellX) + CELL_SIZE / 2;
	self->z = CELL_TO_WORLD(cellZ) + CELL_SIZE / 2;
	self->targetCellX = cellX;
	self->targetCellZ = cellZ;
	switch(actorType)
	{
	case ActorType_Dog:
		self->hp = 1;
		break;
	case ActorType_SS:
		self->hp = 100;
		break;
	case ActorType_Boss:
#if WOLF3D_ENABLE_BOSS
		self->hp = (int16_t)Actor_bossHpForDifficulty();
#else
		self->hp = 25;
#endif
		break;
	default:
		self->hp = 25;
		break;
	}
	self->frame = 0;
	self->flags.persistent = (actorType == ActorType_Boss) ? 1 : 0;
	self->flags.frozen = 0;
	self->flags.alive = 1;
	self->flags.seesPlayer = 0;
}
void Actor_update(Actor* self){
	if(self->type == ActorType_Empty)
		return;

	Actor_updateFrozenState(self);

	if(self->flags.frozen)
		return;

	Actor_refreshLineOfSight(self);

	bool updateFrame = (engine.frameCount & 0x3) == 0;

	switch(self->state)
	{
	case ActorState_Idle:
		if(self->flags.seesPlayer)
		{
			if(self->type == ActorType_Dog)
			{
				Platform_playEnemySoundAt(Sound_DogBark, self->x, self->z);
			}
			else if(self->type == ActorType_Boss)
			{
				Platform_playEnemySoundAt(Sound_BossActive, self->x, self->z);
			}
			else if(self->type == ActorType_SS)
			{
				Platform_playEnemySoundAt(Sound_SSAlert, self->x, self->z);
			}
			else
			{
				Platform_playEnemySoundAt(Sound_GuardAlert, self->x, self->z);
			}
			Actor_switchState(self, ActorState_Active);
		}
		break;
	case ActorState_Active:
	{
		bool movementSucceeded = Actor_tryMove(self);
		if(movementSucceeded)
		{
			self->frame = (engine.frameCount >> 2) & 0x3;
			if(self->frame == 3)
				self->frame = 1;
		}
		else
		{
			self->frame = 1;
		}
		if(Actor_shouldShootPlayer(self, movementSucceeded))
		{
			Actor_switchState(self, ActorState_Aiming);
		}
	}
		break;
	case ActorState_Aiming:
		if(updateFrame)
		{
			Actor_switchState(self, ActorState_Shooting);
		}
		break;
	case ActorState_Shooting:
		if(updateFrame)
		{
			Actor_shootPlayer(self);
			Actor_switchState(self, ActorState_Recoiling);
		}
		break;
	case ActorState_Recoiling:
		if(updateFrame)
		{
			if(self->type == ActorType_Boss && self->flags.seesPlayer)
			{
				Actor_switchState(self, ActorState_Shooting);
			}
			else if(self->type == ActorType_SS && (getRandomNumber() & 0x3) && self->flags.seesPlayer)
			{
				Actor_switchState(self, ActorState_Shooting);
			}
			else
			{
				Actor_switchState(self, ActorState_Active);
			}
		}
		break;
	case ActorState_Injured:
		if(updateFrame)
		{
			Actor_switchState(self, ActorState_Active);
		}
		break;
	case ActorState_Dying:
		if(updateFrame)
		{
			self->frame++;
			if(self->frame == 9)
				Actor_switchState(self, ActorState_Dead);
		}
		break;
	case ActorState_Dead:
		break;
	default:
		self->frame = 0;
		break;
	}
}
void Actor_draw(Actor* self){
	switch(self->type)
	{
	case ActorType_Dog:
		Renderer_queueSpriteScaled(&engine.renderer, &Data_dogSprite_frames[self->frame], Data_dogSprite, self->x, self->z, WOLF3D_FULL_SIZE_SPRITE_SCALE_DIVISOR);
		break;
	case ActorType_SS:
		Renderer_queueSpriteScaled(&engine.renderer, &Data_ssSprite_frames[self->frame], Data_ssSprite, self->x, self->z, WOLF3D_FULL_SIZE_SPRITE_SCALE_DIVISOR);
		break;
	case ActorType_Boss:
#if WOLF3D_ENABLE_BOSS
		Renderer_queueSpriteScaled(&engine.renderer, &Data_bossSprite_frames[self->frame], Data_bossSprite, self->x, self->z, WOLF3D_FULL_SIZE_SPRITE_SCALE_DIVISOR);
#else
		Renderer_queueSprite(&engine.renderer, &Data_guardSprite_frames[self->frame], Data_guardSprite, self->x, self->z);
#endif
		break;
	case ActorType_Guard:
	default:
		Renderer_queueSprite(&engine.renderer, &Data_guardSprite_frames[self->frame], Data_guardSprite, self->x, self->z);
		break;
	}
}
void Actor_updateFrozenState(Actor* self){
	int cellX = WORLD_TO_CELL(self->x);
	int cellZ = WORLD_TO_CELL(self->z);

	self->flags.frozen = cellX < engine.map.bufferX || cellZ < engine.map.bufferZ || cellX >= engine.map.bufferX + MAP_BUFFER_SIZE || cellZ >= engine.map.bufferZ + MAP_BUFFER_SIZE;
}
void Actor_damage(Actor* self, int amount){
	if(self->hp == 0)
		return;

	if(amount > self->hp)
	{
		self->hp = 0;
	}
	else self->hp -= amount;

	if(self->hp == 0)
	{
		Actor_switchState(self, ActorState_Dying);
		Map_markActorKilled(&engine.map, self->spawnId);
		if(self->type == ActorType_Dog)
		{
			Platform_playEnemySoundAt(Sound_DogDeath, self->x, self->z);
		}
		else if(self->type == ActorType_SS)
		{
			Platform_playEnemySoundAt(Sound_SSDeath, self->x, self->z);
			Actor_dropItem(self, Tile_Item_MachineGun);
		}
		else if(self->type == ActorType_Boss)
		{
			Platform_playEnemySoundAt(Sound_BossDeath, self->x, self->z);
			Actor_dropItem(self, Tile_Item_Key1);
		}
		else
		{
			Platform_playEnemySoundAt(Actor_randomGuardDeathSound(), self->x, self->z);
			Actor_dropItem(self, Tile_Item_Clip);
		}
	}
	else
	{
		Platform_playEnemySoundAt(Sound_HitEnemy, self->x, self->z);
		if(self->type != ActorType_Dog && self->type != ActorType_Boss)
		{
			Actor_switchState(self, ActorState_Injured);
		}
	}
}
void Actor_switchState(Actor* self, uint8_t newState){
	self->state = newState;

	switch(newState)
	{
	case ActorState_Injured:
		self->frame = 5;
		break;
	case ActorState_Dying:
		self->frame = 5;
		break;
	case ActorState_Dead:
		self->frame = 9;
		break;
	case ActorState_Aiming:
	case ActorState_Recoiling:
		self->frame = 3;
		break;
	case ActorState_Shooting:
		self->frame = 4;
		if(self->type == ActorType_Dog)
		{
			Platform_playEnemySoundAt(Sound_DogAttack, self->x, self->z);
		}
		else if(self->type == ActorType_SS)
		{
			Platform_playEnemySoundAt(Sound_SSAttack, self->x, self->z);
		}
		else if(self->type == ActorType_Boss)
		{
			Platform_playEnemySoundAt(Sound_BossAttack, self->x, self->z);
		}
		else
		{
			Platform_playEnemySoundAt(Sound_GuardAttack, self->x, self->z);
		}
		break;
	default:
		break;
	}
}
void Actor_dropItem(Actor* self, uint8_t itemType){
	int cellX = WORLD_TO_CELL(self->x);
	int cellZ = WORLD_TO_CELL(self->z);

	if(Actor_tryDropItem(self, itemType, cellX, cellZ))
		return;

	for(int i = cellX - 1; i < cellX + 1; i++)
	{
		for(int j = cellZ - 1; j < cellZ + 1; j++)
		{
			if(Actor_tryDropItem(self, itemType, i, j))
				return;
		}
	}
}
bool Actor_tryDropItem(Actor* self, uint8_t itemType, int cellX, int cellZ){
	uint8_t tile = Map_getTile(&engine.map, cellX, cellZ);
	if(tile == 0)
	{
		Map_placeItem(&engine.map, itemType, cellX, cellZ, DYNAMIC_ITEM_ID);
		return true;
	}
	return false;
}
bool Actor_isPlayerColliding(Actor* self){
	if(self->x >= engine.player.x - MIN_ACTOR_DISTANCE && self->x <= engine.player.x + MIN_ACTOR_DISTANCE
	&& self->z >= engine.player.z - MIN_ACTOR_DISTANCE && self->z <= engine.player.z + MIN_ACTOR_DISTANCE)
	{
		return true;
	}
	return false;
}
bool Actor_tryMove(Actor* self){
	int movement = (self->type == ActorType_Dog) ? 2 : 1;

	if(Map_isBlocked(&engine.map, self->targetCellX, self->targetCellZ))
	{
		if(self->type != ActorType_Dog)
		{
			Map_openDoorsAt(&engine.map, self->targetCellX, self->targetCellZ, Direction_None);
		}
		return false;
	}

	int16_t targetX = CELL_TO_WORLD(self->targetCellX) + CELL_SIZE / 2;
	int16_t targetZ = CELL_TO_WORLD(self->targetCellZ) + CELL_SIZE / 2;

	int8_t deltaX = clamp(targetX - self->x, -movement, movement);
	int8_t deltaZ = clamp(targetZ - self->z, -movement, movement);

	self->x += deltaX;
	self->z += deltaZ;

	if(Actor_isPlayerColliding(self))
	{
		self->x -= deltaX;
		self->z -= deltaZ;
		return false;
	}

	if(self->x == targetX && self->z == targetZ)
	{
		Actor_pickNewTargetCell(self);
	}
	return true;
}
bool Actor_tryPickCell(Actor* self, int8_t newX, int8_t newZ){
	if(Map_isBlocked(&engine.map, newX, newZ) && !Map_isDoor(&engine.map, newX, newZ))
		return false;
	if(Map_isBlocked(&engine.map, self->targetCellX, newZ) && !Map_isDoor(&engine.map, self->targetCellX, newZ))
		return false;
	if(Map_isBlocked(&engine.map, newX, self->targetCellZ) && !Map_isDoor(&engine.map, newX, self->targetCellZ))
		return false;

	for(int n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		if(self != &engine.actors[n] && engine.actors[n].type != ActorType_Empty && engine.actors[n].hp > 0)
		{
			if(engine.actors[n].targetCellX == newX && engine.actors[n].targetCellZ == newZ)
				return false;
		}
	}

	self->targetCellX = newX;
	self->targetCellZ = newZ;

	return true;
}
bool Actor_tryPickCells(Actor* self, int8_t deltaX, int8_t deltaZ){
	return Actor_tryPickCell(self, self->targetCellX + deltaX, self->targetCellZ + deltaZ)
		|| Actor_tryPickCell(self, self->targetCellX + deltaX, self->targetCellZ)
		|| Actor_tryPickCell(self, self->targetCellX, self->targetCellZ + deltaZ)
		|| Actor_tryPickCell(self, self->targetCellX - deltaX, self->targetCellZ + deltaZ)
		|| Actor_tryPickCell(self, self->targetCellX + deltaX, self->targetCellZ - deltaZ);
}
void Actor_pickNewTargetCell(Actor* self){
	int8_t deltaX = clamp(WORLD_TO_CELL(engine.player.x) - self->targetCellX, -1, 1);
	int8_t deltaZ = clamp(WORLD_TO_CELL(engine.player.z) - self->targetCellZ, -1, 1);
	uint8_t dodgeChance = getRandomNumber();

	if(deltaX == 0)
	{
		if(dodgeChance < 64)
		{
			deltaX = -1;
		}
		else if(dodgeChance < 128)
		{
			deltaX = 1;
		}
	}
	else if(deltaZ == 0)
	{
		if(dodgeChance < 64)
		{
			deltaZ = -1;
		}
		else if(dodgeChance < 128)
		{
			deltaZ = 1;
		}
	}

	Actor_tryPickCells(self, deltaX, deltaZ);
}
int8_t Actor_getPlayerCellDistance(Actor* self){
	int8_t dx = WORLD_TO_CELL(mabs(engine.player.x - self->x));
	int8_t dz = WORLD_TO_CELL(mabs(engine.player.z - self->z));
	return max(dx, dz);
}
bool Actor_shouldShootPlayer(Actor* self, bool movementSucceeded){
	if(self->flags.seesPlayer)
	{
		int8_t dist = Actor_getPlayerCellDistance(self);
		if(self->type == ActorType_Dog)
		{
			return dist == 1 && !movementSucceeded;
		}

		int chance = 16 / max(dist, 1);
		if(self->type == ActorType_Boss)
		{
			chance = 64 / max(dist, 1);
		}

		return getRandomNumber() < chance;
	}
	return false;
}
void Actor_shootPlayer(Actor* self){
	if(self->flags.seesPlayer)
	{
		int8_t dist = Actor_getPlayerCellDistance(self);
		int hitchance = 256 - dist * 16;

		if(self->type == ActorType_Dog && dist > 1)
		{
			return;
		}

		if(getRandomNumber() < hitchance)
		{
			uint8_t damage;

			if (dist < 2)
				damage = getRandomNumber()>>2;
			else if (dist<4)
				damage = getRandomNumber()>>3;
			else
				damage = getRandomNumber()>>4;

			if(self->type == ActorType_Boss)
			{
				damage = (uint8_t)(damage + (getRandomNumber() >> 4));
			}

			if(damage > 0)
			{
				Player_damage(&engine.player, damage);
				if(engine.player.hp == 0)
				{
					for(int8_t id = 0; id < MAX_ACTIVE_ACTORS; id++)
					{
						if(self == &engine.actors[id])
						{
							engine.player.killer = id;
							break;
						}
					}
				}
			}
		}
	}
}
