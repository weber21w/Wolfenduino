#ifndef PLAYER_H_
#define PLAYER_H_

#include "FixedMath.h"

enum WeaponType
{
	WeaponType_Knife,
	WeaponType_Pistol,
	WeaponType_MachineGun,
	WeaponType_ChainGun
};

typedef struct Player
{
	int16_t x, z;
	angle_t direction;
	uint8_t hp;
	uint8_t lives;
	uint8_t killer;
	uint8_t ticksSinceStrafePressed;
	uint8_t weaponCycleHeld;
	union
	{
		struct
		{
			uint8_t hasMachineGun : 1;
			uint8_t hasChainGun : 1;
			uint8_t hasKey1 : 1;
			uint8_t hasKey2 : 1;
		} inventory;
		uint8_t inventoryFlags;
	};
	struct
	{
		uint8_t type;
		uint8_t ammo;
		uint8_t frame;
		uint8_t time;
		uint8_t debounce : 1;
		uint8_t shooting : 1;
	} weapon;
} Player;

void Player_construct(Player* self);
void Player_init(Player* self);
void Player_update(Player* self);
void Player_move(Player* self, int16_t deltaX, int16_t deltaZ);
void Player_damage(Player* self, uint8_t amount);
void Player_updateWeapon(Player* self);
void Player_shootWeapon(Player* self);
bool Player_isPlayerColliding(Player* self);
bool Player_isPointColliding(Player* self, int16_t x, int16_t z);

#endif
