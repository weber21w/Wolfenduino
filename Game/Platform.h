#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "Defines.h"

#define INPUT_BIT(x) (1u << (x))

#define WOLF3D_SBUFFER_WIDTH 128
#define WOLF3D_SBUFFER_HEIGHT 64
#define WOLF3D_SBUFFER_SIZE (WOLF3D_SBUFFER_WIDTH * (WOLF3D_SBUFFER_HEIGHT / 8))
#define WOLF3D_SBUFFER_X_OFFSET ((WOLF3D_SBUFFER_WIDTH - DISPLAYWIDTH) / 2)
#define WOLF3D_SBUFFER_Y_OFFSET ((WOLF3D_SBUFFER_HEIGHT - DISPLAYHEIGHT) / 2)

enum
{
	Input_Dpad_Up		= INPUT_BIT(0),
	Input_Dpad_Right	= INPUT_BIT(1),
	Input_Dpad_Down		= INPUT_BIT(2),
	Input_Dpad_Left		= INPUT_BIT(3),
	Input_Btn_A			= INPUT_BIT(4),
	Input_Btn_B			= INPUT_BIT(5),
	Input_Btn_C			= INPUT_BIT(6),
	Input_Btn_SL		= INPUT_BIT(7),
	Input_Btn_SR		= INPUT_BIT(8),
	Input_Btn_X			= INPUT_BIT(9),
	Input_Btn_Y			= INPUT_BIT(10),
	Input_Btn_Select	= INPUT_BIT(11),
};

typedef struct PlatformState
{
	uint16_t inputState;
	uint8_t muted;
	uint16_t lastSfxFrame;
	uint8_t sfxTriggersThisFrame;
} PlatformState;

extern PlatformState Platform;

#if defined(__AVR__)
extern uint8_t sBuffer[];
void ArduboyDisplay(void);
#else
extern uint8_t sBuffer[WOLF3D_SBUFFER_SIZE];
void ArduboyDisplay(void);
#endif

void Platform_init(void);
void Platform_updateInput(void);
uint16_t Platform_readInput(void);
bool Platform_isMuted(void);
void Platform_setMuted(bool muted);
void Platform_startLevelMusic(void);
void Platform_stopMusic(void);
void Platform_playSound(uint8_t soundId);
void Platform_playSoundAt(uint8_t soundId, int16_t sourceX, int16_t sourceZ);
void Platform_playSoundCell(uint8_t soundId, int8_t cellX, int8_t cellZ);
void Platform_playEnemySoundAt(uint8_t soundId, int16_t sourceX, int16_t sourceZ);
void Platform_playEnemySoundCell(uint8_t soundId, int8_t cellX, int8_t cellZ);

void Wolf3D_beginFrame(void);
void clearDisplay(uint8_t colour);
void Platform_drawRleBitmap128x64(const uint8_t* data, uint16_t len);
void setPixel(uint8_t x, uint8_t y);
void clearPixel(uint8_t x, uint8_t y);
void drawPixel(uint8_t x, uint8_t y, uint8_t colour);
void drawVerticalSpan(uint8_t x, int16_t y0, int16_t y1, uint8_t colour);
void Platform_fillOriginalArduboyBackground(void);
void Wolf3D_presentDeferredFrame(void);

#endif
