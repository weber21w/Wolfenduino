#ifndef DATA_GUI_H_
#define DATA_GUI_H_

#include <stdint.h>
#include "Defines.h"

/*
 * Keep PROGMEM only on the storage definition in Game/Data_GUI.c.
 * Some avr-libc/toolchain combinations do not expose PROGMEM early enough
 * for extern declarations, and the attribute is not needed on the extern.
 */

/* 128x64 page-buffer RLE. Pairs are count, byte. Byte bits mark dark/foreground pixels. */
#define Wolf3D_TitleRle_LEN 610
extern const uint8_t Wolf3D_TitleRle[];

#if WOLF3D_FX_HELP_BITMAP
#define Wolf3D_HelpRle_LEN 550
extern const uint8_t Wolf3D_HelpRle[];
#endif

#if WOLF3D_FX_DIFFICULTY_FACES
#define WOLF3D_GUI_FACE_WIDTH 23
#define WOLF3D_GUI_FACE_HEIGHT 32
#define WOLF3D_GUI_FACE_PAGE_BYTES (WOLF3D_GUI_FACE_WIDTH * (WOLF3D_GUI_FACE_HEIGHT / 8))
#define WOLF3D_GUI_FACE_BABY 0
#define WOLF3D_GUI_FACE_EASY 1
#define WOLF3D_GUI_FACE_MEDIUM 2
#define WOLF3D_GUI_FACE_HARD 3
#define WOLF3D_GUI_FACE_DEAD 4
#define WOLF3D_GUI_FACE_NORMAL 5
#define WOLF3D_GUI_FACE_COUNT 6
/* Face bitmap storage is local to Game/Menu.c. */
#endif

#endif
