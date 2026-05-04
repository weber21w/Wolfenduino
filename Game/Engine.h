#ifndef ENGINE_H_
#define ENGINE_H_

#include "Defines.h"
#include "Platform.h"
#include "Renderer.h"
#include "Player.h"
#include "Map.h"
#include "Actor.h"
#include "Menu.h"

enum
{
	GameState_Title,
	GameState_Menu,
	GameState_PauseMenu,
	GameState_Loading,
	GameState_Playing,
	GameState_Dead,
	GameState_FinishedLevel,
	GameState_StartingLevel,
	GameState_Win
};

enum Difficulty
{
	Difficulty_Baby,
	Difficulty_Easy,
	Difficulty_Medium,
	Difficulty_Hard
};

typedef struct Engine
{
	Renderer renderer;
	Player player;
	Map map;
	Menu menu;
	Actor actors[MAX_ACTIVE_ACTORS];
	int16_t frameCount;
	uint8_t gameState;
	uint8_t difficulty;
	uint8_t mapVisible;
	uint8_t mapSelectHeld;
	uint8_t mapZoom;
	uint8_t mapSLHeld;
	uint8_t mapSRHeld;
} Engine;

extern Engine engine;

void Engine_init(void);
void Engine_update(void);
void Engine_draw(void);
void Engine_startLevel(bool resetPlayer);
void Engine_startLevelDefault(void);
void Engine_startNewGame(void);
void Engine_startingLevel(void);
void Engine_drawTitle(void);
Actor* Engine_spawnActor(uint8_t spawnId, uint8_t actorType, int8_t cellX, int8_t cellZ);

#endif
