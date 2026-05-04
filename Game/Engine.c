#include "Engine.h"
#include "../DataHeaders/Data_GUI.h"

Engine engine;

void Engine_init(void)
{
	Menu_init(&engine.menu);
	engine.difficulty = Difficulty_Medium;
	Map_initStreaming(&engine.map);
	#if WOLF3D_TITLE_SCREEN_ENABLE
	engine.gameState = GameState_Title;
#else
	engine.gameState = GameState_Menu;
#endif
	engine.map.currentLevel = 0;
	engine.mapVisible = 0;
	engine.mapSelectHeld = 0;
	engine.mapZoom = WOLF3D_MAP_DEFAULT_SCALE;
	engine.mapSLHeld = 0;
	engine.mapSRHeld = 0;
}

void Engine_startLevel(bool resetPlayer)
{
	if(resetPlayer)
	{
		engine.player.hp = 0;
	}
	engine.frameCount = 0;
	engine.gameState = GameState_StartingLevel;
}

void Engine_startLevelDefault(void)
{
	Engine_startLevel(true);
}


void Engine_startNewGame(void)
{
	engine.map.currentLevel = 0;
	engine.player.hp = 0;
	engine.player.lives = WOLF3D_PLAYER_START_LIVES;
	engine.player.inventoryFlags = 0;
	Engine_startLevelDefault();
}

void Engine_startingLevel(void)
{
	engine.gameState = GameState_Loading;
	for(int n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		engine.actors[n].type = ActorType_Empty;
		engine.actors[n].spawnId = 0xff;
	}

	Map_init(&engine.map);
	Renderer_init(&engine.renderer);
	Player_init(&engine.player);
	Player_update(&engine.player);

	engine.frameCount = 0;
	engine.mapVisible = 0;
	engine.mapSelectHeld = 0;
	engine.mapZoom = WOLF3D_MAP_DEFAULT_SCALE;
	engine.mapSLHeld = 0;
	engine.mapSRHeld = 0;
	Platform_startLevelMusic();
	engine.gameState = GameState_Playing;
}

void Engine_update(void)
{
	switch(engine.gameState)
	{
	case GameState_Title:
		if(Platform_readInput() & (Input_Btn_A | Input_Btn_C))
		{
			Menu_switchMenu(&engine.menu, (MenuData*)Menu_Main);
			engine.gameState = GameState_Menu;
		}
		break;
	case GameState_Playing:
#if WOLF3D_MAP_TOGGLE_ENABLE
	{
		uint16_t input = Platform_readInput();
		if(input & Input_Btn_Select)
		{
			if(!engine.mapSelectHeld)
			{
				engine.mapSelectHeld = 1;
				engine.mapVisible ^= 1;
			}
		}
		else
		{
			engine.mapSelectHeld = 0;
		}

#if WOLF3D_MAP_DEDICATED_SCREEN
		if(engine.mapVisible)
		{
			if(input & Input_Btn_SL)
			{
				if(!engine.mapSLHeld && engine.mapZoom > WOLF3D_MAP_MIN_SCALE)
				{
					engine.mapZoom--;
				}
				engine.mapSLHeld = 1;
			}
			else
			{
				engine.mapSLHeld = 0;
			}

			if(input & Input_Btn_SR)
			{
				if(!engine.mapSRHeld && engine.mapZoom < WOLF3D_MAP_MAX_SCALE)
				{
					engine.mapZoom++;
				}
				engine.mapSRHeld = 1;
			}
			else
			{
				engine.mapSRHeld = 0;
			}
		}
		else
		{
			engine.mapSLHeld = 0;
			engine.mapSRHeld = 0;
		}
#endif
	}
#endif
		Player_update(&engine.player);

		if(engine.player.hp > 0)
		{
			Map_update(&engine.map);
#if WOLF3D_MAP_DEDICATED_SCREEN && WOLF3D_SEEN_WALL_BITSET
			if(engine.mapVisible)
			{
				Renderer_updateSeenWalls(&engine.renderer);
			}
#endif
			for(int n = 0; n < MAX_ACTIVE_ACTORS; n++)
			{
				Actor_update(&engine.actors[n]);
			}
		}

		if(Platform_readInput() & Input_Btn_C)
		{
			engine.gameState = GameState_PauseMenu;
			Menu_switchMenu(&engine.menu, (MenuData*)Menu_Paused);
		}
		break;
	case GameState_Menu:
	case GameState_PauseMenu:
		Menu_update(&engine.menu);
		break;
	case GameState_FinishedLevel:
		Platform_stopMusic();
		engine.gameState = GameState_Menu;
		Menu_switchMenu(&engine.menu, Menu_FloorComplete);
		break;
	case GameState_Win:
		Platform_stopMusic();
		if(Platform_readInput() & (Input_Btn_A | Input_Btn_C))
		{
			engine.map.currentLevel = 0;
			engine.gameState = GameState_Menu;
			Menu_switchMenu(&engine.menu, (MenuData*)Menu_Main);
		}
		break;
	case GameState_StartingLevel:
		if(engine.frameCount >= WOLF3D_GET_PSYCHED_FRAMES)
		{
			Engine_startingLevel();
		}
		break;
	case GameState_Dead:
		if(engine.frameCount >= WOLF3D_DEATH_TOTAL_FRAMES)
		{
			if(engine.player.lives == 0)
			{
				engine.gameState = GameState_Menu;
				Menu_switchMenu(&engine.menu, Menu_GameOver);
			}
			else
			{
				engine.player.lives--;
				Engine_startLevelDefault();
			}
		}
		break;
	}

	engine.frameCount++;
}

void Engine_drawTitle(void)
{
#if WOLF3D_FX_TITLE_BITMAP
	Platform_drawRleBitmap128x64(Wolf3D_TitleRle, Wolf3D_TitleRle_LEN);
#else
	clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
	Renderer_drawStringScaled(&engine.renderer, PSTR("WOLFENDUINO"), 12, 8, 2, WOLF3D_FONT_FOREGROUND_COLOUR, WOLF3D_FONT_BACKGROUND_COLOUR);
	Renderer_drawStringScaled(&engine.renderer, PSTR("3D"), 56, 24, 2, WOLF3D_FONT_FOREGROUND_COLOUR, WOLF3D_FONT_BACKGROUND_COLOUR);
#endif
	if(engine.frameCount & 16)
	{
		Renderer_drawString(&engine.renderer, PSTR("PRESS A/START"), 38, 50);
	}
}

void Engine_draw(void)
{
	switch(engine.gameState)
	{
	case GameState_Title:
		Engine_drawTitle();
		break;
	case GameState_Menu:
	case GameState_PauseMenu:
		Menu_draw(&engine.menu);
		break;
	case GameState_Playing:
#if WOLF3D_MAP_DEDICATED_SCREEN
		if(engine.mapVisible)
		{
			clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
			Renderer_drawMap(&engine.renderer);
		}
		else
#endif
		{
			Renderer_drawFrame(&engine.renderer);
		}
		break;
	case GameState_Win:
		clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
		Renderer_drawStringScaled(&engine.renderer, PSTR("YOU WIN"), 37, 18, 1, WOLF3D_FONT_FOREGROUND_COLOUR, WOLF3D_FONT_BACKGROUND_COLOUR);
		Renderer_drawString(&engine.renderer, PSTR("PRESS START"), 42, 42);
		break;
	case GameState_StartingLevel:
	case GameState_Loading:
		clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
		Renderer_drawString(&engine.renderer, PSTR("GET PSYCHED!"), 44, 5);
		Renderer_drawString(&engine.renderer, PSTR("FLOOR:"), 51, 53);
		Renderer_drawInt(&engine.renderer, engine.map.currentLevel + 1, 80, 53);
#if WOLF3D_FX_DIFFICULTY_FACES
		Menu_drawFace(WOLF3D_GUI_FACE_NORMAL, 36, 16);
#endif
		Renderer_drawString(&engine.renderer, PSTR("X"), 70, 32);
		Renderer_drawInt(&engine.renderer, engine.player.lives, 82, 32);
		break;
	case GameState_Dead:
		if(engine.frameCount < WOLF3D_DEATH_STATIC_FRAMES)
		{
			for(int n = 0; n < DISPLAYWIDTH * (DISPLAYHEIGHT / 15); n++)
			{
				uint8_t x = (n + getRandomNumber16() + getRandomNumber16()) % DISPLAYWIDTH;
				uint8_t y = (n + getRandomNumber16() + getRandomNumber16()) % DISPLAYHEIGHT;
				setPixel(x, y);
			}
		}
		else
		{
			clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
#if WOLF3D_FX_DIFFICULTY_FACES
			Menu_drawFace(WOLF3D_GUI_FACE_DEAD, 8, 16);
#endif
			if(engine.player.lives == 0)
			{
				Renderer_drawStringScaled(&engine.renderer, PSTR("GAME OVER"), 38, 18, 1, WOLF3D_FONT_FOREGROUND_COLOUR, WOLF3D_FONT_BACKGROUND_COLOUR);
			}
			else
			{
				Renderer_drawStringScaled(&engine.renderer, PSTR("YOU DIED"), 38, 14, 1, WOLF3D_FONT_FOREGROUND_COLOUR, WOLF3D_FONT_BACKGROUND_COLOUR);
				Renderer_drawString(&engine.renderer, PSTR("LIVES"), 50, 30);
				Renderer_drawInt(&engine.renderer, engine.player.lives, 88, 30);
				Renderer_drawString(&engine.renderer, PSTR("RELOADING"), 46, 45);
			}
		}
		break;
	}
}

Actor* Engine_spawnActor(uint8_t spawnId, uint8_t actorType, int8_t cellX, int8_t cellZ)
{
	for(int n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		if(engine.actors[n].type != ActorType_Empty && engine.actors[n].spawnId == spawnId)
		{
			return NULL;
		}
	}

	for(int n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		if(engine.actors[n].type == ActorType_Empty)
		{
			Actor_init(&engine.actors[n], spawnId, actorType, cellX, cellZ);
			return &engine.actors[n];
		}
	}

	for(int n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		if(engine.actors[n].flags.frozen && !engine.actors[n].flags.persistent)
		{
			Actor_init(&engine.actors[n], spawnId, actorType, cellX, cellZ);
			return &engine.actors[n];
		}
	}

	WARNING("Could not find a slot for new actor\n");
	return NULL;
}
