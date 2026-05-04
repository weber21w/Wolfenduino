#ifndef ACTOR_H_
#define ACTOR_H_

#include "Defines.h"

enum ActorType
{
	ActorType_Empty,
	ActorType_Guard,
	ActorType_Dog,
	ActorType_SS,
	ActorType_Boss
};

enum ActorState
{
	ActorState_Idle,
	ActorState_Active,
	ActorState_Injured,
	ActorState_Aiming,
	ActorState_Shooting,
	ActorState_Recoiling,
	ActorState_Dying,
	ActorState_Dead
};

typedef struct Actor
{
	uint8_t spawnId;
	uint8_t type;
	int16_t x, z;
	uint8_t state;
	uint8_t frame;
	int16_t hp;
	uint8_t targetCellX, targetCellZ;
	struct
	{
		uint8_t persistent : 1;
		uint8_t frozen : 1;
		uint8_t alive : 1;
		uint8_t seesPlayer : 1;
	} flags;
} Actor;

void Actor_init(Actor* self, uint8_t spawnId, uint8_t actorType, int8_t cellX, int8_t cellZ);
void Actor_update(Actor* self);
void Actor_draw(Actor* self);
void Actor_damage(Actor* self, int amount);
void Actor_switchState(Actor* self, uint8_t newState);
void Actor_updateFrozenState(Actor* self);
bool Actor_tryMove(Actor* self);
void Actor_pickNewTargetCell(Actor* self);
bool Actor_tryPickCell(Actor* self, int8_t x, int8_t z);
bool Actor_tryPickCells(Actor* self, int8_t deltaX, int8_t deltaZ);
void Actor_dropItem(Actor* self, uint8_t itemType);
bool Actor_tryDropItem(Actor* self, uint8_t itemType, int cellX, int cellZ);
bool Actor_isPlayerColliding(Actor* self);
void Actor_shootPlayer(Actor* self);
bool Actor_shouldShootPlayer(Actor* self, bool movementSucceeded);
int8_t Actor_getPlayerCellDistance(Actor* self);

#endif
