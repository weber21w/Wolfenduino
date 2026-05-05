#ifndef DEFINES_H_
#define DEFINES_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define PLATFORM_UZEBOX 1
#define WOLF3D_AVR_ONLY_BUILD 1
#define PROGMEM_MAP_STREAMING 1
#define WOLF3D_COMPRESSED_MAP_META 1
#define WOLF3D_COMPRESSED_MAP_DATA 1
#define DEFER_RENDER 1
#define DISPLAYWIDTH 128
#define DISPLAYHEIGHT 64
#define PLATFORM_UZEBOX_MODE23 1
#define EMULATE_ARDUBOY 1
#define USE_SIMPLE_COLLISIONS 1
#define WOLF3D_CLEAR_EACH_FRAME 0
#define WOLF3D_START_CHEAT_ALL_WEAPONS 0
#define WOLF3D_TITLE_SCREEN_ENABLE 0
#define WOLF3D_FX_TEXT_GUI 1
#define WOLF3D_FX_TITLE_BITMAP 1
#define WOLF3D_TITLE_MENU_BACKGROUND 1
#define WOLF3D_FX_DIFFICULTY_FACES 1
#define WOLF3D_HELP_MENU_ENABLE 0
#define WOLF3D_FX_HELP_BITMAP 0
#ifndef WOLF3D_ENABLE_BOSS
#define WOLF3D_ENABLE_BOSS 0
#endif

/*
 * Flash content tier.  The ATmega644 build cannot fit the full 8-level
 * guard/dog/SS set plus the rest of the port, so default to the first few
 * WolfenduinoFX levels.  If boss support is enabled, the fourth compiled
 * level is the WolfenduinoFX boss floor.
 */
#if WOLF3D_ENABLE_BOSS
#define WOLF3D_FLASH_LEVEL_COUNT 5
#else
#define WOLF3D_FLASH_LEVEL_COUNT 4
#endif
#define WOLF3D_MENU_START_SELECTS 1
#define WOLF3D_MODE23_INVERT_OUTPUT 1
#define WOLF3D_FAST_ORIGINAL_ARDUBOY_BACKGROUND 1
#define WOLF3D_HOLD_FIRE_REPEATS 1
#define WOLF3D_DITHER_CEILING 1
#define WOLF3D_DITHER_FLOOR 1
#define WOLF3D_FAST_CLIPPED_WALL_STRIPS 1
#define WOLF3D_FAST_CLIPPED_SPRITES 1
#define WOLF3D_SPRITE_GRAY_STIPPLE 1
#define WOLF3D_WALL_GRAY_STIPPLE 1
#define WOLF3D_WALL_SIDE_STIPPLE 1
#define WOLF3D_WALL_VERTICAL_FLAG 0x80
#define WOLF3D_WALL_TEXTURE_MASK 0x7f
#define WOLF3D_FONT_FOREGROUND_COLOUR 0
#define WOLF3D_FONT_BACKGROUND_COLOUR 1
#define WOLF3D_MAP_TOGGLE_ENABLE 1
#define WOLF3D_MAP_DEDICATED_SCREEN 1
#define WOLF3D_MAP_MIN_SCALE 1
#define WOLF3D_MAP_MAX_SCALE 5
#define WOLF3D_MAP_DEFAULT_SCALE 4
#define WOLF3D_FAST_FULLSCREEN_MAP_DRAW 1

#define WOLF3D_MAP_PLAYER_ARROW 1
#define WOLF3D_MAP_ARROW_LEN_BASE 2
#define WOLF3D_MAP_ARROW_LEN_SCALE 1
#define WOLF3D_MAP_ARROW_LEN_MAX 8
#define WOLF3D_MAP_ARROW_WING_MIN 1
#define WOLF3D_MAP_ARROW_WING_DIV 2

/*
 * Automap visibility for the no-SPI-RAM build.
 * One bit represents a 2x2 block of map cells: 32x32 bits = 128 bytes SRAM.
 */
#define WOLF3D_SEEN_WALL_BITSET 1
#define WOLF3D_MAP_ONLY_SEEN_WALLS 1
#define WOLF3D_SEEN_WALL_GRANULARITY 2
#define WOLF3D_SEEN_WALL_DIM ((MAP_SIZE + WOLF3D_SEEN_WALL_GRANULARITY - 1) / WOLF3D_SEEN_WALL_GRANULARITY)
#define WOLF3D_SEEN_WALL_BYTES ((WOLF3D_SEEN_WALL_DIM * WOLF3D_SEEN_WALL_DIM + 7) / 8)

#define WOLF3D_MAP_SHOW_ACTORS 0
#define WOLF3D_MAP_SHOW_ITEMS 0
#define WOLF3D_FULL_MAP_RAM_CACHE 0
#define WOLF3D_MAP_SCALE 3
#define WOLF3D_MAP_X 40
#define WOLF3D_MAP_Y 8
#define WOLF3D_FAST_TILE_FIRST_RENDER 1
#define WOLF3D_FAST_COLUMN_PIXEL_WRITER 1
#define WOLF3D_FAST_WALL_PAGE_BLIT 1
#define WOLF3D_FORCE_WALL_STRIP_EDGES 1
#define WOLF3D_FAST_DEFERRED_WALL_BOUNDS 1
#define WOLF3D_FAST_NEIGHBOR_TILE_READS 1
#define WOLF3D_FAST_WALL_COLOUR_LUT 1
#ifndef WOLF3D_ENABLE_UZEBOX_SFX
#define WOLF3D_ENABLE_UZEBOX_SFX 1
#endif
#define WOLF3D_SFX_VOLUME 0xff
#define WOLF3D_SFX_DISTANCE_VOLUME 1
#define WOLF3D_SFX_VOLUME_NEAR 0xff
#define WOLF3D_SFX_VOLUME_FAR 0x00
#define WOLF3D_SFX_DISTANCE_CUTOFF_CELLS 20
#define WOLF3D_SFX_DISTANCE_CURVE 2
#define WOLF3D_SFX_MAX_TRIGGERS_PER_FRAME 1

#define WOLF3D_ACTOR_LOS_THROTTLE 1
#define WOLF3D_ACTOR_LOS_FRAME_MASK 1
#define WOLF3D_HUD_FLASH_FRAMES 10
#define WOLF3D_PLAYER_START_LIVES 3
#define WOLF3D_GET_PSYCHED_FRAMES 120

/*
 * Big flash-for-speed wall lookup package.
 * Default ON.  Set this one define to 0 to recover roughly 22 KB of flash.
 * It controls both the unpacked 16x16 wall texel table and the 256x64
 * vertical texture-coordinate table.  The individual knobs may still be
 * overridden before this header is included if needed.
 */
#define WOLF3D_FAST_WALL_LUTS 0
#ifndef WOLF3D_FAST_UNPACKED_WALL_TEXTURES
#define WOLF3D_FAST_UNPACKED_WALL_TEXTURES WOLF3D_FAST_WALL_LUTS
#endif
#ifndef WOLF3D_FAST_WALL_V_LOOKUP
#define WOLF3D_FAST_WALL_V_LOOKUP WOLF3D_FAST_WALL_LUTS
#endif

#define WOLF3D_FAST_RAM_BACKGROUND_CACHE 0
#define WOLF3D_FAST_FLASH_UNROLLED_BACKGROUND 1
#define WOLF3D_SLOW_DEATH_TURN 1
#define WOLF3D_DEATH_TURN_FRAME_MASK 1
#define WOLF3D_DEATH_STATIC_FRAMES 75
#define WOLF3D_DEATH_MESSAGE_FRAMES 90
#define WOLF3D_DEATH_TOTAL_FRAMES (WOLF3D_DEATH_STATIC_FRAMES + WOLF3D_DEATH_MESSAGE_FRAMES)

#ifndef PROGMEM
#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#define PROGMEM
#define PSTR(x) (x)
#define pgm_read_byte(x) (*((const uint8_t*)(x)))
#define pgm_read_word(x) (*((const uint16_t*)(x)))
#define pgm_read_ptr(x) (*((const void* const*)(x)))
#endif
#endif

#ifndef PSTR
#define PSTR(x) (x)
#endif
#ifndef pgm_read_ptr
#define pgm_read_ptr(x) ((const void*)pgm_read_word(x))
#endif

#define WARNING(msg, ...) do { (void)0; } while(0)
#define ERROR(msg)

#ifndef NULL
#define NULL ((void*)0)
#endif

#if !defined(max) && !defined(min)
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define min3(a, b, c) (min(min(a, b), c))
#define max3(a, b, c) (max(max(a, b), c))
#define sign(x) ((x) < 0 ? -1 : 1)
#define mabs(x) ((x) < 0 ? -(x) : (x))

#define HALF_DISPLAYWIDTH (DISPLAYWIDTH >> 1)
#define HALF_DISPLAYHEIGHT (DISPLAYHEIGHT >> 1)

#define CELL_SIZE 32
#define CELL_SIZE_SHIFT 5
#define CELL_TO_WORLD(x) ((x) << CELL_SIZE_SHIFT)
#define WORLD_TO_CELL(x) ((x) >> CELL_SIZE_SHIFT)

#define MAP_SIZE 64
#define MAP_BUFFER_SIZE 16

#define TEXTURE_SIZE 16
#define TEXTURE_STRIDE 4

#define FOV 44
#define HALF_FOV 22
#define CULLING_FOV 35
#define DRAW_DISTANCE (CELL_SIZE * 24)

#define NEAR_PLANE_MULTIPLIER 222
#define NEAR_PLANE (DISPLAYWIDTH * NEAR_PLANE_MULTIPLIER / 256)

#define CLIP_PLANE 1
#define SPRITE_CLIP_PLANE 16
#define CAMERA_SCALE 1
#define MOVEMENT 7
#define TURN 3
#define MIN_WALL_DISTANCE 8
#define MAX_DOORS 20

#define DOOR_FRAME_TEXTURE 19

#define MAX_ACTIVE_ACTORS 5
#define MAX_ACTIVE_ITEMS 12

#define EMPTY_ITEM_SLOT 0xff
#define DYNAMIC_ITEM_ID 0xfe

#define ACTOR_HITBOX_SIZE 16
#define MIN_ACTOR_DISTANCE 32

#define FIRST_FONT_GLYPH 32
#define LAST_FONT_GLYPH 95
#define FONT_WIDTH 3
#define FONT_HEIGHT 5
#define FONT_GLYPH_BYTE_SIZE 2

#endif
