#ifndef MENU_H_
#define MENU_H_

#include "Defines.h"

typedef const void* MenuData;
typedef const void* const MenuEntry;
typedef void (*MenuFn)(void);

typedef struct Menu
{
	const MenuEntry* currentMenu;
	int8_t currentSelection;
	int8_t debounceInput;
} Menu;

void Menu_init(Menu* self);
void Menu_draw(Menu* self);
void Menu_update(Menu* self);
int8_t Menu_numMenuItems(Menu* self);
void Menu_switchMenu(Menu* self, const MenuEntry* newMenu);

#if WOLF3D_FX_DIFFICULTY_FACES
void Menu_drawFace(uint8_t id, uint8_t x, uint8_t y);
#endif

void Menu_newGame(void);
void Menu_quit(void);
void Menu_skillBaby(void);
void Menu_skillEasy(void);
void Menu_skillMedium(void);
void Menu_skillHard(void);
void Menu_toggleSound(void);
void Menu_continueGame(void);
#if WOLF3D_HELP_MENU_ENABLE
void Menu_showHelp(void);
#endif
void Menu_nextLevel(void);

extern const MenuEntry Menu_Main[];
extern const MenuEntry Menu_Paused[];
extern const MenuEntry Menu_FloorComplete[];
extern const MenuEntry Menu_GameOver[];

#endif
