#include "Engine.h"

#if defined(__AVR__)
#include <avr/io.h>
#include <avr/interrupt.h>
#include <uzebox.h>
#endif


#if defined(__AVR__)
static uint16_t Game_GetVsyncCounterStable(void)
{
	uint16_t a;
	uint16_t b;

	do
	{
		a = GetVsyncCounter();
		b = GetVsyncCounter();
	}
	while(a != b);

	return b;
}

static void Game_WaitFor30HzFrameSlot(void)
{
	static uint16_t lastFrameCounter;
	static bool initialized = false;
	uint16_t now;

	if(!initialized)
	{
		lastFrameCounter = Game_GetVsyncCounterStable();
		initialized = true;
	}

	do
	{
		now = Game_GetVsyncCounterStable();
	}
	while((uint16_t)(now - lastFrameCounter) < 2);

	lastFrameCounter = now;
}
#endif

static uint8_t Game_PgmStringWidth(const char* str)
{
	uint8_t len = 0;

	while(pgm_read_byte(str + len) != 0)
	{
		len++;
	}

	return (uint8_t)(len * (FONT_WIDTH + 1));
}

static void Game_DrawBootIntroStringCentered(const char* str, uint8_t y)
{
	uint8_t width = Game_PgmStringWidth(str);
	uint8_t x = 0;

	if(width < DISPLAYWIDTH)
	{
		x = (uint8_t)((DISPLAYWIDTH - width) >> 1);
	}

	Renderer_drawString(&engine.renderer, str, x, y);
}

static void Game_ShowBootIntro(void)
{
	clearDisplay(WOLF3D_FONT_BACKGROUND_COLOUR);
	Game_DrawBootIntroStringCentered(PSTR("GAME BY:"), 8);
	Game_DrawBootIntroStringCentered(PSTR("JHHOWARD(@JAMESHOWARD)"), 18);
	Game_DrawBootIntroStringCentered(PSTR("UZEBOX PORT:"), 38);
	Game_DrawBootIntroStringCentered(PSTR("D3THADD3R(LEE WEBER)"), 48);

#if defined(__AVR__)
	FadeOut(0, true);
#endif
	Wolf3D_presentDeferredFrame();
#if defined(__AVR__)
	FadeIn(1, true);
	WaitVsync(120);
	FadeOut(3, true);
	FadeIn(1, false);
#endif
}

int main(void)
{
#if defined(__AVR__)
	ClearVram();
#endif
	Platform_init();
	Engine_init();
	Game_ShowBootIntro();
	for(;;)
	{
#if defined(__AVR__)
		Game_WaitFor30HzFrameSlot();
#endif
		Platform_updateInput();
		Engine_update();
		Wolf3D_beginFrame();
		Engine_draw();
#ifdef DEFER_RENDER
		if(engine.gameState == GameState_Playing
#if WOLF3D_MAP_DEDICATED_SCREEN
			&& !engine.mapVisible
#endif
		)
		{
			Renderer_drawDeferredFrame(&engine.renderer);
		}
#endif
		Wolf3D_presentDeferredFrame();
	}
	return 0;
}
