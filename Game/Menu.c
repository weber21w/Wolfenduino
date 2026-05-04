#include <string.h>
#include "Engine.h"
#include "Menu.h"
#include "Sounds.h"
#include "../DataHeaders/Data_GUI.h"

#define MENU_ENTRY_END 0
#define MENU_STR(x) ((const void*)(x))
#define MENU_CALLBACK(x) ((const void*)(x))

// Main menu
const char Str_Wolfenduino3D[] PROGMEM = "WOLFENDUINO 3D";
const char Str_Continue[] PROGMEM = "CONTINUE";
const char Str_NewGame[] PROGMEM = "NEW GAME";
const char Str_Sound[] PROGMEM = "SOUND:";
const char Str_On[] PROGMEM = "ON";
const char Str_Off[] PROGMEM = "OFF";
#if WOLF3D_HELP_MENU_ENABLE
const char Str_Help[] PROGMEM = "HELP";
#endif
const char Str_Quit[] PROGMEM = "QUIT";

const MenuEntry Menu_Main[] PROGMEM = 
{
	Str_Wolfenduino3D,
	Str_NewGame,		MENU_CALLBACK(Menu_newGame),
	Str_Sound,			MENU_CALLBACK(Menu_toggleSound),
#if WOLF3D_HELP_MENU_ENABLE
	Str_Help,			MENU_CALLBACK(Menu_showHelp),
#endif
	Str_Quit,			MENU_CALLBACK(Menu_quit),
	MENU_ENTRY_END
};

const MenuEntry Menu_Paused[] PROGMEM = 
{
	Str_Wolfenduino3D,
	Str_Continue,		MENU_CALLBACK(Menu_continueGame),
	Str_NewGame,		MENU_CALLBACK(Menu_newGame),
	Str_Sound,			MENU_CALLBACK(Menu_toggleSound),
#if WOLF3D_HELP_MENU_ENABLE
	Str_Help,			MENU_CALLBACK(Menu_showHelp),
#endif
	Str_Quit,			MENU_CALLBACK(Menu_quit),
	MENU_ENTRY_END
};

// Difficulty menu
const char Str_ChooseDifficulty[] PROGMEM = "HOW TOUGH ARE YOU?";
const char Str_SkillBaby[] PROGMEM = "CAN I PLAY, DADDY?";
const char Str_SkillEasy[] PROGMEM = "DON'T HURT ME.";
const char Str_SkillMedium[] PROGMEM = "BRING 'EM ON!";
const char Str_SkillHard[] PROGMEM = "I AM DEATH\n  INCARNATE!";

const MenuEntry Menu_ChooseDifficulty[] PROGMEM = 
{
	Str_ChooseDifficulty,
	Str_SkillBaby,		MENU_CALLBACK(Menu_skillBaby),
	Str_SkillEasy,		MENU_CALLBACK(Menu_skillEasy),
	Str_SkillMedium,	MENU_CALLBACK(Menu_skillMedium),
	Str_SkillHard,		MENU_CALLBACK(Menu_skillHard),
	MENU_ENTRY_END
};

#if WOLF3D_HELP_MENU_ENABLE
const char Str_HelpTitle[] PROGMEM = "HELP";
const MenuEntry Menu_Help[] PROGMEM =
{
	Str_HelpTitle,
	MENU_ENTRY_END
};
#endif

const char Str_FloorComplete[] PROGMEM = "FLOOR COMPLETE";
const MenuEntry Menu_FloorComplete[] PROGMEM =
{
	Str_FloorComplete,
	MENU_ENTRY_END
};

const char Str_GameOver[] PROGMEM = "GAME OVER";
const MenuEntry Menu_GameOver[] PROGMEM =
{
	Str_GameOver,
	MENU_ENTRY_END
};

static const MenuEntry* Menu_rootForState(void)
{
	return (engine.gameState == GameState_PauseMenu) ? Menu_Paused : Menu_Main;
}

static uint8_t Menu_pgmStrLen(const char* str)
{
	uint8_t len = 0;
	while(pgm_read_byte(str++))
	{
		len++;
	}
	return len;
}

static uint8_t Menu_titleWidth(const char* title)
{
	return (uint8_t)(Menu_pgmStrLen(title) * (FONT_WIDTH + 1));
}

static void Menu_drawHLine(uint8_t x, uint8_t y, uint8_t w, uint8_t colour)
{
	while(w--)
	{
		drawPixel(x++, y, colour);
	}
}

static void Menu_drawVLine(uint8_t x, uint8_t y, uint8_t h, uint8_t colour)
{
	while(h--)
	{
		drawPixel(x, y++, colour);
	}
}

static void Menu_drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t colour)
{
	if(w < 2 || h < 2)
	{
		return;
	}
	Menu_drawHLine(x, y, w, colour);
	Menu_drawHLine(x, (uint8_t)(y + h - 1), w, colour);
	Menu_drawVLine(x, y, h, colour);
	Menu_drawVLine((uint8_t)(x + w - 1), y, h, colour);
}

static void Menu_drawCentered(const char* text, uint8_t y)
{
	uint8_t width = Menu_titleWidth(text);
	uint8_t x = (width >= DISPLAYWIDTH) ? 0 : (uint8_t)((DISPLAYWIDTH - width) >> 1);
	Renderer_drawString(&engine.renderer, text, x, y);
}

static void Menu_drawChrome(const char* title)
{
	uint8_t width = Menu_titleWidth(title);
	uint8_t x = (width >= DISPLAYWIDTH) ? 0 : (uint8_t)((DISPLAYWIDTH - width) >> 1);
	Menu_drawBox(0, 0, DISPLAYWIDTH, DISPLAYHEIGHT, WOLF3D_FONT_FOREGROUND_COLOUR);
	Menu_drawHLine(4, 10, DISPLAYWIDTH - 8, WOLF3D_FONT_FOREGROUND_COLOUR);
	Renderer_drawString(&engine.renderer, title, x, 3);
}

#if WOLF3D_FX_DIFFICULTY_FACES
/* Six 23x32 faces from the FX UI assets, stored as 1bpp page columns.
 * 0 = baby, 1 = easy, 2 = medium, 3 = hard, 4 = dead/game over, 5 = normal/Get Psyched.
 * Bit 1 = dark/foreground pixel.
 */
static const uint8_t Menu_Faces[] PROGMEM =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0xf0, 0xfd, 0x03, 0x00, 0xfc, 0x02, 0xfc, 0x00,
	0x7e, 0xff, 0x83, 0x01, 0x8e, 0xe7, 0x7f, 0x73, 0xde, 0xd3, 0xff, 0x5a, 0xce, 0xc3, 0xff, 0x2d,
	0xde, 0xf3, 0xcf, 0x2b, 0xce, 0xef, 0xb4, 0x2b, 0xde, 0xff, 0x15, 0x56, 0xce, 0xff, 0x15, 0x56,
	0xde, 0xff, 0x15, 0x56, 0xce, 0xef, 0xb4, 0x2b, 0xde, 0xf7, 0xcf, 0x2b, 0xce, 0xc3, 0xff, 0x2d,
	0xde, 0xd3, 0xff, 0x5a, 0xcc, 0xe3, 0x7f, 0x7b, 0xbc, 0xff, 0x83, 0x79, 0x7c, 0x00, 0xfc, 0x00,
	0xf0, 0xfd, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xf0, 0x01, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xef, 0xff, 0x01,
	0xc0, 0xe7, 0xff, 0x43, 0xc0, 0xd3, 0xff, 0x63, 0xc0, 0xc3, 0xff, 0x73, 0xc0, 0xf3, 0xff, 0x73,
	0xc0, 0xef, 0xec, 0x7b, 0xc0, 0xff, 0xed, 0x7b, 0x80, 0xff, 0xed, 0x7b, 0x80, 0xff, 0xed, 0x7b,
	0x80, 0xeb, 0xec, 0x7b, 0x80, 0xf3, 0xff, 0x7b, 0x80, 0xc3, 0xff, 0x7b, 0x80, 0xd3, 0xff, 0x7b,
	0x80, 0xe7, 0xff, 0x7d, 0x80, 0xef, 0xff, 0x7e, 0x80, 0xff, 0xff, 0x00, 0x00, 0xc0, 0x01, 0x00,
	0x00, 0xf0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00,
	0x00, 0xe0, 0x01, 0x00, 0x00, 0xff, 0x3f, 0x00, 0x80, 0xf7, 0xff, 0x01, 0xc0, 0xf3, 0xff, 0x43,
	0xc0, 0xc3, 0xff, 0x63, 0xc0, 0xc7, 0xff, 0x73, 0xc0, 0xef, 0xef, 0x73, 0xc0, 0xef, 0xec, 0x7b,
	0xc0, 0xff, 0xed, 0x7b, 0x80, 0xff, 0xed, 0x7b, 0x80, 0xff, 0xed, 0x7b, 0x80, 0xef, 0xec, 0x7b,
	0x80, 0xef, 0xef, 0x7b, 0x80, 0xcf, 0xff, 0x7b, 0xc0, 0xc7, 0xff, 0x7b, 0xc0, 0xf7, 0xff, 0x7d,
	0xc0, 0xef, 0xff, 0x7e, 0x80, 0xff, 0x7f, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xf0, 0x03, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0x00, 0x00, 0xf0, 0x01, 0x00,
	0x00, 0xf0, 0x1f, 0x00, 0x00, 0xbc, 0xff, 0x01, 0x00, 0x1e, 0xff, 0x41, 0x00, 0x9e, 0xfe, 0x63,
	0x00, 0xbe, 0xfe, 0x77, 0x00, 0x3e, 0xbf, 0x77, 0x00, 0x7e, 0xb7, 0x77, 0x00, 0xfe, 0xb7, 0x77,
	0x00, 0xfe, 0xaf, 0x77, 0x00, 0xfc, 0xb7, 0x77, 0x00, 0x7c, 0xb7, 0x77, 0x00, 0x38, 0xbf, 0x77,
	0x00, 0xb8, 0xce, 0x77, 0x00, 0x98, 0xfe, 0x77, 0x00, 0x1c, 0xff, 0x77, 0x00, 0xbe, 0xff, 0x7b,
	0x00, 0xfc, 0xff, 0x01, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x01, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0xf0, 0x7f, 0x00,
	0x00, 0xbc, 0xfe, 0x00, 0x00, 0x1e, 0xff, 0x21, 0x00, 0x13, 0xff, 0x23, 0x00, 0x18, 0x3e, 0x12,
	0x00, 0x3e, 0x1e, 0x20, 0x00, 0x3c, 0xd3, 0x21, 0x00, 0xfc, 0xcf, 0x33, 0x00, 0xfe, 0xaf, 0x31,
	0x00, 0xf6, 0x0f, 0x33, 0x00, 0x28, 0x06, 0x39, 0x00, 0x10, 0x12, 0x20, 0x00, 0x00, 0x3c, 0x00,
	0x00, 0x10, 0xfc, 0x1b, 0x00, 0x00, 0xfe, 0x3d, 0x00, 0x3c, 0xfe, 0x1f, 0x00, 0xfc, 0xf6, 0x0f,
	0x00, 0xe0, 0x01, 0x00, 0x00, 0xb0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xf0, 0x01, 0x00, 0x00, 0xe0, 0x0f, 0x00, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0xef, 0xff, 0x00,
	0x80, 0xf7, 0xff, 0x41, 0xc0, 0xc7, 0xff, 0x43, 0xc0, 0xc7, 0xef, 0x63, 0xc0, 0xef, 0xef, 0x63,
	0x80, 0xcf, 0xec, 0x63, 0x80, 0xff, 0xed, 0x63, 0x80, 0xff, 0xed, 0x63, 0x80, 0xff, 0xed, 0x63,
	0x00, 0xcf, 0xec, 0x73, 0x00, 0xec, 0xef, 0x73, 0x00, 0xc6, 0xef, 0x7b, 0x00, 0xc4, 0xff, 0x7b,
	0x00, 0xf7, 0xff, 0x7d, 0x80, 0xef, 0xff, 0x7f, 0x00, 0xff, 0xff, 0x7f, 0x00, 0xe0, 0x0f, 0x00,
	0x00, 0xf0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#endif

#if WOLF3D_FX_DIFFICULTY_FACES
void Menu_drawFace(uint8_t id, uint8_t x, uint8_t y)
{
	if(id >= WOLF3D_GUI_FACE_COUNT)
	{
		id = WOLF3D_GUI_FACE_MEDIUM;
	}

	const uint8_t* src = Menu_Faces + ((uint16_t)id * WOLF3D_GUI_FACE_PAGE_BYTES);

	for(uint8_t ix = 0; ix < WOLF3D_GUI_FACE_WIDTH; ix++)
	{
		for(uint8_t page = 0; page < (WOLF3D_GUI_FACE_HEIGHT / 8); page++)
		{
			uint8_t bits = pgm_read_byte(src++);
			for(uint8_t bit = 0; bit < 8; bit++)
			{
				drawPixel((uint8_t)(x + ix), (uint8_t)(y + (page * 8) + bit), (bits & (1u << bit)) ? WOLF3D_FONT_FOREGROUND_COLOUR : WOLF3D_FONT_BACKGROUND_COLOUR);
			}
		}
	}
}
#endif

void Menu_toggleSound(void){
	Platform_setMuted(!Platform_isMuted());
}
void Menu_newGame(void){
	Menu_switchMenu(&engine.menu, Menu_ChooseDifficulty);
}
void Menu_continueGame(void){
	engine.gameState = GameState_Playing;
}
#if WOLF3D_HELP_MENU_ENABLE
void Menu_showHelp(void){
	Menu_switchMenu(&engine.menu, Menu_Help);
}
#endif

#ifdef PLATFORM_GAMEBUINO
extern Gamebuino gb;
void Menu_quit(void){
	gb.changeGame();
}
#else
void Menu_quit(void){
}
#endif
static void Menu_startSkill(uint8_t difficulty)
{
	engine.difficulty = difficulty;
	Engine_startNewGame();
}

void Menu_skillBaby(void){
	Menu_startSkill(Difficulty_Baby);
}
void Menu_skillEasy(void){
	Menu_startSkill(Difficulty_Easy);
}
void Menu_skillMedium(void){
	Menu_startSkill(Difficulty_Medium);
}
void Menu_skillHard(void){
	Menu_startSkill(Difficulty_Hard);
}
void Menu_nextLevel(void){
#if WOLF3D_FLASH_LEVEL_COUNT > 1
	if(engine.map.currentLevel + 1 < WOLF3D_FLASH_LEVEL_COUNT)
	{
		engine.map.currentLevel++;
		Engine_startLevel(false);
	}
	else
#endif
	{
		engine.gameState = GameState_Win;
	}
}
void Menu_init(Menu* self){
	Menu_switchMenu(self, Menu_Main);
}
void Menu_draw(Menu* self){
	const char* title;
	int8_t index;
	uint8_t y;
	uint8_t item;

#if WOLF3D_HELP_MENU_ENABLE
	if(self->currentMenu == Menu_Help)
	{
#if WOLF3D_FX_HELP_BITMAP
		Platform_drawRleBitmap128x64(Wolf3D_HelpRle, Wolf3D_HelpRle_LEN);
#else
		clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
		Menu_drawChrome(Str_HelpTitle);
		Renderer_drawString(&engine.renderer, PSTR("MOVE"), 6, 18);
		Renderer_drawString(&engine.renderer, PSTR("A SHOOT"), 60, 18);
		Renderer_drawString(&engine.renderer, PSTR("B STRAFE"), 60, 28);
		Renderer_drawString(&engine.renderer, PSTR("START PAUSE"), 44, 38);
#endif
		Renderer_drawString(&engine.renderer, PSTR("B BACK"), 47, 56);
		return;
	}
#endif

	if(self->currentMenu == Menu_FloorComplete)
	{
		clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
		Menu_drawBox(4, 4, DISPLAYWIDTH - 8, DISPLAYHEIGHT - 8, WOLF3D_FONT_FOREGROUND_COLOUR);
		Menu_drawCentered(Str_FloorComplete, 12);
		Renderer_drawString(&engine.renderer, PSTR("FLOOR"), 45, 28);
		Renderer_drawInt(&engine.renderer, engine.map.currentLevel + 1, 82, 28);
#if WOLF3D_FLASH_LEVEL_COUNT > 1
		Renderer_drawString(&engine.renderer, PSTR("PRESS START"), 36, 46);
#else
		Renderer_drawString(&engine.renderer, PSTR("PRESS START"), 36, 42);
		Renderer_drawString(&engine.renderer, PSTR("TO REPLAY"), 42, 50);
#endif
		return;
	}

	if(self->currentMenu == Menu_GameOver)
	{
		clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
		Menu_drawBox(4, 4, DISPLAYWIDTH - 8, DISPLAYHEIGHT - 8, WOLF3D_FONT_FOREGROUND_COLOUR);
		Menu_drawCentered(Str_GameOver, 8);
#if WOLF3D_FX_DIFFICULTY_FACES
		Menu_drawFace(WOLF3D_GUI_FACE_DEAD, 15, 22);
#endif
		Renderer_drawString(&engine.renderer, PSTR("FLOOR"), 58, 24);
		Renderer_drawInt(&engine.renderer, engine.map.currentLevel + 1, 95, 24);
		Renderer_drawString(&engine.renderer, PSTR("PRESS START"), 47, 45);
		Renderer_drawString(&engine.renderer, PSTR("NEW GAME"), 53, 53);
		return;
	}

#if WOLF3D_TITLE_MENU_BACKGROUND
	if(self->currentMenu == Menu_Main)
	{
#if WOLF3D_FX_TITLE_BITMAP
		Platform_drawRleBitmap128x64(Wolf3D_TitleRle, Wolf3D_TitleRle_LEN);
#else
		clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
#endif
		y = 25;
	}
	else
#endif
	{
		clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
		title = (const char*)pgm_read_ptr(&self->currentMenu[0]);
		Menu_drawChrome(title);
		y = 18;
	}

	index = 1;
	item = 0;

	while(1)
	{
		const char* text;
		if(pgm_read_ptr(&self->currentMenu[index]) == 0)
			break;
		text = (const char*)pgm_read_ptr(&self->currentMenu[index]);
		Renderer_drawString(&engine.renderer, text, 14, y);

		if(text == Str_Sound)
		{
			if(Platform_isMuted())
			{
				Renderer_drawString(&engine.renderer, Str_Off, 48, y);
			}
			else
			{
				Renderer_drawString(&engine.renderer, Str_On, 48, y);
			}
		}
		index += 2;
		y += 8;
		item++;
	}

	if(Menu_numMenuItems(self))
	{
		uint8_t markerBaseY = 18;
#if WOLF3D_TITLE_MENU_BACKGROUND
		if(self->currentMenu == Menu_Main)
		{
			markerBaseY = 25;
		}
#endif
		uint8_t markerY = (uint8_t)(markerBaseY + self->currentSelection * 8);
		if(engine.frameCount & 8)
		{
			Renderer_drawString(&engine.renderer, PSTR("*"), 5, markerY);
		}
		else
		{
			Renderer_drawString(&engine.renderer, PSTR(">"), 5, markerY);
		}
	}

	if(self->currentMenu == Menu_ChooseDifficulty)
	{
#if WOLF3D_FX_DIFFICULTY_FACES
		Menu_drawBox(98, 18, 27, 36, WOLF3D_FONT_FOREGROUND_COLOUR);
		Menu_drawFace((uint8_t)self->currentSelection, 100, 20);
#else
		static const char skill0[] PROGMEM = "BJ";
		static const char skill1[] PROGMEM = "OK";
		static const char skill2[] PROGMEM = "!!";
		static const char skill3[] PROGMEM = "XX";
		const char* face = skill0;
		switch(self->currentSelection)
		{
		case Difficulty_Easy: face = skill1; break;
		case Difficulty_Medium: face = skill2; break;
		case Difficulty_Hard: face = skill3; break;
		default: break;
		}
		Menu_drawBox(96, 22, 24, 18, WOLF3D_FONT_FOREGROUND_COLOUR);
		Renderer_drawString(&engine.renderer, face, 104, 29);
#endif
	}
}
void Menu_update(Menu* self){
	uint16_t input = Platform_readInput();
	if(!self->debounceInput)
	{
#if WOLF3D_HELP_MENU_ENABLE
		if(self->currentMenu == Menu_Help)
		{
			if(input & (Input_Btn_A | Input_Btn_B | Input_Btn_C))
			{
				Menu_switchMenu(self, Menu_rootForState());
			}
		}
		else
#endif
		if(self->currentMenu == Menu_FloorComplete)
		{
			if(input & (Input_Btn_A | Input_Btn_B | Input_Btn_C))
			{
				Menu_nextLevel();
			}
		}
		else if(self->currentMenu == Menu_GameOver)
		{
			if(input & (Input_Btn_A | Input_Btn_B | Input_Btn_C))
			{
				Menu_switchMenu(self, Menu_Main);
				engine.gameState = GameState_Menu;
			}
		}
		else
		{
			if(input & Input_Dpad_Up)
			{
				self->currentSelection --;
				if(self->currentSelection == -1)
					self->currentSelection = Menu_numMenuItems(self) - 1;
				Platform_playSound(Sound_HitEnemy);
			}
			if(input & Input_Dpad_Down)
			{
				self->currentSelection ++;
				if(self->currentSelection == Menu_numMenuItems(self))
					self->currentSelection = 0;
				Platform_playSound(Sound_HitEnemy);
			}
			if(input & (Input_Btn_A
#if WOLF3D_MENU_START_SELECTS
				| Input_Btn_C
#endif
			))
			{
				MenuFn fn = (MenuFn)pgm_read_ptr(&self->currentMenu[self->currentSelection * 2 + 2]);
				Platform_playSound(Sound_AttackPistol);
				fn();
			}
			if(input & Input_Btn_C)
			{
				if(engine.gameState == GameState_PauseMenu)
					Menu_continueGame();
			}
			if(input & Input_Btn_B)
			{
				if(self->currentMenu == Menu_ChooseDifficulty)
				{
					Menu_switchMenu(self, Menu_rootForState());
				}
				else if(self->currentMenu == Menu_Paused)
				{
					Menu_continueGame();
				}
			}
		}
	}
	self->debounceInput = input != 0;
}
int8_t Menu_numMenuItems(Menu* self){
	int8_t index = 1;
	int8_t count = 0;

	while(1)
	{
		if(pgm_read_ptr(&self->currentMenu[index]) == 0)
			break;
		index += 2;
		count++;
	}
	return count;
}
void Menu_switchMenu(Menu* self, const MenuEntry* newMenu){
	self->currentMenu = newMenu;
	self->currentSelection = 0;
	self->debounceInput = true;
}
