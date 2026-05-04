#include "Engine.h"
#include "Player.h"
#include "TileTypes.h"
#include "FixedMath.h"
#include "Sounds.h"

void Player_construct(Player* self)
{
	(void)self;
}

static bool Player_canUseWeapon(Player* self, uint8_t weaponType)
{
	switch(weaponType)
	{
	case WeaponType_Knife:
		return true;
	case WeaponType_Pistol:
		return self->weapon.ammo > 0;
	case WeaponType_MachineGun:
		return self->inventory.hasMachineGun && self->weapon.ammo > 0;
	case WeaponType_ChainGun:
		return self->inventory.hasChainGun && self->weapon.ammo > 0;
	default:
		return false;
	}
}

static void Player_cycleWeapon(Player* self, int8_t direction)
{
	int8_t weaponType = (int8_t)self->weapon.type;

	for(uint8_t n = 0; n < 4; n++)
	{
		weaponType += direction;
		if(weaponType < WeaponType_Knife)
		{
			weaponType = WeaponType_ChainGun;
		}
		else if(weaponType > WeaponType_ChainGun)
		{
			weaponType = WeaponType_Knife;
		}

		if(Player_canUseWeapon(self, (uint8_t)weaponType))
		{
			self->weapon.type = (uint8_t)weaponType;
			self->weapon.frame = 0;
			self->weapon.time = 0;
			self->weapon.shooting = false;
			return;
		}
	}
}

static void Player_updateWeaponCycleInput(Player* self, uint16_t input)
{
	uint8_t cycleButtons = 0;

	if(input & Input_Btn_X)
	{
		cycleButtons |= 1;
	}
	if(input & Input_Btn_Y)
	{
		cycleButtons |= 2;
	}

	if(cycleButtons == 0)
	{
		self->weaponCycleHeld = 0;
		return;
	}

	if(self->weaponCycleHeld != 0)
	{
		return;
	}

	self->weaponCycleHeld = cycleButtons;

	if(cycleButtons & 2)
	{
		Player_cycleWeapon(self, 1);
	}
	else
	{
		Player_cycleWeapon(self, -1);
	}
}
void Player_update(Player* self){
	int16_t cos_dir = FixedMath_Cos(self->direction);
	int16_t sin_dir = FixedMath_Sin(self->direction);

	if(self->hp > 0)
	{
		uint16_t input = Platform_readInput();
		bool strafe = (input & Input_Btn_A) != 0;

		Player_updateWeaponCycleInput(self, input);

		if(input == Input_Btn_A)
		{
			if(!self->ticksSinceStrafePressed)
			{
				self->ticksSinceStrafePressed = 1;
			}
			else if(self->ticksSinceStrafePressed > 1)
			{
				self->ticksSinceStrafePressed = 0;
				if(self->weapon.type == WeaponType_Pistol)
				{
					if(self->inventory.hasMachineGun)
					{
						self->weapon.type = WeaponType_MachineGun;
					}
					else if(self->inventory.hasChainGun)
					{
						self->weapon.type = WeaponType_ChainGun;
					}
					else self->weapon.type = WeaponType_Knife;
				}
				else if(self->weapon.type == WeaponType_MachineGun)
				{
					if(self->inventory.hasChainGun)
					{
						self->weapon.type = WeaponType_ChainGun;
					}
					else self->weapon.type = WeaponType_Knife;
				}
				else if(self->weapon.type == WeaponType_ChainGun)
				{
					self->weapon.type = WeaponType_ChainGun;
				}
				else if(self->weapon.ammo > 0)
				{
					self->weapon.type = WeaponType_Pistol;
				}
			}
		}
		else if(!input)
		{
			if(self->ticksSinceStrafePressed > 0)
			{
				self->ticksSinceStrafePressed ++;
				if(self->ticksSinceStrafePressed > 5)
				{
					self->ticksSinceStrafePressed = 0;
				}
			}
		}
		else self->ticksSinceStrafePressed = 0;

		int16_t movement = MOVEMENT;
		int16_t turn = TURN;
		int16_t deltaX = 0, deltaZ = 0;
		bool strafeLeft = false;
		bool strafeRight = false;
    
		Player_updateWeapon(self);
    
		if (input & Input_Dpad_Down)
		{
			deltaX -= (movement * cos_dir) >> (FIXED_SHIFT);
			deltaZ -= (movement * sin_dir) >> (FIXED_SHIFT);
		}
    
		if (input & Input_Dpad_Up)
		{
			deltaX += (movement * cos_dir) >> (FIXED_SHIFT);
			deltaZ += (movement * sin_dir) >> (FIXED_SHIFT);
		}
    
		if (input & Input_Dpad_Left)
		{
			if (strafe)
			{
				strafeLeft = true;
			}
			else
				self->direction -= turn;
		}	
    
		if (input & Input_Dpad_Right)
		{
			if (strafe)
			{
				strafeRight = true;
			}
			else
				self->direction += turn;
		}

		/* Merge all strafe sources into one left/right intent. This keeps
		 * A+left/right and the Uzebox/SNES shoulder buttons from stacking
		 * into a double-speed strafe when both methods are held. */
		if (input & Input_Btn_SL)
		{
			strafeLeft = true;
		}

		if (input & Input_Btn_SR)
		{
			strafeRight = true;
		}

		if(strafeLeft && !strafeRight)
		{
			deltaX += (movement * sin_dir) >> (FIXED_SHIFT);
			deltaZ -= (movement * cos_dir) >> (FIXED_SHIFT);
		}
		else if(strafeRight && !strafeLeft)
		{
			deltaX -= (movement * sin_dir) >> (FIXED_SHIFT);
			deltaZ += (movement * cos_dir) >> (FIXED_SHIFT);
		}
  
		Player_move(self, deltaX, deltaZ);

		//int16_t projectedX = self->x / CELL_SIZE;
		//int16_t projectedZ = self->z / CELL_SIZE;

		// Check for doors
		int8_t cellX = WORLD_TO_CELL(self->x);
		int8_t cellZ = WORLD_TO_CELL(self->z);

		Map_openDoorsAt(&engine.map, cellX, cellZ, Direction_None);

		if(mabs(cos_dir) > mabs(sin_dir))
		{
			if(cos_dir > 0)
			{
				Map_openDoorsAt(&engine.map, cellX + 1, cellZ, Direction_East);
			}
			else
			{
				Map_openDoorsAt(&engine.map, cellX - 1, cellZ, Direction_West);
			}
		}
		else
		{
			if(sin_dir > 0)
			{
				Map_openDoorsAt(&engine.map, cellX, cellZ + 1, Direction_South);
			}
			else
			{
				Map_openDoorsAt(&engine.map, cellX, cellZ - 1, Direction_North);
			}
		}

		// Collect any items
		for(int8_t n = 0; n < MAX_ACTIVE_ITEMS; n++)
		{
			if(engine.map.items[n].type != 0 && engine.map.items[n].x == cellX && engine.map.items[n].z == cellZ)
			{
				bool collected = true;
				uint8_t pickupSound = Sound_CollectAmmo;

				// Collect self item
				switch(engine.map.items[n].type)
				{
				case Tile_Item_MachineGun:
					self->weapon.ammo = min(self->weapon.ammo + 8, 99);
					self->weapon.type = WeaponType_MachineGun;
					self->inventory.hasMachineGun = true;
					engine.renderer.hudAmmoFlash = WOLF3D_HUD_FLASH_FRAMES;
					pickupSound = Sound_CollectWeapon;
					break;
				case Tile_Item_Clip:
					if(self->weapon.ammo < 99)
					{
						if(self->weapon.ammo == 0 && self->weapon.type == WeaponType_Knife)
						{
							self->weapon.type = WeaponType_Pistol;
						}
						self->weapon.ammo = min(self->weapon.ammo + 8, 99);
						engine.renderer.hudAmmoFlash = WOLF3D_HUD_FLASH_FRAMES;
					}
					else collected = false;
					break;
				case Tile_Item_FirstAid:
					if(self->hp < 100)
					{
						self->hp = min(100, self->hp + 25);
						engine.renderer.hudHealthFlash = WOLF3D_HUD_FLASH_FRAMES;
						pickupSound = Sound_CollectHealth;
					}
					else collected = false;
					break;
				case Tile_Item_Food:
					if(self->hp < 100)
					{
						self->hp = min(100, self->hp + 10);
						engine.renderer.hudHealthFlash = WOLF3D_HUD_FLASH_FRAMES;
						pickupSound = Sound_CollectHealth;
					}
					else collected = false;
					break;
				default:
					collected = false;
					break;
				}
				if(collected)
				{
					Platform_playSound(pickupSound);
					engine.map.items[n].type = 0;
					Map_markItemCollected(&engine.map, engine.map.items[n].spawnId);
				}
			}
		}
	}
	else
	{
		/* Slow the death look-at-killer turn now that the renderer is fast. */
#if WOLF3D_SLOW_DEATH_TURN
		if((engine.frameCount & WOLF3D_DEATH_TURN_FRAME_MASK) == 0)
#endif
		{
			int16_t rotCos = FixedMath_Cos(-self->direction);
		int16_t rotSin = FixedMath_Sin(-self->direction);
		int16_t xt = (int16_t)(FIXED_TO_INT(rotSin * (int32_t)(engine.actors[self->killer].x - self->x)) + FIXED_TO_INT(rotCos * (int32_t)(engine.actors[self->killer].z - self->z)));

		if(xt > 0)
		{
			self->direction += TURN;
		}
		else
		{
			self->direction -= TURN;
		}
		rotCos = FixedMath_Cos(-self->direction);
		rotSin = FixedMath_Sin(-self->direction);

		int16_t newXt = (int16_t)(FIXED_TO_INT(rotSin * (int32_t)(engine.actors[self->killer].x - self->x)) + FIXED_TO_INT(rotCos * (int32_t)(engine.actors[self->killer].z - self->z)));
		if((xt < 0 && newXt >= 0) || (xt > 0 && newXt <= 0))
		{
			engine.gameState = GameState_Dead;
			engine.frameCount = 0;
		}
		}
		engine.renderer.damageIndicator = 5;
	}

	// Update the stream position
	int16_t projectedX = WORLD_TO_CELL(self->x) + cos_dir / 19;
	int16_t projectedZ = WORLD_TO_CELL(self->z) + sin_dir / 19;

	Map_updateBufferPosition(&engine.map, projectedX - MAP_BUFFER_SIZE / 2, projectedZ - MAP_BUFFER_SIZE / 2);
}

#ifdef USE_SIMPLE_COLLISIONS
bool Player_isPlayerColliding(Player* self){
	for(int8_t n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		Actor* actor = &engine.actors[n];
		if(actor->type != ActorType_Empty && actor->hp > 0 && Actor_isPlayerColliding(actor))
		{
			return true;
		}
	}

	return Player_isPointColliding(self, self->x - MIN_WALL_DISTANCE, self->z - MIN_WALL_DISTANCE)
		|| Player_isPointColliding(self, self->x + MIN_WALL_DISTANCE, self->z - MIN_WALL_DISTANCE)
		|| Player_isPointColliding(self, self->x + MIN_WALL_DISTANCE, self->z + MIN_WALL_DISTANCE)
		|| Player_isPointColliding(self, self->x - MIN_WALL_DISTANCE, self->z + MIN_WALL_DISTANCE);
}
bool Player_isPointColliding(Player* self, int16_t pointX, int16_t pointZ){
	int8_t cellX = WORLD_TO_CELL(pointX);
	int8_t cellZ = WORLD_TO_CELL(pointZ);

	return (Map_isBlocked(&engine.map, cellX, cellZ));
}
void Player_move(Player* self, int16_t deltaX, int16_t deltaZ){
	self->x += deltaX;
	self->z += deltaZ;

	if(Player_isPlayerColliding(self))
	{
		self->z -= deltaZ;
		if(Player_isPlayerColliding(self))
		{
			self->x -= deltaX;
			self->z += deltaZ;
			if(Player_isPlayerColliding(self))
			{
				self->z -= deltaZ;
			}
		}
	}
}

#else
void Player_move(Player* self, int16_t deltaX, int16_t deltaZ){
	for(int8_t n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		Actor* actor = &engine.actors[n];
		if(actor->type != ActorType_Empty && actor->hp > 0)
		{
			int16_t diffX = mabs((self->x + deltaX) - actor->x);
			int16_t diffZ = mabs((self->z + deltaZ) - actor->z);

			if(diffX < MIN_ACTOR_DISTANCE && diffZ < MIN_ACTOR_DISTANCE)
			{
				if(diffX > diffZ)
				{
					if(self->x < actor->x)
					{
						deltaX = (actor->x - MIN_ACTOR_DISTANCE) - self->x;
					}
					else
					{
						deltaX = (actor->x + MIN_ACTOR_DISTANCE) - self->x;
					}
				}
				else
				{
					if(self->z < actor->z)
					{
						deltaZ = (actor->z - MIN_ACTOR_DISTANCE) - self->z;
					}
					else
					{
						deltaZ = (actor->z + MIN_ACTOR_DISTANCE) - self->z;
					}
				}
			}
		}
	}

	int8_t cellX = self->x / CELL_SIZE;
	int8_t cellZ = self->z / CELL_SIZE;

	if(deltaX < 0)
	{
		if(Map_isBlocked(&engine.map, cellX - 1, cellZ)
			|| (self->z < cellZ * CELL_SIZE + MIN_WALL_DISTANCE && Map_isBlocked(&engine.map, cellX - 1, cellZ - 1))
			|| (self->z > cellZ * CELL_SIZE + CELL_SIZE - MIN_WALL_DISTANCE && Map_isBlocked(&engine.map, cellX - 1, cellZ + 1)))
		{
			if(self->x + deltaX < cellX * CELL_SIZE + MIN_WALL_DISTANCE)
			{
				deltaX = (cellX * CELL_SIZE + MIN_WALL_DISTANCE) - self->x;
				cellX = self->x / CELL_SIZE;
			}
		}
	}
	else if(deltaX > 0)
	{
		if(Map_isBlocked(&engine.map, cellX + 1, cellZ)
			|| (self->z < cellZ * CELL_SIZE + MIN_WALL_DISTANCE && Map_isBlocked(&engine.map, cellX + 1, cellZ - 1))
			|| (self->z > cellZ * CELL_SIZE + CELL_SIZE - MIN_WALL_DISTANCE && Map_isBlocked(&engine.map, cellX + 1, cellZ + 1)))
		{
			if(self->x + deltaX > cellX * CELL_SIZE + CELL_SIZE - MIN_WALL_DISTANCE)
			{
				deltaX = (cellX * CELL_SIZE + CELL_SIZE - MIN_WALL_DISTANCE) - self->x;
				cellX = self->x / CELL_SIZE;
			}
		}
	}

	if(deltaZ < 0)
	{
		if(Map_isBlocked(&engine.map, cellX, cellZ - 1)
			|| (self->x < cellX * CELL_SIZE + MIN_WALL_DISTANCE && Map_isBlocked(&engine.map, cellX - 1, cellZ - 1))
			|| (self->x > cellX * CELL_SIZE + CELL_SIZE - MIN_WALL_DISTANCE && Map_isBlocked(&engine.map, cellX + 1, cellZ - 1)))
		{
			if(self->z + deltaZ < cellZ * CELL_SIZE + MIN_WALL_DISTANCE)
			{
				deltaZ = (cellZ * CELL_SIZE + MIN_WALL_DISTANCE) - self->z;
			}
		}
	}
	else if(deltaZ > 0)
	{
		if(Map_isBlocked(&engine.map, cellX, cellZ + 1)
			|| (self->x < cellX * CELL_SIZE + MIN_WALL_DISTANCE && Map_isBlocked(&engine.map, cellX - 1, cellZ + 1))
			|| (self->x > cellX * CELL_SIZE + CELL_SIZE - MIN_WALL_DISTANCE && Map_isBlocked(&engine.map, cellX + 1, cellZ + 1)))
		{
			if(self->z + deltaZ > cellZ * CELL_SIZE + CELL_SIZE - MIN_WALL_DISTANCE)
			{
				deltaZ = (cellZ * CELL_SIZE + CELL_SIZE - MIN_WALL_DISTANCE) - self->z;
			}
		}
	}


	self->x += deltaX;
	self->z += deltaZ;
}

#endif

#define NUM_WEAPON_FRAMES 4
#define Player_isAutomaticWeapon(w) ((w) == WeaponType_MachineGun || (w) == WeaponType_ChainGun)
void Player_updateWeapon(Player* self){
	uint16_t input = Platform_readInput();
	uint8_t fireHeld = (input & Input_Btn_B) ? 1 : 0;

	if(fireHeld)
	{
#if WOLF3D_HOLD_FIRE_REPEATS
		/* Wolf3D-style behaviour: holding fire keeps starting new shots.
		 * The old debounce gate made pistol/knife/chaingun fire only once per press.
		 */
		if(!self->weapon.debounce)
		{
			self->weapon.debounce = true;
		}
		if(self->weapon.shooting == false)
		{
			self->weapon.shooting = true;
			self->weapon.time = 0;
		}
#else
		if(!self->weapon.debounce)
		{
			self->weapon.debounce = true;
			if(self->weapon.shooting == false)
			{
				self->weapon.shooting = true;
				self->weapon.time = 0;
			}
		}
#endif
	}
	else
	{
		self->weapon.debounce = false;
	}

	if(self->weapon.shooting)
	{
		self->weapon.time++;

		switch(self->weapon.time)
		{
		case 2:
			self->weapon.frame = 1;
			break;
		case 4:
			self->weapon.frame = 2;
			Player_shootWeapon(self);
			break;
		case 6:
			if(Player_isAutomaticWeapon(self->weapon.type))
			{
				self->weapon.frame = 1;
			}
			else self->weapon.frame = 3;
			break;
		case 8:
			if(Player_isAutomaticWeapon(self->weapon.type))
			{
				if(fireHeld)
				{
					self->weapon.time = 2;
				}
				else
				{
					self->weapon.frame = 0;
					self->weapon.shooting = false;
				}
			}
			else self->weapon.frame = 1;
			break;
		case 10:
			self->weapon.frame = 0;
			self->weapon.shooting = false;
			break;
		}
	}
}
void Player_init(Player* self){
	if(self->hp == 0)
	{
#if WOLF3D_START_CHEAT_ALL_WEAPONS
		self->weapon.type = WeaponType_ChainGun;
		self->weapon.ammo = 99;
		self->hp = 100;
		self->inventoryFlags = 0;
		self->inventory.hasMachineGun = 1;
		self->inventory.hasChainGun = 1;
#else
		self->weapon.type = WeaponType_Pistol;
		self->weapon.ammo = 8;
		self->hp = 100;
		self->inventoryFlags = 0;
#endif
	}

	self->weapon.frame = 0;
	self->weapon.time = 0;
	self->weapon.debounce = false;
	self->weapon.shooting = false;
	self->weaponCycleHeld = 0;

	// Find player start tile.  Do not use Map_updateBufferPosition() here:
	// streaming every 16x16 chunk while searching pollutes the active door/item/
	// actor tables before the real play buffer is selected.
	bool foundStart = false;
	for(int8_t z = 0; z < MAP_SIZE; z++)
	{
		for(int8_t x = 0; x < MAP_SIZE; x++)
		{
			uint8_t tile = Map_readRawTile(&engine.map, x, z);

			if(tile >= Tile_PlayerStart_North && tile <= Tile_PlayerStart_West)
			{
				self->x = CELL_TO_WORLD(x) + CELL_SIZE / 2;
				self->z = CELL_TO_WORLD(z) + CELL_SIZE / 2;
				self->direction = (uint8_t)((tile - Tile_PlayerStart_North - 1) * DEGREES_90);
				foundStart = true;
			}
		}
	}

	if(!foundStart)
	{
		self->x = CELL_TO_WORLD(1) + CELL_SIZE / 2;
		self->z = CELL_TO_WORLD(1) + CELL_SIZE / 2;
		self->direction = 0;
	}
}
void Player_shootWeapon(Player* self){
	switch(self->weapon.type)
	{
	case WeaponType_Knife:
		Platform_playSound(Sound_AttackKnife);
		break;
	case WeaponType_MachineGun:
	case WeaponType_ChainGun:
		Platform_playSound(Sound_AttackMachineGun);
		break;
	default:
		Platform_playSound(Sound_AttackPistol);
		break;
	}

	if(self->weapon.type != WeaponType_Knife)
	{
		if(self->weapon.ammo == 0)
		{
			self->weapon.type = WeaponType_Knife;
			self->weapon.time = 0;
			self->weapon.frame = 0;
			self->weapon.shooting = false;
			return;
		}
		self->weapon.ammo--;
	}

	int16_t rotCos = FixedMath_Cos(-self->direction);
	int16_t rotSin = FixedMath_Sin(-self->direction);
	int8_t closestActor = -1;
	int16_t actorDistance = 0;
	
	for(int8_t n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		if(engine.actors[n].type != ActorType_Empty && engine.actors[n].hp > 0)
		{
			int16_t zt = (int16_t)(FIXED_TO_INT(rotCos * (int32_t)(engine.actors[n].x - self->x)) - FIXED_TO_INT(rotSin * (int32_t)(engine.actors[n].z - self->z)));
			int16_t xt = (int16_t)(FIXED_TO_INT(rotSin * (int32_t)(engine.actors[n].x - self->x)) + FIXED_TO_INT(rotCos * (int32_t)(engine.actors[n].z - self->z)));

			if(zt > CLIP_PLANE && xt > -ACTOR_HITBOX_SIZE / 2 && xt < ACTOR_HITBOX_SIZE / 2 && (zt < INT_TO_FIXED(CELL_SIZE) || self->weapon.type != WeaponType_Knife))
			{
				if(closestActor == -1 || zt < actorDistance)
				{
					closestActor = n;
					actorDistance = zt;
				}
			}
		}
	}

	if(closestActor != -1)
	{
		if(self->weapon.type == WeaponType_Knife)
		{
			uint8_t damage = getRandomNumber() >> 4;
			Actor_damage(&engine.actors[closestActor], damage);
		}
		else if(Map_isClearLine(&engine.map, self->x, self->z, engine.actors[closestActor].x, engine.actors[closestActor].z))
		{
			int8_t dist = Actor_getPlayerCellDistance(&engine.actors[closestActor]);
			int damage;

			if (dist < 2)
				damage = getRandomNumber() / 4;
			else if (dist<4)
				damage = getRandomNumber() / 6;
			else
			{
				if ( (getRandomNumber() / 12) < dist)           // missed
					goto missed;
				damage = getRandomNumber() / 6;
			}
			
			Actor_damage(&engine.actors[closestActor], damage);
			WARNING("BANG!\n");
		}
		else
		{
			WARNING("NOT A CLEAR LINE!\n");
		}
	}
	else
	{
		WARNING("NO TARGET!\n");
	}

missed:
	if(self->weapon.type != WeaponType_Knife && self->weapon.ammo == 0)
	{
		self->weapon.type = WeaponType_Knife;
		self->weapon.time = 0;
		self->weapon.frame = 0;
		self->weapon.shooting = false;
	}

}
void Player_damage(Player* self, uint8_t amount){
	WARNING("Player damage: %d\n", (int)amount);
	engine.renderer.damageIndicator = 5;
	engine.renderer.hudHealthFlash = WOLF3D_HUD_FLASH_FRAMES;

	if(self->hp == 0)
	{
		return;
	}

	if(amount >= self->hp)
	{
		self->hp = 0;
		Platform_stopMusic();
		Platform_playSound(Sound_PlayerDeath);
	}
	else
	{
		self->hp -= amount;
		Platform_playSound(Sound_PlayerDamage);
	}
}
