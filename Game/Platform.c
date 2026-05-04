#include "Engine.h"
#include "Platform.h"
#include "Sounds.h"

#if defined(__AVR__)
#include <avr/io.h>
#include <uzebox.h>
#if WOLF3D_ENABLE_UZEBOX_SFX
#include "../DataHeaders/Data_Audio.h"
#endif

extern Track tracks[];
extern void TriggerCommon(Track* track, u8 patch, u8 volume, u8 note);

static void Platform_triggerFx5(uint8_t patch, uint8_t volume)
{
	Track* track;

	track = &tracks[4];
	track->flags |= TRACK_FLAGS_PRIORITY;
	track->patchCommandStreamPos = NULL;
	TriggerCommon(track, patch, volume, 80);
	track->flags |= TRACK_FLAGS_PLAYING;
}
#else
uint8_t sBuffer[WOLF3D_SBUFFER_SIZE];
void ArduboyDisplay(void)
{
}
#endif

PlatformState Platform;

#if defined(__AVR__) && WOLF3D_ENABLE_UZEBOX_SFX
static const uint8_t Wolf3D_soundPatchMap[Sound_Count] PROGMEM =
{
	WOLF_PATCH_OPENDOORSND,
	WOLF_PATCH_CLOSEDOORSND,
	WOLF_PATCH_PUSHWALLSND,
	WOLF_PATCH_ATKKNIFESND,
	WOLF_PATCH_ATKPISTOLSND,
	WOLF_PATCH_ATKMACHINEGUNSND,
	WOLF_PATCH_NAZIFIRESND,
	WOLF_PATCH_GETAMMOSND,
	WOLF_PATCH_GETMACHINESND,
	WOLF_PATCH_HEALTH1SND,
	WOLF_PATCH_HITENEMYSND,
	WOLF_PATCH_DEATHSCREAM1SND,
	WOLF_PATCH_DOGBARKSND,
	WOLF_PATCH_DOGATTACKSND,
	WOLF_PATCH_DOGDEATHSND,
	WOLF_PATCH_SCHUTZADSND,
	WOLF_PATCH_SSFIRESND,
	WOLF_PATCH_NAZIFIRESND,
	WOLF_PATCH_BOSSFIRESND,
	WOLF_PATCH_LEBENSND,
	WOLF_PATCH_HALTSND,
	WOLF_PATCH_DEATHSCREAM2SND,
	WOLF_PATCH_DEATHSCREAM3SND,
	WOLF_PATCH_LEBENSND,
	WOLF_PATCH_TAKEDAMAGESND,
	WOLF_PATCH_PLAYERDEATHSND,
};

static const uint8_t Wolf3D_enemySoundPatchMap[Sound_Count] PROGMEM =
{
	WOLF_PATCH_OPENDOORSND,
	WOLF_PATCH_CLOSEDOORSND,
	WOLF_PATCH_PUSHWALLSND,
	WOLF_PATCH_ATKKNIFESND,
	WOLF_PATCH_ATKPISTOLSND,
	WOLF_PATCH_ATKMACHINEGUNSND,
	WOLF_PATCH_PCM_NAZIFIRESND,
	WOLF_PATCH_GETAMMOSND,
	WOLF_PATCH_GETMACHINESND,
	WOLF_PATCH_HEALTH1SND,
	WOLF_PATCH_PCM_HITENEMYSND,
	WOLF_PATCH_PCM_DEATHSCREAM1SND,
	WOLF_PATCH_PCM_DOGBARKSND,
	WOLF_PATCH_PCM_DOGATTACKSND,
	WOLF_PATCH_PCM_DOGDEATHSND,
	WOLF_PATCH_PCM_SCHUTZADSND,
	WOLF_PATCH_PCM_SSFIRESND,
	WOLF_PATCH_PCM_NAZIFIRESND,
	WOLF_PATCH_PCM_BOSSFIRESND,
	WOLF_PATCH_PCM_LEBENSND,
	WOLF_PATCH_PCM_HALTSND,
	WOLF_PATCH_PCM_DEATHSCREAM2SND,
	WOLF_PATCH_PCM_DEATHSCREAM3SND,
	WOLF_PATCH_PCM_LEBENSND,
	WOLF_PATCH_TAKEDAMAGESND,
	WOLF_PATCH_PLAYERDEATHSND,
};
#endif

static uint16_t Platform_sBufferIndex(uint8_t x, uint8_t y, uint8_t* mask)
{
	uint8_t sx = x + WOLF3D_SBUFFER_X_OFFSET;
	uint8_t sy = y + WOLF3D_SBUFFER_Y_OFFSET;
	*mask = (uint8_t)(1u << (sy & 7));
	return (uint16_t)sx + ((uint16_t)(sy >> 3) * WOLF3D_SBUFFER_WIDTH);
}

static uint8_t Platform_spanMask(uint8_t firstBit, uint8_t lastBit)
{
	uint8_t lowMask = (uint8_t)(0xffu << firstBit);
	uint8_t highMask = (uint8_t)(0xffu >> (7u - lastBit));
	return (uint8_t)(lowMask & highMask);
}

static uint8_t Platform_applyOutputPolarity(uint8_t colour)
{
#if WOLF3D_MODE23_INVERT_OUTPUT
	return colour ? 0 : 1;
#else
	return colour ? 1 : 0;
#endif
}

static uint8_t Platform_applyByteOutputPolarity(uint8_t value)
{
#if WOLF3D_MODE23_INVERT_OUTPUT
	return (uint8_t)~value;
#else
	return value;
#endif
}

#if WOLF3D_FAST_FLASH_UNROLLED_BACKGROUND && WOLF3D_FAST_ORIGINAL_ARDUBOY_BACKGROUND && defined(PLATFORM_UZEBOX_MODE23) && DISPLAYWIDTH == 128 && DISPLAYHEIGHT == 64 && WOLF3D_SBUFFER_WIDTH == 128 && WOLF3D_SBUFFER_HEIGHT == 64 && WOLF3D_SBUFFER_X_OFFSET == 0 && WOLF3D_SBUFFER_Y_OFFSET == 0

#if WOLF3D_MODE23_INVERT_OUTPUT
#define WOLF3D_BG_CLEAR_BYTE      0xff
#define WOLF3D_BG_XOR_EVEN_BYTE   0xaa
#define WOLF3D_BG_XOR_ODD_BYTE    0x55
#define WOLF3D_BG_FLOOR_EVEN_BYTE 0xff
#define WOLF3D_BG_FLOOR_ODD_BYTE  0x55
#else
#define WOLF3D_BG_CLEAR_BYTE      0x00
#define WOLF3D_BG_XOR_EVEN_BYTE   0x55
#define WOLF3D_BG_XOR_ODD_BYTE    0xaa
#define WOLF3D_BG_FLOOR_EVEN_BYTE 0x00
#define WOLF3D_BG_FLOOR_ODD_BYTE  0xaa
#endif

#if WOLF3D_DITHER_CEILING
#define WOLF3D_BG_CEILING_EVEN_BYTE WOLF3D_BG_XOR_EVEN_BYTE
#define WOLF3D_BG_CEILING_ODD_BYTE  WOLF3D_BG_XOR_ODD_BYTE
#else
#define WOLF3D_BG_CEILING_EVEN_BYTE WOLF3D_BG_CLEAR_BYTE
#define WOLF3D_BG_CEILING_ODD_BYTE  WOLF3D_BG_CLEAR_BYTE
#endif

#if WOLF3D_DITHER_FLOOR
#define WOLF3D_BG_FLOOR0_BYTE WOLF3D_BG_FLOOR_EVEN_BYTE
#define WOLF3D_BG_FLOOR1_BYTE WOLF3D_BG_FLOOR_ODD_BYTE
#else
#define WOLF3D_BG_FLOOR0_BYTE WOLF3D_BG_CLEAR_BYTE
#define WOLF3D_BG_FLOOR1_BYTE WOLF3D_BG_CLEAR_BYTE
#endif

/*
 * One half of the 128x64 Arduboy page buffer is 4 pages * 128 bytes.
 * The background patterns repeat every two bytes horizontally, so each half
 * is 256 byte-pairs. On AVR this emits straight-line flash code instead of
 * spending 1024 bytes of SRAM on a cached background image.
 */
static inline void Platform_fillMode23BackgroundAsm(void)
{
#if defined(__AVR__)
	uint8_t* dst = sBuffer;
	__asm__ __volatile__(
		"ldi r18, %[ceil0]" "\n\t"
		"ldi r19, %[ceil1]" "\n\t"
		".rept 256" "\n\t"
		"st Z+, r18" "\n\t"
		"st Z+, r19" "\n\t"
		".endr" "\n\t"
		"ldi r18, %[floor0]" "\n\t"
		"ldi r19, %[floor1]" "\n\t"
		".rept 256" "\n\t"
		"st Z+, r18" "\n\t"
		"st Z+, r19" "\n\t"
		".endr" "\n\t"
		: "+z" (dst)
		: [ceil0] "M" (WOLF3D_BG_CEILING_EVEN_BYTE),
		  [ceil1] "M" (WOLF3D_BG_CEILING_ODD_BYTE),
		  [floor0] "M" (WOLF3D_BG_FLOOR0_BYTE),
		  [floor1] "M" (WOLF3D_BG_FLOOR1_BYTE)
		: "r18", "r19", "memory"
	);
#else
	uint8_t page;
	uint8_t x;

	for(page = 0; page < 4; page++)
	{
		uint8_t* dst = &sBuffer[(uint16_t)page * WOLF3D_SBUFFER_WIDTH];
		for(x = 0; x < WOLF3D_SBUFFER_WIDTH; x += 2)
		{
			dst[x] = WOLF3D_BG_CEILING_EVEN_BYTE;
			dst[x + 1] = WOLF3D_BG_CEILING_ODD_BYTE;
		}
	}
	for(page = 4; page < 8; page++)
	{
		uint8_t* dst = &sBuffer[(uint16_t)page * WOLF3D_SBUFFER_WIDTH];
		for(x = 0; x < WOLF3D_SBUFFER_WIDTH; x += 2)
		{
			dst[x] = WOLF3D_BG_FLOOR0_BYTE;
			dst[x + 1] = WOLF3D_BG_FLOOR1_BYTE;
		}
	}
#endif
}
#endif

#if WOLF3D_FAST_RAM_BACKGROUND_CACHE && WOLF3D_FAST_ORIGINAL_ARDUBOY_BACKGROUND && defined(PLATFORM_UZEBOX_MODE23)
static uint8_t Wolf3D_backgroundCache[WOLF3D_SBUFFER_SIZE];

static void Platform_buildBackgroundCache(void)
{
	uint8_t page;
	uint8_t x;

#if WOLF3D_DITHER_CEILING
	for(page = 0; page < 4; page++)
	{
		uint8_t* dst = &Wolf3D_backgroundCache[(uint16_t)page * WOLF3D_SBUFFER_WIDTH];
		for(x = 0; x < WOLF3D_SBUFFER_WIDTH; x += 2)
		{
			dst[x] = Platform_applyByteOutputPolarity(0x55);
			dst[x + 1] = Platform_applyByteOutputPolarity(0xaa);
		}
	}
#else
	memset(Wolf3D_backgroundCache, Platform_applyByteOutputPolarity(0x00), WOLF3D_SBUFFER_WIDTH * 4);
#endif

#if WOLF3D_DITHER_FLOOR
	for(page = 4; page < 8; page++)
	{
		uint8_t* dst = &Wolf3D_backgroundCache[(uint16_t)page * WOLF3D_SBUFFER_WIDTH];
		for(x = 0; x < WOLF3D_SBUFFER_WIDTH; x += 2)
		{
			dst[x] = Platform_applyByteOutputPolarity(0x00);
			dst[x + 1] = Platform_applyByteOutputPolarity(0xaa);
		}
	}
#else
	memset(&Wolf3D_backgroundCache[WOLF3D_SBUFFER_WIDTH * 4], Platform_applyByteOutputPolarity(0x00), WOLF3D_SBUFFER_WIDTH * 4);
#endif
}
#endif

void Platform_init(void)
{
#if defined(__AVR__) && WOLF3D_ENABLE_UZEBOX_SFX
	InitMusicPlayer(patches);
#endif
#if WOLF3D_FAST_RAM_BACKGROUND_CACHE && WOLF3D_FAST_ORIGINAL_ARDUBOY_BACKGROUND && defined(PLATFORM_UZEBOX_MODE23)
	Platform_buildBackgroundCache();
#endif
	Platform.inputState = 0;
	Platform.muted = 0;
	Platform.lastSfxFrame = 0xffff;
	Platform.normalSfxTriggersThisFrame = 0;
	Platform.enemySfxTriggersThisFrame = 0;
	clearDisplay(0);
}

void Platform_updateInput(void)
{
#if defined(__AVR__)
	u16 joy = ReadJoypad(0);
	u16 input = 0;
	if(joy & BTN_UP) input |= Input_Dpad_Up;
	if(joy & BTN_RIGHT) input |= Input_Dpad_Right;
	if(joy & BTN_DOWN) input |= Input_Dpad_Down;
	if(joy & BTN_LEFT) input |= Input_Dpad_Left;
	if(joy & BTN_A) input |= Input_Btn_A;
	if(joy & BTN_B) input |= Input_Btn_B;
	if(joy & BTN_START) input |= Input_Btn_C;
#if defined(BTN_SELECT)
	if(joy & BTN_SELECT) input |= Input_Btn_Select;
#endif
	if(joy & BTN_SL) input |= Input_Btn_SL;
	if(joy & BTN_SR) input |= Input_Btn_SR;
	if(joy & BTN_X) input |= Input_Btn_X;
	if(joy & BTN_Y) input |= Input_Btn_Y;
	Platform.inputState = input;
#endif
}

uint16_t Platform_readInput(void)
{
	return Platform.inputState;
}

bool Platform_isMuted(void)
{
	return Platform.muted != 0;
}

void Platform_setMuted(bool muted)
{
	Platform.muted = muted ? 1 : 0;
	if(Platform.muted)
	{
		Platform_stopMusic();
	}
}

void Platform_startLevelMusic(void)
{
#if defined(__AVR__) && WOLF3D_ENABLE_UZEBOX_SFX
	if(!Platform.muted)
	{
		StartSong(e1m1_mus);
	}
#endif
}

void Platform_stopMusic(void)
{
#if defined(__AVR__) && WOLF3D_ENABLE_UZEBOX_SFX
	StopSong();
#endif
}

static uint8_t Platform_soundVolumeForSource(int16_t sourceX, int16_t sourceZ)
{
#if WOLF3D_SFX_DISTANCE_VOLUME
	uint16_t dx;
	uint16_t dz;
	uint8_t cellDist;
	uint8_t remaining;
	uint8_t span;
	uint16_t level;

	dx = (sourceX >= engine.player.x) ? (uint16_t)(sourceX - engine.player.x) : (uint16_t)(engine.player.x - sourceX);
	dz = (sourceZ >= engine.player.z) ? (uint16_t)(sourceZ - engine.player.z) : (uint16_t)(engine.player.z - sourceZ);
	cellDist = (uint8_t)((dx + dz) >> CELL_SIZE_SHIFT);

	if(cellDist >= WOLF3D_SFX_DISTANCE_CUTOFF_CELLS)
	{
		return 0;
	}

	remaining = (uint8_t)(WOLF3D_SFX_DISTANCE_CUTOFF_CELLS - cellDist);
	span = (uint8_t)(WOLF3D_SFX_VOLUME_NEAR - WOLF3D_SFX_VOLUME_FAR);

	/*
	 * Convert remaining range to a 0..255 gain, then optionally square it.
	 * Linear attenuation was technically working, but on the TV/speaker it was
	 * too subtle for mid-distance events.  The squared curve makes far guards,
	 * doors, and pushwalls audibly quieter while keeping nearby events loud.
	 */
	level = ((uint16_t)remaining * 255u) / WOLF3D_SFX_DISTANCE_CUTOFF_CELLS;
#if WOLF3D_SFX_DISTANCE_CURVE >= 2
	level = (level * level + 255u) >> 8;
#endif

	return (uint8_t)(WOLF3D_SFX_VOLUME_FAR + ((level * span + 255u) >> 8));
#else
	(void)sourceX;
	(void)sourceZ;
	return WOLF3D_SFX_VOLUME;
#endif
}

static void Platform_playSoundVolumeRouted(uint8_t soundId, uint8_t volume, bool enemyChannel)
{
	if(Platform.muted || volume == 0)
	{
		return;
	}

#if WOLF3D_SFX_MAX_TRIGGERS_PER_FRAME > 0
	/*
	 * Normal SFX and enemy-channel SFX use different hardware tracks, so do
	 * not let a player weapon sound consume the same per-frame trigger budget
	 * as an enemy death/alert/attack.  With one shared counter, killing an
	 * enemy on the same frame as the pistol/machinegun trigger silently dropped
	 * the enemy death sound.
	 */
	if(Platform.lastSfxFrame != (uint16_t)engine.frameCount)
	{
		Platform.lastSfxFrame = (uint16_t)engine.frameCount;
		Platform.normalSfxTriggersThisFrame = 0;
		Platform.enemySfxTriggersThisFrame = 0;
	}
	if(enemyChannel)
	{
		if(Platform.enemySfxTriggersThisFrame >= WOLF3D_SFX_MAX_TRIGGERS_PER_FRAME)
		{
			return;
		}
		Platform.enemySfxTriggersThisFrame++;
	}
	else
	{
		if(Platform.normalSfxTriggersThisFrame >= WOLF3D_SFX_MAX_TRIGGERS_PER_FRAME)
		{
			return;
		}
		Platform.normalSfxTriggersThisFrame++;
	}
#endif

#if defined(__AVR__) && WOLF3D_ENABLE_UZEBOX_SFX
	if(soundId < Sound_Count)
	{
		if(enemyChannel)
		{
			uint8_t patch = pgm_read_byte(&Wolf3D_enemySoundPatchMap[soundId]);
			Platform_triggerFx5(patch, volume);
		}
		else
		{
			uint8_t patch = pgm_read_byte(&Wolf3D_soundPatchMap[soundId]);
			TriggerFx(patch, volume, true);
		}
	}
#else
	(void)soundId;
	(void)volume;
	(void)enemyChannel;
#endif
}

static void Platform_playSoundVolume(uint8_t soundId, uint8_t volume)
{
	Platform_playSoundVolumeRouted(soundId, volume, false);
}

static void Platform_playEnemySoundVolume(uint8_t soundId, uint8_t volume)
{
	Platform_playSoundVolumeRouted(soundId, volume, true);
}

void Platform_playSoundAt(uint8_t soundId, int16_t sourceX, int16_t sourceZ)
{
	Platform_playSoundVolume(soundId, Platform_soundVolumeForSource(sourceX, sourceZ));
}

void Platform_playSoundCell(uint8_t soundId, int8_t cellX, int8_t cellZ)
{
	Platform_playSoundAt(soundId, CELL_TO_WORLD(cellX) + CELL_SIZE / 2, CELL_TO_WORLD(cellZ) + CELL_SIZE / 2);
}

void Platform_playEnemySoundAt(uint8_t soundId, int16_t sourceX, int16_t sourceZ)
{
	Platform_playEnemySoundVolume(soundId, Platform_soundVolumeForSource(sourceX, sourceZ));
}

void Platform_playEnemySoundCell(uint8_t soundId, int8_t cellX, int8_t cellZ)
{
	Platform_playEnemySoundAt(soundId, CELL_TO_WORLD(cellX) + CELL_SIZE / 2, CELL_TO_WORLD(cellZ) + CELL_SIZE / 2);
}

void Platform_playSound(uint8_t soundId)
{
	Platform_playSoundVolume(soundId, WOLF3D_SFX_VOLUME_NEAR);
}

void Wolf3D_beginFrame(void)
{
#if WOLF3D_CLEAR_EACH_FRAME
	if(engine.gameState != GameState_Playing)
	{
		clearDisplay(0);
	}
#endif
}


void Platform_drawRleBitmap128x64(const uint8_t* data, uint16_t len)
{
	uint16_t src = 0;
	uint16_t dst = 0;

	while(src + 1u < len && dst < WOLF3D_SBUFFER_SIZE)
	{
		uint8_t count = pgm_read_byte(&data[src++]);
		uint8_t value = pgm_read_byte(&data[src++]);
#if !WOLF3D_MODE23_INVERT_OUTPUT
		value = (uint8_t)~value;
#endif
		while(count && dst < WOLF3D_SBUFFER_SIZE)
		{
			sBuffer[dst++] = value;
			count--;
		}
	}

	while(dst < WOLF3D_SBUFFER_SIZE)
	{
#if WOLF3D_MODE23_INVERT_OUTPUT
		sBuffer[dst++] = 0x00;
#else
		sBuffer[dst++] = 0xff;
#endif
	}
}

void clearDisplay(uint8_t colour)
{
	uint8_t physical = Platform_applyOutputPolarity(colour) ? 0xff : 0x00;
	memset(sBuffer, physical, WOLF3D_SBUFFER_SIZE);
}

void drawPixel(uint8_t x, uint8_t y, uint8_t colour)
{
	uint8_t mask;
	uint16_t index;

	if(x >= DISPLAYWIDTH || y >= DISPLAYHEIGHT)
	{
		return;
	}

	index = Platform_sBufferIndex(x, y, &mask);
	if(Platform_applyOutputPolarity(colour))
	{
		sBuffer[index] |= mask;
	}
	else
	{
		sBuffer[index] &= (uint8_t)~mask;
	}
}

void setPixel(uint8_t x, uint8_t y)
{
	drawPixel(x, y, 1);
}

void clearPixel(uint8_t x, uint8_t y)
{
	drawPixel(x, y, 0);
}

void drawVerticalSpan(uint8_t x, int16_t y0, int16_t y1, uint8_t colour)
{
	uint8_t sx;
	int16_t sy0;
	int16_t sy1;

	if(x >= DISPLAYWIDTH)
	{
		return;
	}
	if(y0 > y1)
	{
		int16_t tmp = y0;
		y0 = y1;
		y1 = tmp;
	}
	if(y1 < 0 || y0 >= DISPLAYHEIGHT)
	{
		return;
	}
	if(y0 < 0)
	{
		y0 = 0;
	}
	if(y1 >= DISPLAYHEIGHT)
	{
		y1 = DISPLAYHEIGHT - 1;
	}

	sx = x + WOLF3D_SBUFFER_X_OFFSET;
	sy0 = y0 + WOLF3D_SBUFFER_Y_OFFSET;
	sy1 = y1 + WOLF3D_SBUFFER_Y_OFFSET;

	while(sy0 <= sy1)
	{
		uint8_t page = (uint8_t)(sy0 >> 3);
		uint8_t firstBit = (uint8_t)(sy0 & 7);
		uint8_t lastBit = 7;
		int16_t pageLastY = ((int16_t)page << 3) + 7;
		uint16_t index;
		uint8_t mask;

		if(pageLastY > sy1)
		{
			pageLastY = sy1;
			lastBit = (uint8_t)(sy1 & 7);
		}

		mask = Platform_spanMask(firstBit, lastBit);
		index = (uint16_t)sx + ((uint16_t)page * WOLF3D_SBUFFER_WIDTH);
		if(Platform_applyOutputPolarity(colour))
		{
			sBuffer[index] |= mask;
		}
		else
		{
			sBuffer[index] &= (uint8_t)~mask;
		}

		sy0 = pageLastY + 1;
	}
}


void Platform_fillOriginalArduboyBackground(void)
{
#if WOLF3D_FAST_FLASH_UNROLLED_BACKGROUND && WOLF3D_FAST_ORIGINAL_ARDUBOY_BACKGROUND && defined(PLATFORM_UZEBOX_MODE23) && DISPLAYWIDTH == 128 && DISPLAYHEIGHT == 64 && WOLF3D_SBUFFER_WIDTH == 128 && WOLF3D_SBUFFER_HEIGHT == 64 && WOLF3D_SBUFFER_X_OFFSET == 0 && WOLF3D_SBUFFER_Y_OFFSET == 0
	Platform_fillMode23BackgroundAsm();
	return;
#endif
#if WOLF3D_FAST_RAM_BACKGROUND_CACHE && WOLF3D_FAST_ORIGINAL_ARDUBOY_BACKGROUND && DISPLAYWIDTH == 128 && DISPLAYHEIGHT == 64 && WOLF3D_SBUFFER_WIDTH == 128 && WOLF3D_SBUFFER_HEIGHT == 64 && WOLF3D_SBUFFER_X_OFFSET == 0 && WOLF3D_SBUFFER_Y_OFFSET == 0
	memcpy(sBuffer, Wolf3D_backgroundCache, WOLF3D_SBUFFER_SIZE);
	return;
#endif
#if WOLF3D_FAST_ORIGINAL_ARDUBOY_BACKGROUND && DISPLAYWIDTH == 128 && DISPLAYHEIGHT == 64 && WOLF3D_SBUFFER_WIDTH == 128 && WOLF3D_SBUFFER_HEIGHT == 64 && WOLF3D_SBUFFER_X_OFFSET == 0 && WOLF3D_SBUFFER_Y_OFFSET == 0
	uint8_t page;
	uint8_t x;

	/*
	 * Fast Mode 23 page-layout background.
	 *
	 * Logical byte patterns before output-polarity inversion:
	 *   clear:      0x00
	 *   xor dither: even x = 0x55, odd x = 0xaa
	 *   floor dot:  even x = 0x00, odd x = 0xaa
	 *
	 * WOLF3D_DITHER_CEILING is intentionally separate.  The original Arduboy
	 * path used a flat clear ceiling, but the Uzebox 1bpp picture reads better
	 * with a stippled ceiling.
	 */
#if WOLF3D_DITHER_CEILING
	for(page = 0; page < 4; page++)
	{
		uint8_t* dst = &sBuffer[(uint16_t)page * WOLF3D_SBUFFER_WIDTH];
		for(x = 0; x < WOLF3D_SBUFFER_WIDTH; x += 2)
		{
			dst[x] = Platform_applyByteOutputPolarity(0x55);
			dst[x + 1] = Platform_applyByteOutputPolarity(0xaa);
		}
	}
#else
	memset(sBuffer, Platform_applyByteOutputPolarity(0x00), WOLF3D_SBUFFER_WIDTH * 4);
#endif

#if WOLF3D_DITHER_FLOOR
	for(page = 4; page < 8; page++)
	{
		uint8_t* dst = &sBuffer[(uint16_t)page * WOLF3D_SBUFFER_WIDTH];
		for(x = 0; x < WOLF3D_SBUFFER_WIDTH; x += 2)
		{
			dst[x] = Platform_applyByteOutputPolarity(0x00);
			dst[x + 1] = Platform_applyByteOutputPolarity(0xaa);
		}
	}
#else
	memset(&sBuffer[WOLF3D_SBUFFER_WIDTH * 4], Platform_applyByteOutputPolarity(0x00), WOLF3D_SBUFFER_WIDTH * 4);
#endif
#else
	for(uint8_t x = 0; x < DISPLAYWIDTH; x++)
	{
		for(uint8_t y = 0; y < DISPLAYHEIGHT; y++)
		{
			uint8_t colour;

			if(y < HALF_DISPLAYHEIGHT)
			{
#if WOLF3D_DITHER_CEILING
				colour = ((x ^ y) & 1) ? 0 : 1;
#else
				colour = 0;
#endif
			}
			else
			{
#if WOLF3D_DITHER_FLOOR
				colour = ((x & y) & 1) ? 1 : 0;
#else
				colour = 0;
#endif
			}

			drawPixel(x, y, colour);
		}
	}
#endif
}

void Wolf3D_presentDeferredFrame(void)
{
	ArduboyDisplay();
}
