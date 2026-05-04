#include "Engine.h"
#include "Renderer.h"
#include "FixedMath.h"
#include "TileTypes.h"

#include "../DataHeaders/Data_Walls.h"
#if WOLF3D_FAST_UNPACKED_WALL_TEXTURES && WOLF3D_FAST_WALL_V_LOOKUP
#include "../DataHeaders/Data_WallLUT.h"
#endif
#include "../DataHeaders/Data_Pistol.h"
#include "../DataHeaders/Data_Knife.h"
#include "../DataHeaders/Data_Machinegun.h"
#include "../DataHeaders/Data_Decorations.h"
#include "../DataHeaders/Data_BlockingDecorations.h"
#include "../DataHeaders/Data_Items.h"
#include "../DataHeaders/Data_Font.h"

#if defined(PLATFORM_UZEBOX_MODE23)
static inline uint8_t Renderer_physicalColour(uint8_t colour)
{
#if WOLF3D_MODE23_INVERT_OUTPUT
	return colour ? 0 : 1;
#else
	return colour ? 1 : 0;
#endif
}

static inline void Renderer_writePixelFast(uint8_t x, uint8_t y, uint8_t colour)
{
	uint8_t sx = (uint8_t)(x + WOLF3D_SBUFFER_X_OFFSET);
	uint8_t sy = (uint8_t)(y + WOLF3D_SBUFFER_Y_OFFSET);
	uint16_t index = (uint16_t)sx + (((uint16_t)sy >> 3) * WOLF3D_SBUFFER_WIDTH);
	uint8_t mask = (uint8_t)(1u << (sy & 7));

	if(Renderer_physicalColour(colour))
	{
		sBuffer[index] |= mask;
	}
	else
	{
		sBuffer[index] &= (uint8_t)~mask;
	}
}

static inline void Renderer_writePixelFastClipped(int16_t x, int16_t y, uint8_t colour)
{
	if(x >= 0 && x < DISPLAYWIDTH && y >= 0 && y < DISPLAYHEIGHT)
	{
		Renderer_writePixelFast((uint8_t)x, (uint8_t)y, colour);
	}
}

static inline void Renderer_writeColumnPixelPtrFast(uint8_t** dst, uint8_t* mask, uint8_t colour)
{
	if(Renderer_physicalColour(colour))
	{
		**dst |= *mask;
	}
	else
	{
		**dst &= (uint8_t)~(*mask);
	}

	*mask <<= 1;
	if(*mask == 0)
	{
		*mask = 1;
		*dst += WOLF3D_SBUFFER_WIDTH;
	}
}

#if WOLF3D_FAST_WALL_PAGE_BLIT
static inline void Renderer_mergePageByte(uint8_t* dst, uint8_t writeMask, uint8_t valueMask)
{
	*dst = (uint8_t)((*dst & (uint8_t)~writeMask) | valueMask);
}
#endif
#else
static inline void Renderer_writePixelFast(uint8_t x, uint8_t y, uint8_t colour)
{
	drawPixel(x, y, colour);
}

static inline void Renderer_writePixelFastClipped(int16_t x, int16_t y, uint8_t colour)
{
	if(x >= 0 && x < DISPLAYWIDTH && y >= 0 && y < DISPLAYHEIGHT)
	{
		drawPixel((uint8_t)x, (uint8_t)y, colour);
	}
}
#endif

static inline void Renderer_fillRectFast(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t colour)
{
	for(uint8_t yy = 0; yy < h; yy++)
	{
		for(uint8_t xx = 0; xx < w; xx++)
		{
			Renderer_writePixelFastClipped((int16_t)x + xx, (int16_t)y + yy, colour);
		}
	}
}


static inline void Renderer_maskedPageWrite(uint8_t* dst, uint8_t mask, uint8_t colour)
{
#if defined(PLATFORM_UZEBOX_MODE23)
	uint8_t value = Renderer_physicalColour(colour) ? mask : 0;
	*dst = (uint8_t)((*dst & (uint8_t)~mask) | value);
#else
	(void)dst;
	(void)mask;
	(void)colour;
#endif
}

static inline void Renderer_fillRectPageFast(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t colour)
{
#if defined(PLATFORM_UZEBOX_MODE23) && WOLF3D_FAST_FULLSCREEN_MAP_DRAW
	uint8_t sx0;
	uint8_t sx1;
	uint8_t yEnd;

	if(w == 0 || h == 0 || x >= DISPLAYWIDTH || y >= DISPLAYHEIGHT)
	{
		return;
	}
	if((uint16_t)x + w > DISPLAYWIDTH)
	{
		w = (uint8_t)(DISPLAYWIDTH - x);
	}
	if((uint16_t)y + h > DISPLAYHEIGHT)
	{
		h = (uint8_t)(DISPLAYHEIGHT - y);
	}

	sx0 = (uint8_t)(x + WOLF3D_SBUFFER_X_OFFSET);
	sx1 = (uint8_t)(sx0 + w);
	y = (uint8_t)(y + WOLF3D_SBUFFER_Y_OFFSET);
	yEnd = (uint8_t)(y + h - 1);

	while(y <= yEnd)
	{
		uint8_t page = (uint8_t)(y >> 3);
		uint8_t firstBit = (uint8_t)(y & 7);
		uint8_t lastBit = 7;
		uint8_t pageLast = (uint8_t)((page << 3) + 7);
		uint8_t mask;
		uint8_t sx;

		if(pageLast > yEnd)
		{
			pageLast = yEnd;
			lastBit = (uint8_t)(yEnd & 7);
		}

		mask = (uint8_t)((uint8_t)(0xffu << firstBit) & (uint8_t)(0xffu >> (7u - lastBit)));
		for(sx = sx0; sx < sx1; sx++)
		{
			Renderer_maskedPageWrite(&sBuffer[(uint16_t)sx + ((uint16_t)page * WOLF3D_SBUFFER_WIDTH)], mask, colour);
		}
		y = (uint8_t)(pageLast + 1);
	}
#else
	Renderer_fillRectFast(x, y, w, h, colour);
#endif
}

static inline void Renderer_fillMapCellFast(uint8_t x, uint8_t y, uint8_t scale, uint8_t colour)
{
	Renderer_fillRectPageFast(x, y, scale, scale, colour);
}

static inline uint8_t Renderer_xorStippleColour(uint8_t x, uint8_t y)
{
	return ((x ^ y) & 1) ? 0 : 1;
}

static inline uint8_t Renderer_andStippleColour(uint8_t x, uint8_t y)
{
	return ((x & y) & 1) ? 1 : 0;
}

#if WOLF3D_FAST_WALL_COLOUR_LUT
/*
 * 16-byte SRAM lookup: index = texel*4 + xParity*2 + yParity.
 * This removes a per-pixel switch from the wall-strip inner loop.
 */
static const uint8_t Renderer_wallLogicalColourLut[16] =
{
#if WOLF3D_WALL_GRAY_STIPPLE
	1, 0, 0, 1,	/* texel 0: xor stipple gray */
	0, 0, 0, 0,	/* texel 1: black */
	1, 1, 1, 1,	/* texel 2: white */
	0, 0, 0, 1	/* texel 3: dark stipple */
#else
	0, 0, 0, 0,
	0, 0, 0, 0,
	1, 1, 1, 1,
	1, 1, 1, 1
#endif
};
#endif

static inline uint8_t Renderer_wallTexColourFast(uint8_t xParity, uint8_t yParity, uint8_t texData)
{
#if WOLF3D_FAST_WALL_COLOUR_LUT
	return Renderer_wallLogicalColourLut[((texData & 3) << 2) | (xParity << 1) | yParity];
#else
	switch(texData)
	{
	case 1:
		return 0;
	case 2:
		return 1;
	case 0:
#if WOLF3D_WALL_GRAY_STIPPLE
		return (xParity ^ yParity) ? 0 : 1;
#else
		return 0;
#endif
	default:
#if WOLF3D_WALL_GRAY_STIPPLE
		return (xParity & yParity) ? 1 : 0;
#else
		return 1;
#endif
	}
#endif
}

static inline void Renderer_writeWallTexPixelFast(uint8_t x, uint8_t y, uint8_t texData)
{
	switch(texData)
	{
	case 1:
		Renderer_writePixelFast(x, y, 0);
		break;
	case 2:
		Renderer_writePixelFast(x, y, 1);
		break;
	case 0:
#if WOLF3D_WALL_GRAY_STIPPLE
		Renderer_writePixelFast(x, y, Renderer_xorStippleColour(x, y));
#else
		Renderer_writePixelFast(x, y, 0);
#endif
		break;
	case 3:
#if WOLF3D_WALL_GRAY_STIPPLE
		Renderer_writePixelFast(x, y, Renderer_andStippleColour(x, y));
#else
		Renderer_writePixelFast(x, y, 1);
#endif
		break;
	}
}

static inline void Renderer_writeSpriteTexPixelFast(uint8_t x, uint8_t y, uint8_t texData)
{
	switch(texData)
	{
	case 0:
		break;
	case 1:
		Renderer_writePixelFast(x, y, 0);
		break;
	case 2:
		Renderer_writePixelFast(x, y, 1);
		break;
	case 3:
#if WOLF3D_SPRITE_GRAY_STIPPLE
		Renderer_writePixelFast(x, y, Renderer_xorStippleColour(x, y));
#else
		Renderer_writePixelFast(x, y, 0);
#endif
		break;
	}
}

static inline void Renderer_writeWeaponTexPixelFast(int16_t x, int16_t y, uint8_t texData)
{
	if(x < 0 || x >= DISPLAYWIDTH || y < 0 || y >= DISPLAYHEIGHT || texData == 0)
	{
		return;
	}

	switch(texData)
	{
	case 1:
		Renderer_writePixelFast((uint8_t)x, (uint8_t)y, 1);
		break;
	case 2:
		Renderer_writePixelFast((uint8_t)x, (uint8_t)y, 0);
		break;
	case 3:
#if WOLF3D_SPRITE_GRAY_STIPPLE
		Renderer_writePixelFast((uint8_t)x, (uint8_t)y, Renderer_xorStippleColour((uint8_t)x, (uint8_t)y));
#else
		Renderer_writePixelFast((uint8_t)x, (uint8_t)y, 0);
#endif
		break;
	}
}

static inline uint8_t BitPairReader_readFast(BitPairReader* self);

static inline uint8_t Renderer_clampWallWidth(int16_t w)
{
	if(w <= 0)
	{
		return 0;
	}
	if(w >= 255)
	{
		return 255;
	}
	return (uint8_t)w;
}

static inline void Renderer_advanceSpriteV(int16_t w, int16_t* verror, int8_t* v, BitPairReader* textureReader, uint8_t* texData)
{
	*verror -= 15;
	while(*verror < 0)
	{
		*texData = BitPairReader_readFast(textureReader);
		*verror += w;
		(*v)++;
	}
}

static inline uint8_t BitPairReader_readFast(BitPairReader* self)
{
	uint8_t result = (uint8_t)((self->m_lastRead >> self->m_readOffset) & 3);
	self->m_readOffset += 2;
	if(self->m_readOffset == 8)
	{
		self->m_ptr++;
		self->m_lastRead = pgm_read_byte(self->m_ptr);
		self->m_readOffset = 0;
	}
	return result;
}

static inline void Renderer_advanceWallInterpolation(int16_t dx, int16_t dw, int16_t wstep, int16_t* w, int16_t* werror, int8_t du, int8_t ustep, int8_t* u, int16_t* uerror)
{
	*werror -= dw;
	*uerror -= du;

	if(dx > 0)
	{
		while(*werror < 0)
		{
			*w += wstep;
			*werror += dx;
		}
		while(*uerror < 0)
		{
			*u += ustep;
			*uerror += dx;
		}
	}
}

static inline void Renderer_storeWallColumn(Renderer* self, uint8_t x, uint8_t drawW, int8_t u, uint8_t textureId)
{
	if(drawW == 0 || drawW <= self->wbuffer[x])
	{
		return;
	}

	self->wbuffer[x] = drawW;

#ifdef DEFER_RENDER
	self->ubuffer[x] = u;
	self->texbuffer[x] = textureId;
#if WOLF3D_FAST_DEFERRED_WALL_BOUNDS
	if(x < self->wallFirstX)
	{
		self->wallFirstX = x;
	}
	if(x > self->wallLastX)
	{
		self->wallLastX = x;
	}
#endif
#else
	Renderer_drawStrip(self, x, drawW, u, textureId);
#endif
}

static inline void Renderer_drawVisibleWallSide(Renderer* self, int8_t testX, int8_t testZ, int16_t x1, int16_t z1, int16_t x2, int16_t z2, uint8_t textureId)
{
#if WOLF3D_FAST_NEIGHBOR_TILE_READS
	uint8_t neighborTile = Map_getTile(&engine.map, testX, testZ);
	if(Map_tileIsDoor(neighborTile))
	{
		Renderer_drawWall(self, x1, z1, x2, z2, DOOR_FRAME_TEXTURE);
	}
	else if(!Map_tileIsWall(neighborTile))
	{
		Renderer_drawWall(self, x1, z1, x2, z2, textureId);
	}
#else
	if(Map_isDoor(&engine.map, testX, testZ))
	{
		Renderer_drawWall(self, x1, z1, x2, z2, DOOR_FRAME_TEXTURE);
	}
	else if(!Map_isSolid(&engine.map, testX, testZ))
	{
		Renderer_drawWall(self, x1, z1, x2, z2, textureId);
	}
#endif
}



void Renderer_init(Renderer* self){
	self->damageIndicator = 0;
	self->hudHealthFlash = 0;
	self->hudAmmoFlash = 0;
	self->renderQueueHead = NULL_QUEUE_ITEM;
	Renderer_initWBuffer(self);
#ifdef DEFER_RENDER
	memset(self->ubuffer, 0, sizeof(self->ubuffer));
	memset(self->texbuffer, 0, sizeof(self->texbuffer));
#endif
	for(uint8_t n = 0; n < RENDER_QUEUE_CAPACITY; n++)
	{
		self->renderQueue[n].data = NULL;
		self->renderQueue[n].next = NULL_QUEUE_ITEM;
	}
}
void Renderer_drawDamage(Renderer* self){
	if(self->damageIndicator > 0)
	{
		self->damageIndicator --;
		for(uint8_t x = 0; x < DISPLAYWIDTH; x++)
		{
			Renderer_writePixelFast(x, 0, 1);
			Renderer_writePixelFast(x, DISPLAYHEIGHT - 1, 1);
		}
		drawVerticalSpan(0, 0, DISPLAYHEIGHT - 1, 1);
		drawVerticalSpan(DISPLAYWIDTH - 1, 0, DISPLAYHEIGHT - 1, 1);
	}
}
static void Renderer_drawGlyphColour(Renderer* self, char glyph, uint8_t x, uint8_t y, uint8_t fg, uint8_t bg)
{
	const uint8_t* ptr = Data_font + glyph * FONT_GLYPH_BYTE_SIZE;
	uint8_t readMask = 1;
	uint8_t read = pgm_read_byte(ptr++);

	for(uint8_t i = 0; i < FONT_WIDTH; i++)
	{
		for(uint8_t j = 0; j < FONT_HEIGHT; j++)
		{
			uint8_t colour = (read & readMask) ? fg : bg;
			Renderer_writePixelFastClipped((int16_t)x + i, (int16_t)y + j, colour);
			readMask <<= 1;
			if(readMask == 0)
			{
				readMask = 1;
				read = pgm_read_byte(ptr++);
			}
		}
	}
	for(uint8_t j = 0; j < FONT_HEIGHT; j++)
	{
		Renderer_writePixelFastClipped((int16_t)x + FONT_WIDTH, (int16_t)y + j, bg);
	}
}

static void Renderer_drawIntColour(Renderer* self, int8_t val, uint8_t x, uint8_t y, uint8_t fg, uint8_t bg)
{
	for(uint8_t i = 0; i < 3; i++)
	{
		uint8_t c = val % 10;
		if(val > 0 || i == 0)
		{
			Renderer_drawGlyphColour(self, c + '0' - FIRST_FONT_GLYPH, x, y, fg, bg);
		}
		else
		{
			Renderer_drawGlyphColour(self, ' ' - FIRST_FONT_GLYPH, x, y, fg, bg);
		}
		x -= FONT_WIDTH + 1;
		val = val / 10;
	}
}

void Renderer_drawHud(Renderer* self){
	uint8_t hudHeight = DISPLAYHEIGHT - FONT_HEIGHT;
	uint8_t healthFg = WOLF3D_FONT_FOREGROUND_COLOUR;
	uint8_t healthBg = WOLF3D_FONT_BACKGROUND_COLOUR;
	uint8_t ammoFg = WOLF3D_FONT_FOREGROUND_COLOUR;
	uint8_t ammoBg = WOLF3D_FONT_BACKGROUND_COLOUR;

	if(self->hudHealthFlash > 0)
	{
		if(self->hudHealthFlash & 2)
		{
			healthFg = WOLF3D_FONT_BACKGROUND_COLOUR;
			healthBg = WOLF3D_FONT_FOREGROUND_COLOUR;
		}
		self->hudHealthFlash--;
	}
	if(self->hudAmmoFlash > 0)
	{
		if(self->hudAmmoFlash & 2)
		{
			ammoFg = WOLF3D_FONT_BACKGROUND_COLOUR;
			ammoBg = WOLF3D_FONT_FOREGROUND_COLOUR;
		}
		self->hudAmmoFlash--;
	}

	Renderer_drawGlyphColour(self, '+' - FIRST_FONT_GLYPH, 0, hudHeight, healthFg, healthBg);
	Renderer_drawIntColour(self, engine.player.hp, (FONT_WIDTH + 1) * 3, hudHeight, healthFg, healthBg);
	Renderer_drawGlyphColour(self, '*' - FIRST_FONT_GLYPH, DISPLAYWIDTH - (FONT_WIDTH + 1) * 4, hudHeight, ammoFg, ammoBg);
	Renderer_drawIntColour(self, engine.player.weapon.ammo, DISPLAYWIDTH - (FONT_WIDTH + 1), hudHeight, ammoFg, ammoBg);
}

#if WOLF3D_MAP_PLAYER_ARROW
static void Renderer_drawLineClipped(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t colour)
{
	int16_t dx = mabs(x1 - x0);
	int16_t sx = x0 < x1 ? 1 : -1;
	int16_t dy = -mabs(y1 - y0);
	int16_t sy = y0 < y1 ? 1 : -1;
	int16_t err = dx + dy;

	for(;;)
	{
		Renderer_writePixelFastClipped(x0, y0, colour);
		if(x0 == x1 && y0 == y1)
		{
			break;
		}
		int16_t e2 = err << 1;
		if(e2 >= dy)
		{
			err += dy;
			x0 += sx;
		}
		if(e2 <= dx)
		{
			err += dx;
			y0 += sy;
		}
	}
}

static void Renderer_drawMapPlayerArrow(int16_t centerX, int16_t centerY, uint8_t scale)
{
	int16_t dirX = FixedMath_Cos(engine.player.direction);
	int16_t dirY = FixedMath_Sin(engine.player.direction);
	int16_t len = WOLF3D_MAP_ARROW_LEN_BASE + ((int16_t)scale * WOLF3D_MAP_ARROW_LEN_SCALE);
	int16_t wing = (int16_t)scale / WOLF3D_MAP_ARROW_WING_DIV;

	if(wing < WOLF3D_MAP_ARROW_WING_MIN)
	{
		wing = WOLF3D_MAP_ARROW_WING_MIN;
	}
	if(len > WOLF3D_MAP_ARROW_LEN_MAX)
	{
		len = WOLF3D_MAP_ARROW_LEN_MAX;
	}

	int16_t tipX = centerX + ((dirX * len) >> FIXED_SHIFT);
	int16_t tipY = centerY + ((dirY * len) >> FIXED_SHIFT);
	int16_t tailX = centerX - ((dirX * (len >> 1)) >> FIXED_SHIFT);
	int16_t tailY = centerY - ((dirY * (len >> 1)) >> FIXED_SHIFT);
	int16_t leftX = tailX + ((-dirY * wing) >> FIXED_SHIFT);
	int16_t leftY = tailY + (( dirX * wing) >> FIXED_SHIFT);
	int16_t rightX = tailX - ((-dirY * wing) >> FIXED_SHIFT);
	int16_t rightY = tailY - (( dirX * wing) >> FIXED_SHIFT);

	Renderer_drawLineClipped(tipX, tipY, leftX, leftY, WOLF3D_FONT_FOREGROUND_COLOUR);
	Renderer_drawLineClipped(tipX, tipY, rightX, rightY, WOLF3D_FONT_FOREGROUND_COLOUR);
}
#endif
void Renderer_drawMap(Renderer* self){
	(void)self;
	uint8_t scale = engine.mapZoom;
	uint8_t visibleX;
	uint8_t visibleZ;
	uint8_t mapX;
	uint8_t mapY;
	int8_t playerCellX = WORLD_TO_CELL(engine.player.x);
	int8_t playerCellZ = WORLD_TO_CELL(engine.player.z);
	int8_t startX;
	int8_t startZ;

	if(scale < WOLF3D_MAP_MIN_SCALE)
	{
		scale = WOLF3D_MAP_MIN_SCALE;
	}
	if(scale > WOLF3D_MAP_MAX_SCALE)
	{
		scale = WOLF3D_MAP_MAX_SCALE;
	}

#if WOLF3D_MAP_DEDICATED_SCREEN
	visibleX = DISPLAYWIDTH / scale;
	visibleZ = DISPLAYHEIGHT / scale;
	if(visibleX > MAP_SIZE)
	{
		visibleX = MAP_SIZE;
	}
	if(visibleZ > MAP_SIZE)
	{
		visibleZ = MAP_SIZE;
	}
	startX = playerCellX - (int8_t)(visibleX / 2);
	startZ = playerCellZ - (int8_t)(visibleZ / 2);
	if(startX < 0)
	{
		startX = 0;
	}
	if(startZ < 0)
	{
		startZ = 0;
	}
	if(startX > MAP_SIZE - visibleX)
	{
		startX = MAP_SIZE - visibleX;
	}
	if(startZ > MAP_SIZE - visibleZ)
	{
		startZ = MAP_SIZE - visibleZ;
	}
	mapX = (uint8_t)((DISPLAYWIDTH - (visibleX * scale)) / 2);
	mapY = (uint8_t)((DISPLAYHEIGHT - (visibleZ * scale)) / 2);
#else
	visibleX = MAP_BUFFER_SIZE;
	visibleZ = MAP_BUFFER_SIZE;
	startX = engine.map.bufferX;
	startZ = engine.map.bufferZ;
	mapX = WOLF3D_MAP_X;
	mapY = WOLF3D_MAP_Y;
#endif

	#if !WOLF3D_MAP_DEDICATED_SCREEN
	Renderer_fillRectFast(mapX, mapY, (uint8_t)(visibleX * scale), (uint8_t)(visibleZ * scale), WOLF3D_FONT_BACKGROUND_COLOUR);
#endif

	for(uint8_t z = 0; z < visibleZ; z++)
	{
		for(uint8_t x = 0; x < visibleX; x++)
		{
			int8_t cellX = startX + x;
			int8_t cellZ = startZ + z;
#if WOLF3D_MAP_DEDICATED_SCREEN
			uint8_t tile = Map_readRawTile(&engine.map, cellX, cellZ);
#else
			uint8_t tile = Map_getTile(&engine.map, cellX, cellZ);
#endif
#if WOLF3D_SEEN_WALL_BITSET && WOLF3D_MAP_ONLY_SEEN_WALLS
			if(!Map_isWallSeen(&engine.map, cellX, cellZ))
			{
				tile = Tile_Empty;
			}
			else if((tile >= Tile_FirstActor && tile <= Tile_LastActor) || (tile >= Tile_FirstItem && tile <= Tile_LastItem))
			{
				tile = Tile_Empty;
			}
#endif
			uint8_t colour = WOLF3D_FONT_BACKGROUND_COLOUR;

			if(tile >= Tile_FirstWall && tile <= Tile_LastWall)
			{
				colour = WOLF3D_FONT_FOREGROUND_COLOUR;
			}
			else if(tile >= Tile_FirstDoor && tile <= Tile_LastDoor)
			{
				colour = ((x ^ z) & 1) ? WOLF3D_FONT_FOREGROUND_COLOUR : WOLF3D_FONT_BACKGROUND_COLOUR;
			}
			else if(tile == Tile_SecretPushWall || (tile >= Tile_FirstBlockingDecoration && tile <= Tile_LastBlockingDecoration))
			{
				colour = ((x ^ z) & 1) ? WOLF3D_FONT_FOREGROUND_COLOUR : WOLF3D_FONT_BACKGROUND_COLOUR;
			}
			else if((tile >= Tile_FirstActor && tile <= Tile_LastActor) || (tile >= Tile_FirstItem && tile <= Tile_LastItem))
			{
				colour = ((x + z) & 1) ? WOLF3D_FONT_FOREGROUND_COLOUR : WOLF3D_FONT_BACKGROUND_COLOUR;
			}

			if(scale == 1)
			{
				Renderer_writePixelFast((uint8_t)(mapX + x), (uint8_t)(mapY + z), colour);
			}
			else
			{
				if(colour != WOLF3D_FONT_BACKGROUND_COLOUR)
				{
					Renderer_fillMapCellFast((uint8_t)(mapX + (x * scale)), (uint8_t)(mapY + (z * scale)), scale, colour);
				}
			}
		}
	}

	for(uint8_t n = 0; n < MAX_DOORS; n++)
	{
		Door* door = &engine.map.doors[n];
		if(door->type != DoorType_None && door->x >= startX && door->z >= startZ && door->x < startX + visibleX && door->z < startZ + visibleZ)
		{
#if WOLF3D_SEEN_WALL_BITSET && WOLF3D_MAP_ONLY_SEEN_WALLS
			if(!Map_isWallSeen(&engine.map, door->x, door->z))
			{
				continue;
			}
#endif
			uint8_t x = (uint8_t)(door->x - startX);
			uint8_t z = (uint8_t)(door->z - startZ);
			uint8_t colour = ((x ^ z) & 1) ? WOLF3D_FONT_FOREGROUND_COLOUR : WOLF3D_FONT_BACKGROUND_COLOUR;
			if(scale == 1)
			{
				Renderer_writePixelFast((uint8_t)(mapX + x), (uint8_t)(mapY + z), colour);
			}
			else
			{
				if(colour != WOLF3D_FONT_BACKGROUND_COLOUR)
				{
					Renderer_fillMapCellFast((uint8_t)(mapX + (x * scale)), (uint8_t)(mapY + (z * scale)), scale, colour);
				}
			}
		}
	}

#if WOLF3D_MAP_SHOW_ACTORS
	for(uint8_t n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		Actor* actor = &engine.actors[n];
		if(actor->type != ActorType_Empty && actor->hp > 0)
		{
			int8_t cellX = WORLD_TO_CELL(actor->x);
			int8_t cellZ = WORLD_TO_CELL(actor->z);
			if(cellX >= startX && cellZ >= startZ && cellX < startX + visibleX && cellZ < startZ + visibleZ)
			{
				uint8_t x = (uint8_t)(cellX - startX);
				uint8_t z = (uint8_t)(cellZ - startZ);
				if(scale == 1)
				{
					Renderer_writePixelFast((uint8_t)(mapX + x), (uint8_t)(mapY + z), WOLF3D_FONT_FOREGROUND_COLOUR);
				}
				else
				{
					Renderer_fillMapCellFast((uint8_t)(mapX + (x * scale)), (uint8_t)(mapY + (z * scale)), scale, WOLF3D_FONT_FOREGROUND_COLOUR);
					Renderer_writePixelFastClipped((int16_t)(mapX + (x * scale)) + 1, (int16_t)(mapY + (z * scale)) + 1, WOLF3D_FONT_BACKGROUND_COLOUR);
				}
			}
		}
	}
#endif

	if(playerCellX >= startX && playerCellZ >= startZ && playerCellX < startX + visibleX && playerCellZ < startZ + visibleZ)
	{
		int16_t localWorldX = engine.player.x - CELL_TO_WORLD(startX);
		int16_t localWorldZ = engine.player.z - CELL_TO_WORLD(startZ);
		int16_t screenX = (int16_t)mapX + ((localWorldX * scale) >> CELL_SIZE_SHIFT);
		int16_t screenY = (int16_t)mapY + ((localWorldZ * scale) >> CELL_SIZE_SHIFT);

#if WOLF3D_MAP_PLAYER_ARROW
		Renderer_drawMapPlayerArrow(screenX, screenY, scale);
#else
		Renderer_writePixelFastClipped(screenX, screenY, WOLF3D_FONT_FOREGROUND_COLOUR);
#endif
	}
}
void Renderer_drawWeapon(Renderer* self){
	uint8_t frameIndex = engine.player.weapon.frame;
	const SpriteFrame* frame = &Data_pistolSprite_frames[frameIndex > 3 ? 3 : frameIndex];
	const uint8_t* data = Data_pistolSprite;

	switch(engine.player.weapon.type)
	{
	case WeaponType_Knife:
		frame = &Data_knifeSprite_frames[frameIndex > 3 ? 3 : frameIndex];
		data = Data_knifeSprite;
		break;
	case WeaponType_MachineGun:
	case WeaponType_ChainGun:
		frame = &Data_machinegunSprite_frames[frameIndex > 2 ? 2 : frameIndex];
		data = Data_machinegunSprite;
		break;
	case WeaponType_Pistol:
	default:
		break;
	}

	BitPairReader reader;
	BitPairReader_init(&reader, data, pgm_read_word(&frame->offset));
	uint8_t frameWidth = pgm_read_byte(&frame->width);
	uint8_t frameHeight = pgm_read_byte(&frame->height);
	uint8_t x = HALF_DISPLAYWIDTH - 8 + pgm_read_byte(&frame->xOffset);

	for(int8_t i = 0; i < frameWidth; i++)
	{
		for(int8_t j = frameHeight - 1; j >= 0; j--)
		{
			uint8_t pixel = BitPairReader_readFast(&reader);
			Renderer_writeWeaponTexPixelFast((int16_t)i + x, (int16_t)DISPLAYHEIGHT - frameHeight + j, pixel);
		}
	}
}
static void Renderer_updateViewFromPlayer(Renderer* self)
{
	self->view.x = engine.player.x;
	self->view.z = engine.player.z;
	self->view.rotCos = FixedMath_Cos(-engine.player.direction);
	self->view.rotSin = FixedMath_Sin(-engine.player.direction);
	self->view.clipCos = FixedMath_Cos(-engine.player.direction + DEGREES_90 / 2);
	self->view.clipSin = FixedMath_Sin(-engine.player.direction + DEGREES_90 / 2);
	self->view.cellX = WORLD_TO_CELL(engine.player.x);
	self->view.cellZ = WORLD_TO_CELL(engine.player.z);
}

#if WOLF3D_SEEN_WALL_BITSET
void Renderer_updateSeenWalls(Renderer* self)
{
	Renderer_updateViewFromPlayer(self);
	for(int8_t z = engine.map.bufferZ; z < engine.map.bufferZ + MAP_BUFFER_SIZE; z++)
	{
		for(int8_t x = engine.map.bufferX; x < engine.map.bufferX + MAP_BUFFER_SIZE; x++)
		{
			uint8_t tile = Map_getTileFast(&engine.map, x, z);
			if(tile == 0)
			{
				continue;
			}
			if(Renderer_isFrustrumClipped(self, x, z))
			{
				continue;
			}
			if((tile >= Tile_FirstWall && tile <= Tile_LastWall)
			|| (tile >= Tile_FirstDoor && tile <= Tile_LastDoor)
			|| tile == Tile_SecretPushWall
			|| (tile >= Tile_FirstBlockingDecoration && tile <= Tile_LastBlockingDecoration))
			{
				Map_markSeenWall(&engine.map, x, z);
			}
		}
	}
}
#endif

void Renderer_drawFrame(Renderer* self){
	self->renderQueueHead = NULL_QUEUE_ITEM;
	for(int8_t n = 0; n < RENDER_QUEUE_CAPACITY; n++)
	{
		self->renderQueue[n].data = NULL;
	}

	Renderer_updateViewFromPlayer(self);
	Renderer_initWBuffer(self);

#if !defined(DEFER_RENDER)
	Renderer_drawFloorAndCeiling(self);
#endif

	Renderer_drawBufferedCells(self);
	Renderer_drawDoors(self);

	for(int8_t n = 0; n < MAX_ACTIVE_ACTORS; n++)
	{
		if(engine.actors[n].type != ActorType_Empty && !engine.actors[n].flags.frozen)
		{
			Actor_draw(&engine.actors[n]);
		}
	}

	for(int8_t n = 0; n < MAX_ACTIVE_ITEMS; n++)
	{
		if(engine.map.items[n].type != 0)
		{
			int16_t x = CELL_TO_WORLD(engine.map.items[n].x) + CELL_SIZE / 2, z = CELL_TO_WORLD(engine.map.items[n].z) + CELL_SIZE / 2;
			Renderer_queueSprite(self, &Data_itemSprites_frames[(engine.map.items[n].type - Tile_FirstItem)], Data_itemSprites, x, z);
		}
	}
	
#if 0
	int fill1 = 0;
	int fill2 = 0;
	for(int i = engine.map.bufferX; i < engine.map.bufferX + MAP_BUFFER_SIZE; i++)
	{
		for(int j = engine.map.bufferZ; j < engine.map.bufferZ + MAP_BUFFER_SIZE; j++)
		{
			uint8_t tile = Map_getTile(&engine.map, i, j);
			uint8_t colour = 1;

			if(!((self->view.clipCos * (i - self->view.cellX) - self->view.clipSin * (j - self->view.cellZ)) <= 0))
				fill1 ++;
			if(!Renderer_isFrustrumClipped(self, i, j))
				fill2 ++;
			if(tile >= Tile_FirstWall && tile <= Tile_LastWall)
			{
				colour = 0;
				if((self->view.clipCos * (i - self->view.cellX) - self->view.clipSin * (j - self->view.cellZ)) <= 0)
					colour = 1;
				colour = Renderer_isFrustrumClipped(self, i, j) ? 1 : 0;
			}
			drawPixel(i - engine.map.bufferX, j - engine.map.bufferZ, colour);
		}
	}
	WARNING("Old: %d\tNew: %d\t Diff=%f\n", fill1, fill2, (float)fill2 / (float)fill1);
	drawPixel(self->view.cellX - engine.map.bufferX, self->view.cellZ - engine.map.bufferZ, 0);

#endif

#if !defined(DEFER_RENDER)
	for(uint8_t item = self->renderQueueHead; item != NULL_QUEUE_ITEM; item = self->renderQueue[item].next)
	{
		Renderer_drawQueuedSprite(self, item);
	}

	Renderer_drawWeapon(self);
	Renderer_drawDamage(self);

	if(engine.mapVisible)
	{
		Renderer_drawMap(self);
	}
	else
	{
		Renderer_drawHud(self);
	}
	//Renderer_drawString(self, PSTR("*99"), DISPLAYWIDTH - (FONT_WIDTH + 1) * 3, DISPLAYHEIGHT - FONT_HEIGHT);
	/*int y = 4;
	Renderer_drawString(self, PSTR("* CAN I PLAY, DADDY?"), 0, y); y += FONT_HEIGHT + 1;
	Renderer_drawString(self, PSTR("  DON'T HURT ME!"), 0, y); y += FONT_HEIGHT + 1;
	Renderer_drawString(self, PSTR("  BRING 'EM ON!"), 0, y); y += FONT_HEIGHT + 1;
	Renderer_drawString(self, PSTR("  I AM DEATH"), 0, y); y += FONT_HEIGHT + 1;
	Renderer_drawString(self, PSTR("       INCARNATE!"), 0, y); y += FONT_HEIGHT + 1;*/
#endif
}

#ifdef DEFER_RENDER
void Renderer_drawDeferredFrame(Renderer* self){
#if WOLF3D_MAP_DEDICATED_SCREEN
	if(engine.mapVisible)
	{
		return;
	}
#endif
	/*
	 * Restore the original Arduboy-style background pass for deferred rendering.
	 * The previous Uzebox path only cleared the open space above/below wall
	 * strips, which made both ceiling and floor flat black.  The original
	 * renderer draws a clear ceiling and a checker/stippled floor first, then
	 * draws wall strips on top.
	 */
	Renderer_drawFloorAndCeiling(self);

#if WOLF3D_FAST_DEFERRED_WALL_BOUNDS
	if(self->wallFirstX < DISPLAYWIDTH)
	{
		for(uint8_t x = self->wallFirstX; x <= self->wallLastX; x++)
		{
			if(self->wbuffer[x] != 0)
			{
				Renderer_drawStrip(self, x, self->wbuffer[x], self->ubuffer[x], self->texbuffer[x]);
			}
		}
	}
#else
	for(uint8_t x = 0; x < DISPLAYWIDTH; x++)
	{
		if(self->wbuffer[x] != 0)
		{
			Renderer_drawStrip(self, x, self->wbuffer[x], self->ubuffer[x], self->texbuffer[x]);
		}
	}
#endif

	for(uint8_t item = self->renderQueueHead; item != NULL_QUEUE_ITEM; item = self->renderQueue[item].next)
	{
		Renderer_drawQueuedSprite(self, item);
	}

	Renderer_drawWeapon(self);
	Renderer_drawDamage(self);
	if(engine.mapVisible)
	{
		Renderer_drawMap(self);
	}
	else
	{
		Renderer_drawHud(self);
	}
}
#endif
void Renderer_drawBufferedCells(Renderer* self){
	int8_t xd, zd;
	int8_t x1, z1, x2, z2;

	if(self->view.rotCos > 0)
	{
		x1 = engine.map.bufferX;
		x2 = x1 + MAP_BUFFER_SIZE;
		xd = 1;
	}
	else
	{
		x2 = engine.map.bufferX - 1;
		x1 = x2 + MAP_BUFFER_SIZE;
		xd = -1;
	}
	if(self->view.rotSin < 0)
	{
		z1 = engine.map.bufferZ;
		z2 = z1 + MAP_BUFFER_SIZE;
		zd = 1;
	}
	else
	{
		z2 = engine.map.bufferZ - 1;
		z1 = z2 + MAP_BUFFER_SIZE;
		zd = -1;
	}

	if(mabs(self->view.rotCos) < mabs(self->view.rotSin))
	{
		for(int8_t z = z1; z != z2; z += zd)
		{
			for(int8_t x = x1; x != x2; x+= xd)
			{
				Renderer_drawCell(self, x, z);
			}
		}
	}
	else
	{
		for(int8_t x = x1; x != x2; x+= xd)
		{
			for(int8_t z = z1; z != z2; z += zd)
			{
				Renderer_drawCell(self, x, z);
			}
		}
	}
}
void Renderer_initWBuffer(Renderer* self){
	memset(self->wbuffer, 0, sizeof(self->wbuffer));
#ifdef DEFER_RENDER
#if WOLF3D_FAST_DEFERRED_WALL_BOUNDS
	self->wallFirstX = DISPLAYWIDTH;
	self->wallLastX = 0;
#endif
#endif
}

#if defined(PLATFORM_GAMEBUINO)
extern uint8_t _displayBuffer[];
void Renderer_drawFloorAndCeiling(Renderer* self){
	memset(_displayBuffer, 0x00, 3*84);
	for (int y=3, ofs=3*84; y<6; y++)
	{
		for (int8_t x=0; x<84; x+=2)
		{
			_displayBuffer[ofs++] = 0x55;
			_displayBuffer[ofs++] = 0x00;
		}
	}
}
#else
void Renderer_drawFloorAndCeiling(Renderer* self){
	(void)self;

#if defined(PLATFORM_UZEBOX_MODE23)
	Platform_fillOriginalArduboyBackground();
#else
	for(uint8_t x = 0; x < DISPLAYWIDTH; x++)
	{
		for(uint8_t y = 0; y < DISPLAYHEIGHT; y++)
		{
			if(y < HALF_DISPLAYHEIGHT || ((x & y) & 1) == 0)
			{
				clearPixel(x, y);
			}
			else
			{
				setPixel(x, y);
			}
		}
	}
#endif
}
#endif

#if defined(PLATFORM_GAMEBUINO)
extern Gamebuino gb;
#endif
void Renderer_drawCell(Renderer* self, int8_t cellX, int8_t cellZ){
#if defined(PLATFORM_GAMEBUINO)
	// HACK: Keep calling update so that sound doesn't slow down. Ugh..
	;
#endif
	/* Most streamed cells are empty.  Read the 16x16 RAM tile cache first so
	 * empty cells avoid the two fixed-point frustum multiplies. */
	uint8_t tile = Map_getTileFast(&engine.map, cellX, cellZ);
	if(tile == 0)
		return;

#if WOLF3D_FAST_TILE_FIRST_RENDER
	if(Renderer_isFrustrumClipped(self, cellX, cellZ))
		return;
#else
	if(Renderer_isFrustrumClipped(self, cellX, cellZ))
		return;
#endif

#if WOLF3D_SEEN_WALL_BITSET
	if((tile >= Tile_FirstWall && tile <= Tile_LastWall)
	|| (tile >= Tile_FirstDoor && tile <= Tile_LastDoor)
	|| tile == Tile_SecretPushWall
	|| (tile >= Tile_FirstBlockingDecoration && tile <= Tile_LastBlockingDecoration))
	{
		Map_markSeenWall(&engine.map, cellX, cellZ);
	}
#endif

	int16_t worldX = CELL_TO_WORLD(cellX);
	int16_t worldZ = CELL_TO_WORLD(cellZ);
	
	if(tile >= Tile_FirstDecoration && tile <= Tile_LastDecoration)
	{
		Renderer_queueSprite(self, &Data_decorations_frames[tile - Tile_FirstDecoration], Data_decorations, worldX + CELL_SIZE / 2, worldZ + CELL_SIZE / 2);
		return;
	}
	if(tile >= Tile_FirstBlockingDecoration && tile <= Tile_LastBlockingDecoration)
	{
		Renderer_queueSprite(self, &Data_blockingDecorations_frames[tile - Tile_FirstBlockingDecoration], Data_blockingDecorations, worldX + CELL_SIZE / 2, worldZ + CELL_SIZE / 2);
		return;
	}

	if(tile >= Tile_FirstWall && tile <= Tile_LastWall)
	{
		uint8_t textureId = tile - Tile_FirstWall; //Map_getTextureId(&engine.map, cellX, cellZ);

		if (self->view.z < worldZ)
		{
			if (self->view.x > worldX)
			{
				// north west quadrant
				if (self->view.z < worldZ)
				{
					Renderer_drawVisibleWallSide(self, cellX, cellZ - 1, worldX, worldZ, worldX + CELL_SIZE, worldZ, textureId);  // south wall
				}
				if (self->view.x > worldX + CELL_SIZE)
				{
					Renderer_drawVisibleWallSide(self, cellX + 1, cellZ, worldX + CELL_SIZE, worldZ, worldX + CELL_SIZE, worldZ + CELL_SIZE, textureId);  // east wall
				}
			}
			else
			{
				// north east quadrant
				if (self->view.z < worldZ)
				{
					Renderer_drawVisibleWallSide(self, cellX, cellZ - 1, worldX, worldZ, worldX + CELL_SIZE, worldZ, textureId);  // south wall
				}
				if (self->view.x < worldX)
				{
					Renderer_drawVisibleWallSide(self, cellX - 1, cellZ, worldX, worldZ + CELL_SIZE, worldX, worldZ, textureId);  // west wall
				}
			}
		}
		else
		{
			if (self->view.x > worldX)
			{
				// south west quadrant
				if (self->view.z > worldZ + CELL_SIZE)
				{
					Renderer_drawVisibleWallSide(self, cellX, cellZ + 1, worldX + CELL_SIZE, worldZ + CELL_SIZE, worldX, worldZ + CELL_SIZE, textureId);  // north wall
				}
				if (self->view.x > worldX + CELL_SIZE)
				{
					Renderer_drawVisibleWallSide(self, cellX + 1, cellZ, worldX + CELL_SIZE, worldZ, worldX + CELL_SIZE, worldZ + CELL_SIZE, textureId);  // east wall
				}
			}
			else
			{
				// south east quadrant
				if (self->view.z > worldZ + CELL_SIZE)
				{
					Renderer_drawVisibleWallSide(self, cellX, cellZ + 1, worldX + CELL_SIZE, worldZ + CELL_SIZE, worldX, worldZ + CELL_SIZE, textureId);  // north wall
				}
				if (self->view.x < worldX)
				{
					Renderer_drawVisibleWallSide(self, cellX - 1, cellZ, worldX, worldZ + CELL_SIZE, worldX, worldZ, textureId);  // west wall
				}
			}
		}
	}
}

#define FORCE_WALL_STRIP_EDGES WOLF3D_FORCE_WALL_STRIP_EDGES

#if WOLF3D_FAST_UNPACKED_WALL_TEXTURES && WOLF3D_FAST_WALL_V_LOOKUP
void Renderer_drawStrip(Renderer* self, int16_t x, int16_t w, int8_t u, uint8_t textureId)
{
	(void)self;

	if(w <= 0 || x < 0 || x >= DISPLAYWIDTH)
	{
		return;
	}
	if(w > 255)
	{
		w = 255;
	}
	if(textureId >= DATA_WALL_TEXTURE_COUNT)
	{
		textureId = 0;
	}
	if(u < 0)
	{
		u = 0;
	}
	else if(u >= TEXTURE_SIZE)
	{
		u = TEXTURE_SIZE - 1;
	}

	int16_t halfW = w >> 1;
	int16_t y1 = HALF_DISPLAYHEIGHT - halfW;
	int16_t y2 = HALF_DISPLAYHEIGHT + halfW;
	int16_t yEnd;

#if FORCE_WALL_STRIP_EDGES
	yEnd = y2 - 1;
#else
	yEnd = y2;
#endif

	if(yEnd >= 0 && y1 < DISPLAYHEIGHT)
	{
		if(y1 < 0)
		{
			y1 = 0;
		}
		if(yEnd >= DISPLAYHEIGHT)
		{
			yEnd = DISPLAYHEIGHT - 1;
		}

		const uint8_t* vRow = Data_wallVLookup + (((uint16_t)(uint8_t)w) << 6);
		const uint8_t* texColumn = Data_wallTextureTexels + (((uint16_t)textureId) << 8) + (((uint16_t)(uint8_t)u) << 4);

#if defined(PLATFORM_UZEBOX_MODE23) && WOLF3D_FAST_WALL_PAGE_BLIT
		{
			uint8_t sx = (uint8_t)((uint8_t)x + WOLF3D_SBUFFER_X_OFFSET);
			uint8_t xParity = (uint8_t)x & 1;
			uint8_t yy = (uint8_t)y1;
			uint8_t yyEnd = (uint8_t)yEnd;

			while(yy <= yyEnd)
			{
				uint8_t sy = (uint8_t)(yy + WOLF3D_SBUFFER_Y_OFFSET);
				uint8_t page = (uint8_t)(sy >> 3);
				uint8_t writeMask = 0;
				uint8_t valueMask = 0;
				uint8_t pageEnd = (uint8_t)((((uint16_t)page << 3) + 7u) - WOLF3D_SBUFFER_Y_OFFSET);

				if(pageEnd > yyEnd)
				{
					pageEnd = yyEnd;
				}

				for(;;)
				{
					uint8_t bit = (uint8_t)(1u << (((uint8_t)(yy + WOLF3D_SBUFFER_Y_OFFSET)) & 7));
					uint8_t v = pgm_read_byte(vRow + yy);
					uint8_t texData = pgm_read_byte(texColumn + v);
					uint8_t colour = Renderer_wallTexColourFast(xParity, (uint8_t)(yy & 1), texData);

					writeMask |= bit;
					if(Renderer_physicalColour(colour))
					{
						valueMask |= bit;
					}

					if(yy == pageEnd)
					{
						break;
					}
					yy++;
				}

				Renderer_mergePageByte(&sBuffer[(uint16_t)sx + ((uint16_t)page * WOLF3D_SBUFFER_WIDTH)], writeMask, valueMask);
				if(yy == yyEnd)
				{
					break;
				}
				yy++;
			}
		}
#elif defined(PLATFORM_UZEBOX_MODE23) && WOLF3D_FAST_COLUMN_PIXEL_WRITER
		{
			uint8_t sx = (uint8_t)((uint8_t)x + WOLF3D_SBUFFER_X_OFFSET);
			uint8_t sy = (uint8_t)((uint8_t)y1 + WOLF3D_SBUFFER_Y_OFFSET);
			uint8_t* dst = &sBuffer[(uint16_t)sx + (((uint16_t)sy >> 3) * WOLF3D_SBUFFER_WIDTH)];
			uint8_t mask = (uint8_t)(1u << (sy & 7));
			uint8_t xParity = (uint8_t)x & 1;
			uint8_t yParity = (uint8_t)y1 & 1;

			uint8_t yy = (uint8_t)y1;
			uint8_t yyEnd = (uint8_t)yEnd;
			for(;;)
			{
				uint8_t v = pgm_read_byte(vRow + yy);
				uint8_t texData = pgm_read_byte(texColumn + v);
				uint8_t colour = Renderer_wallTexColourFast(xParity, yParity, texData);

				Renderer_writeColumnPixelPtrFast(&dst, &mask, colour);
				if(yy == yyEnd)
				{
					break;
				}
				yy++;
				yParity ^= 1;
			}
		}
#else
		uint8_t yy = (uint8_t)y1;
		uint8_t yyEnd = (uint8_t)yEnd;
		for(;;)
		{
			uint8_t v = pgm_read_byte(vRow + yy);
			uint8_t texData = pgm_read_byte(texColumn + v);
			Renderer_writeWallTexPixelFast((uint8_t)x, yy, texData);
			if(yy == yyEnd)
			{
				break;
			}
			yy++;
		}
#endif
	}

#if FORCE_WALL_STRIP_EDGES
	if(y2 >= 0 && y2 < DISPLAYHEIGHT)
	{
		Renderer_writePixelFast((uint8_t)x, (uint8_t)y2, 1);
	}
#endif
}
#else
void Renderer_drawStrip(Renderer* self, int16_t x, int16_t w, int8_t u, uint8_t textureId)
{
	(void)self;

	if(w <= 0 || x < 0 || x >= DISPLAYWIDTH)
	{
		return;
	}

	int16_t halfW = w >> 1;
	int16_t y1 = HALF_DISPLAYHEIGHT - halfW;
	int16_t y2 = HALF_DISPLAYHEIGHT + halfW;
	int16_t yEnd;
	int16_t verror = halfW;

#if FORCE_WALL_STRIP_EDGES
	yEnd = y2 - 1;
#else
	yEnd = y2;
#endif

	BitPairReader textureReader;
	BitPairReader_init(&textureReader, Data_wallTextures + u * TEXTURE_STRIDE + textureId * (TEXTURE_STRIDE * TEXTURE_SIZE), 0);
	uint8_t texData = BitPairReader_readFast(&textureReader);

#if WOLF3D_FAST_CLIPPED_WALL_STRIPS
	if(yEnd >= 0 && y1 < DISPLAYHEIGHT)
	{
		if(y1 < 0)
		{
			for(int16_t y = y1; y < 0; y++)
			{
				verror -= 15;
				while(verror < 0)
				{
					texData = BitPairReader_readFast(&textureReader);
					verror += w;
				}
			}
			y1 = 0;
		}

		if(yEnd >= DISPLAYHEIGHT)
		{
			yEnd = DISPLAYHEIGHT - 1;
		}

#if defined(PLATFORM_UZEBOX_MODE23) && WOLF3D_FAST_COLUMN_PIXEL_WRITER
		{
			uint8_t sx = (uint8_t)((uint8_t)x + WOLF3D_SBUFFER_X_OFFSET);
			uint8_t sy = (uint8_t)((uint8_t)y1 + WOLF3D_SBUFFER_Y_OFFSET);
			uint8_t* dst = &sBuffer[(uint16_t)sx + (((uint16_t)sy >> 3) * WOLF3D_SBUFFER_WIDTH)];
			uint8_t mask = (uint8_t)(1u << (sy & 7));
			uint8_t xParity = (uint8_t)x & 1;
			uint8_t yParity = (uint8_t)y1 & 1;

			for(int16_t y = y1; y <= yEnd; y++)
			{
				uint8_t colour = Renderer_wallTexColourFast(xParity, yParity, texData);

				Renderer_writeColumnPixelPtrFast(&dst, &mask, colour);
				yParity ^= 1;

				verror -= 15;
				while(verror < 0)
				{
					texData = BitPairReader_readFast(&textureReader);
					verror += w;
				}
			}
		}
#else
		for(int16_t y = y1; y <= yEnd; y++)
		{
			Renderer_writeWallTexPixelFast((uint8_t)x, (uint8_t)y, texData);

			verror -= 15;
			while(verror < 0)
			{
				texData = BitPairReader_readFast(&textureReader);
				verror += w;
			}
		}
#endif
	}
#else
	for(int16_t y = y1; y <= yEnd; y++)
	{
		if(y >= 0 && y < DISPLAYHEIGHT)
		{
			Renderer_writeWallTexPixelFast((uint8_t)x, (uint8_t)y, texData);
		}

		verror -= 15;
		while(verror < 0)
		{
			texData = BitPairReader_readFast(&textureReader);
			verror += w;
		}
	}
#endif

#if FORCE_WALL_STRIP_EDGES
	if(y2 >= 0 && y2 < DISPLAYHEIGHT)
	{
		Renderer_writePixelFast((uint8_t)x, (uint8_t)y2, 1);
	}
#endif
}

#endif

#ifdef PERSPECTIVE_CORRECT_TEXTURE_MAPPING
// draws one side of a cell
void Renderer_drawWall_impl(Renderer* self, int16_t _x1, int16_t _z1, int16_t _x2, int16_t _z2, uint8_t textureId, int8_t _u1, int8_t _u2){
	// find position of wall edges relative to eye

	int16_t z2 = (int16_t)(FIXED_TO_INT(self->view.rotCos * (int32_t)(_x1-self->view.x)) - FIXED_TO_INT(self->view.rotSin * (int32_t)(_z1-self->view.z)));
	int16_t x2 = (int16_t)(FIXED_TO_INT(self->view.rotSin * (int32_t)(_x1-self->view.x)) + FIXED_TO_INT(self->view.rotCos * (int32_t)(_z1-self->view.z)));
	int16_t z1 = (int16_t)(FIXED_TO_INT(self->view.rotCos * (int32_t)(_x2-self->view.x)) - FIXED_TO_INT(self->view.rotSin * (int32_t)(_z2-self->view.z)));
	int16_t x1 = (int16_t)(FIXED_TO_INT(self->view.rotSin * (int32_t)(_x2-self->view.x)) + FIXED_TO_INT(self->view.rotCos * (int32_t)(_z2-self->view.z)));

	// clip to the front pane
	if ((z1<CLIP_PLANE) && (z2<CLIP_PLANE))
		return;
	if (z1 < CLIP_PLANE)
	{
		x1 += (CLIP_PLANE-z1) * (x2-x1) / (z2-z1);
		z1 = CLIP_PLANE;
	}
	else if (z2 < CLIP_PLANE)
	{
		x2 += (CLIP_PLANE-z2) * (x1-x2) / (z1-z2);
		z2 = CLIP_PLANE;
	}

	// apply perspective projection
	int16_t vx1 = (int16_t)(x1 * NEAR_PLANE * CAMERA_SCALE / z1);  
	int16_t vx2 = (int16_t)(x2 * NEAR_PLANE * CAMERA_SCALE / z2); 

	// transform the end points into screen space
	int16_t sx1 = (int16_t)((DISPLAYWIDTH / 2) + vx1);
	int16_t sx2 = (int16_t)((DISPLAYWIDTH / 2) + vx2) - 1;

	// clamp to the visible portion of the screen
	int16_t firstx = max(sx1, 0);
	int16_t lastx = min(sx2, DISPLAYWIDTH-1);
	if (lastx < firstx)
		return;

	int16_t w1 = (int16_t)((CELL_SIZE * NEAR_PLANE * CAMERA_SCALE) / z1);
	int16_t w2 = (int16_t)((CELL_SIZE * NEAR_PLANE * CAMERA_SCALE) / z2);
	int16_t dx = sx2 - sx1;
	int16_t werror = dx >> 1;
	int16_t uerror = werror;
	int16_t w = w1;
	int16_t du, ustep;
	int16_t dw, wstep;

	int16_t u1 = _u1 * w1;
	int16_t u2 = _u2 * w2;
	int16_t u = _u1;
	int16_t z = z1;
	int8_t dz, zstep;
	int16_t zerror = werror;
	if(z1 < z2)
	{
		dz = z2 - z1;
		zstep = 1;
	}
	else
	{
		dz = z1 - z2;
		zstep = -1;
	}

	if(w1 < w2)
	{
		dw = w2 - w1;
		wstep = 1;
	}
	else
	{
		dw = w1 - w2;
		wstep = -1;
	}

	if(u1 < u2)
	{
		du = u2 - u1;
		ustep = 1;
	}
	else
	{
		du = u1 - u2;
		ustep = -1;
	}

	for(int x = sx1; x < firstx; x++)
	{
		Renderer_advanceWallInterpolation(dx, dw, wstep, &w, &werror, du, ustep, &u, &uerror);
	}

	for(int x = firstx; x <= lastx; x++)
	{
		uint8_t drawW = Renderer_clampWallWidth(w);
		Renderer_storeWallColumn(self, (uint8_t)x, drawW, u, textureId);

		Renderer_advanceWallInterpolation(dx, dw, wstep, &w, &werror, du, ustep, &u, &uerror);
	}
}
#else
// draws one side of a cell
void Renderer_drawWall_impl(Renderer* self, int16_t _x1, int16_t _z1, int16_t _x2, int16_t _z2, uint8_t textureId, int8_t _u1, int8_t _u2){
	// find position of wall edges relative to eye

	int16_t z2 = (int16_t)(FIXED_TO_INT(self->view.rotCos * (int32_t)(_x1-self->view.x)) - FIXED_TO_INT(self->view.rotSin * (int32_t)(_z1-self->view.z)));
	int16_t x2 = (int16_t)(FIXED_TO_INT(self->view.rotSin * (int32_t)(_x1-self->view.x)) + FIXED_TO_INT(self->view.rotCos * (int32_t)(_z1-self->view.z)));
	int16_t z1 = (int16_t)(FIXED_TO_INT(self->view.rotCos * (int32_t)(_x2-self->view.x)) - FIXED_TO_INT(self->view.rotSin * (int32_t)(_z2-self->view.z)));
	int16_t x1 = (int16_t)(FIXED_TO_INT(self->view.rotSin * (int32_t)(_x2-self->view.x)) + FIXED_TO_INT(self->view.rotCos * (int32_t)(_z2-self->view.z)));

	// clip to the front pane
	if ((z1<CLIP_PLANE) && (z2<CLIP_PLANE))
		return;
	if (z1 < CLIP_PLANE)
	{
		x1 += (CLIP_PLANE-z1) * (x2-x1) / (z2-z1);
		z1 = CLIP_PLANE;
	}
	else if (z2 < CLIP_PLANE)
	{
		x2 += (CLIP_PLANE-z2) * (x1-x2) / (z1-z2);
		z2 = CLIP_PLANE;
	}

	// apply perspective projection
	int16_t vx1 = (int16_t)(x1 * NEAR_PLANE * CAMERA_SCALE / z1);  
	int16_t vx2 = (int16_t)(x2 * NEAR_PLANE * CAMERA_SCALE / z2); 

	// transform the end points into screen space
	int16_t sx1 = (int16_t)((DISPLAYWIDTH / 2) + vx1);
	int16_t sx2 = (int16_t)((DISPLAYWIDTH / 2) + vx2) - 1;

	// clamp to the visible portion of the screen
	int16_t firstx = max(sx1, 0);
	int16_t lastx = min(sx2, DISPLAYWIDTH-1);
	if (lastx < firstx)
		return;

	int16_t w1 = (int16_t)((CELL_SIZE * NEAR_PLANE * CAMERA_SCALE) / z1);
	int16_t w2 = (int16_t)((CELL_SIZE * NEAR_PLANE * CAMERA_SCALE) / z2);
	int16_t dx = sx2 - sx1;
	int16_t werror = dx >> 1;
	int16_t uerror = werror;
	int16_t w = w1;
	int8_t u = _u1;
	int8_t du, ustep;
	int16_t dw, wstep;

	if(w1 < w2)
	{
		dw = w2 - w1;
		wstep = 1;
	}
	else
	{
		dw = w1 - w2;
		wstep = -1;
	}

	if(_u1 < _u2)
	{
		du = _u2 - _u1;
		ustep = 1;
	}
	else
	{
		du = _u1 - _u2;
		ustep = -1;
	}

	for(int x = sx1; x < firstx; x++)
	{
		Renderer_advanceWallInterpolation(dx, dw, wstep, &w, &werror, du, ustep, &u, &uerror);
	}

	for(int x = firstx; x <= lastx; x++)
	{
		uint8_t drawW = Renderer_clampWallWidth(w);
		Renderer_storeWallColumn(self, (uint8_t)x, drawW, u, textureId);

		Renderer_advanceWallInterpolation(dx, dw, wstep, &w, &werror, du, ustep, &u, &uerror);
	}
}
#endif
void Renderer_drawDoors(Renderer* self){
	for(int n = 0; n < MAX_DOORS; n++)
	{
		Door* door = &engine.map.doors[n];
		uint8_t textureId = door->texture;

		if(!Map_isValid(&engine.map, door->x, door->z))
		{
			continue;
		}
#if WOLF3D_SEEN_WALL_BITSET
		if(!Renderer_isFrustrumClipped(self, door->x, door->z))
		{
			Map_markSeenWall(&engine.map, door->x, door->z);
		}
#endif
		
		if(door->type == DoorType_SecretPushWall)
		{
			int16_t doorX = CELL_TO_WORLD(door->x);
			int16_t doorZ = CELL_TO_WORLD(door->z);

			switch(door->state)
			{
			case DoorState_PushNorth:
				doorZ -= door->open;
				break;
			case DoorState_PushEast:
				doorX += door->open;
				break;
			case DoorState_PushSouth:
				doorZ += door->open;
				break;
			case DoorState_PushWest:
				doorX -= door->open;
				break;
			}

			if(self->view.x < doorX)
			{
				Renderer_drawWall(self, doorX, doorZ + CELL_SIZE, doorX, doorZ, door->texture, 0, 15);
			}
			else if(self->view.x > doorX)
			{
				Renderer_drawWall(self, doorX + CELL_SIZE, doorZ, doorX + CELL_SIZE, doorZ + CELL_SIZE, door->texture, 0, 15);
			}
			if(self->view.z > doorZ + CELL_SIZE)
			{
				Renderer_drawWall(self, doorX + CELL_SIZE, doorZ + CELL_SIZE, doorX, doorZ + CELL_SIZE, door->texture, 0, 15);
			}
			else if(self->view.z < doorZ)
			{
				Renderer_drawWall(self, doorX, doorZ, doorX + CELL_SIZE, doorZ, door->texture, 0, 15);
			}
		}
		else
		{
			int offset = door->open;
			if(offset >= 16)
			{
				continue;
			}

			int16_t worldX = CELL_TO_WORLD(door->x);
			int16_t worldZ = CELL_TO_WORLD(door->z);

			if((door->type & 0x1) == 0)
			{
				worldX += CELL_SIZE / 2;
				if(self->view.x < worldX)
				{
					Renderer_drawWall(self, worldX, worldZ + CELL_SIZE, 
						worldX, worldZ + offset * 2, textureId, 0, 15 - offset);
				}
				else
				{
					Renderer_drawWall(self, worldX, worldZ + offset * 2, 
						worldX, worldZ + CELL_SIZE, textureId, 15 - offset, 0);
				}
			}
			else
			{
				worldZ += CELL_SIZE / 2;
				if(self->view.z > worldZ)
				{
					Renderer_drawWall(self, worldX + CELL_SIZE, worldZ, 
						worldX + offset * 2, worldZ, textureId, 0, 15 - offset);
				}
				else
				{
					Renderer_drawWall(self, worldX + offset * 2, worldZ, 
						worldX + CELL_SIZE, worldZ, textureId, 15 - offset, 0);
				}
			}
		}
	}
}
void Renderer_queueSprite(Renderer* self, const SpriteFrame* frame, const uint8_t* spriteData, int16_t _x, int16_t _z)
{
	Renderer_queueSpriteScaled(self, frame, spriteData, _x, _z, CELL_SIZE / 2);
}

void Renderer_queueSpriteScaled(Renderer* self, const SpriteFrame* frame, const uint8_t* spriteData, int16_t _x, int16_t _z, uint8_t scaleDiv){
#if 1
	int cellX = WORLD_TO_CELL(_x);
	int cellZ = WORLD_TO_CELL(_z);

	if(Renderer_isFrustrumClipped(self, cellX, cellZ))
		return;

	int16_t zt = (int16_t)(FIXED_TO_INT(self->view.rotCos * (int32_t)(_x-self->view.x)) - FIXED_TO_INT(self->view.rotSin * (int32_t)(_z-self->view.z)));
	int16_t xt = (int16_t)(FIXED_TO_INT(self->view.rotSin * (int32_t)(_x-self->view.x)) + FIXED_TO_INT(self->view.rotCos * (int32_t)(_z-self->view.z)));

	// clip to the front plane
	if (zt < CLIP_PLANE)
		return;

	// apply perspective projection
	int16_t vx = (int16_t)(xt * NEAR_PLANE * CAMERA_SCALE / zt);  

	if(vx <= -DISPLAYWIDTH || vx >= DISPLAYWIDTH)
		return;

	int16_t w = (int16_t)((CELL_SIZE * NEAR_PLANE * CAMERA_SCALE) / zt);
	int16_t x = vx + HALF_DISPLAYWIDTH;

	if(w <= 0)
		return;
	if(w > 255)
		w = 255;

	if((x + w) < 0 || (x - w) >= DISPLAYWIDTH)
		return;

	uint8_t newItem = NULL_QUEUE_ITEM;
	for(int n = 0; n < RENDER_QUEUE_CAPACITY; n++)
	{
		if(self->renderQueue[n].data == NULL)
		{
			newItem = n;
			break;
		}
	}

	if(newItem == NULL_QUEUE_ITEM)
	{
		if(w > self->renderQueue[self->renderQueueHead].w)
		{
			newItem = self->renderQueueHead;
			self->renderQueueHead = self->renderQueue[self->renderQueueHead].next;
		}
		else
		{
			//WARNING("Out of queue space!\n");
			return;
		}
	}

	self->renderQueue[newItem].x = x;
	self->renderQueue[newItem].w = w;
	self->renderQueue[newItem].frame = frame;
	self->renderQueue[newItem].data = spriteData;
	self->renderQueue[newItem].scaleDiv = scaleDiv ? scaleDiv : (CELL_SIZE / 2);

	if(self->renderQueueHead == NULL_QUEUE_ITEM)
	{
		self->renderQueueHead = newItem;
		self->renderQueue[newItem].next = NULL_QUEUE_ITEM;
		return;
	}
	else
	{
		if(w < self->renderQueue[self->renderQueueHead].w)
		{
			self->renderQueue[newItem].next = self->renderQueueHead;
			self->renderQueueHead = newItem;
		}
		else
		{
			for(uint8_t item = self->renderQueueHead; item != NULL_QUEUE_ITEM; item = self->renderQueue[item].next)
			{
				if(self->renderQueue[item].next == NULL_QUEUE_ITEM)
				{
					self->renderQueue[item].next = newItem;
					self->renderQueue[newItem].next = NULL_QUEUE_ITEM;
					break;
				}
				else if(w < self->renderQueue[self->renderQueue[item].next].w)
				{
					self->renderQueue[newItem].next = self->renderQueue[item].next;
					self->renderQueue[item].next = newItem;
					break;
				}
			}
		}
	}
#endif
}
void Renderer_drawQueuedSprite(Renderer* self, uint8_t id){
	if(id >= RENDER_QUEUE_CAPACITY || self->renderQueue[id].data == NULL || self->renderQueue[id].frame == NULL)
		return;

	uint8_t frameWidth = pgm_read_byte(&self->renderQueue[id].frame->width);
	uint8_t frameHeight = pgm_read_byte(&self->renderQueue[id].frame->height);
	if(frameWidth == 0 || frameHeight == 0 || self->renderQueue[id].w == 0)
		return;

	uint8_t scaleDiv = self->renderQueue[id].scaleDiv;
	if(scaleDiv == 0)
	{
		scaleDiv = CELL_SIZE / 2;
	}

	int16_t halfW = self->renderQueue[id].w >> 1;
	int16_t y2 = (HALF_DISPLAYHEIGHT) + halfW;
	int16_t y1 = y2 - (self->renderQueue[id].w * frameHeight) / scaleDiv;

	if(y2 < 0 || y1 >= DISPLAYHEIGHT)
		return;

	int16_t w = self->renderQueue[id].w;

	int16_t dx = (w * frameWidth) / scaleDiv;
	if(dx <= 0)
		return;

	int16_t sx1 = self->renderQueue[id].x - halfW + (w * pgm_read_byte(&self->renderQueue[id].frame->xOffset)) / scaleDiv;
	int16_t sx2 = sx1 + dx;
	int16_t firstx = max(sx1, 0);
	int16_t lastx = min(sx2, DISPLAYWIDTH - 1);
	int16_t uerror = dx;
	int8_t u = 0;
	int8_t du = frameWidth, ustep = 1;

	if(lastx < firstx)
		return;

#if WOLF3D_FAST_CLIPPED_SPRITES
	for(int16_t x = sx1; x < firstx; x++)
	{
		uerror -= du;
		while(u < frameWidth - 1 && uerror < 0)
		{
			u += ustep;
			uerror += dx;
		}
	}

	for(int16_t x = firstx; x <= lastx; x++)
#else
	for(int16_t x = sx1; x <= sx2; x++)
#endif
	{
#if WOLF3D_FAST_CLIPPED_SPRITES
		if(w > self->wbuffer[x])
#else
		if (x >= 0 && x < DISPLAYWIDTH && w > self->wbuffer[x])
#endif
		{        
			int16_t verror = halfW;
			int8_t v = 0;
			int16_t yStart = y2;

			BitPairReader textureReader;
			BitPairReader_init(&textureReader, self->renderQueue[id].data, pgm_read_word(&self->renderQueue[id].frame->offset) + frameHeight * u);
			uint8_t texData = BitPairReader_readFast(&textureReader);

#if WOLF3D_FAST_CLIPPED_SPRITES
			if(yStart >= DISPLAYHEIGHT)
			{
				for(int16_t y = yStart; y >= DISPLAYHEIGHT && v < frameHeight; y--)
				{
					Renderer_advanceSpriteV(w, &verror, &v, &textureReader, &texData);
				}
				yStart = DISPLAYHEIGHT - 1;
			}

			for(int16_t y = yStart; y >= y1 && y >= 0 && v < frameHeight; y--)
			{
				if(texData != 0)
				{
					Renderer_writeSpriteTexPixelFast((uint8_t)x, (uint8_t)y, texData);
				}
				Renderer_advanceSpriteV(w, &verror, &v, &textureReader, &texData);
			}
#else
			for(int16_t y = y2; y >= y1 && y >= 0 && v < frameHeight; y--)
			{
				if(y < DISPLAYHEIGHT && texData != 0)
				{
					Renderer_writeSpriteTexPixelFast((uint8_t)x, (uint8_t)y, texData);
				}
				Renderer_advanceSpriteV(w, &verror, &v, &textureReader, &texData);
			}
#endif
		}

		uerror -= du;

		if(dx > 0)
		{
			while(u < frameWidth - 1 && uerror < 0)
			{
				u += ustep;
				uerror += dx;
			}
		}
	}
}

void Renderer_drawGlyph(Renderer* self, char glyph, uint8_t x, uint8_t y){
	const uint8_t* ptr = Data_font + glyph * FONT_GLYPH_BYTE_SIZE;
	uint8_t readMask = 1;
	uint8_t read = pgm_read_byte(ptr++);

	for(int i = 0; i < FONT_WIDTH; i++)
	{
		for(int j = 0; j < FONT_HEIGHT; j++)
		{
			uint8_t colour = (read & readMask) ? WOLF3D_FONT_FOREGROUND_COLOUR : WOLF3D_FONT_BACKGROUND_COLOUR;
			Renderer_writePixelFastClipped((int16_t)x + i, (int16_t)y + j, colour);
			readMask <<= 1;
			if(readMask == 0)
			{
				readMask = 1;
				read = pgm_read_byte(ptr++);
			}
		}
//		clearPixel(x + i, y);
	//	clearPixel(x + i, y + FONT_HEIGHT + 1);
	}
	for(int j = 0; j < FONT_HEIGHT; j++)
	{
		Renderer_writePixelFastClipped((int16_t)x + FONT_WIDTH, (int16_t)y + j, WOLF3D_FONT_BACKGROUND_COLOUR);
	}
}
void Renderer_drawGlyphScaled(Renderer* self, char glyph, uint8_t x, uint8_t y, uint8_t scale, uint8_t fg, uint8_t bg){
	const uint8_t* ptr;
	uint8_t readMask;
	uint8_t read;

	if(scale == 0)
	{
		scale = 1;
	}
	if(glyph < 0)
	{
		return;
	}

	ptr = Data_font + ((uint8_t)glyph * FONT_GLYPH_BYTE_SIZE);
	readMask = 1;
	read = pgm_read_byte(ptr++);

	for(uint8_t i = 0; i < FONT_WIDTH; i++)
	{
		for(uint8_t j = 0; j < FONT_HEIGHT; j++)
		{
			uint8_t colour = (read & readMask) ? fg : bg;
			uint8_t px = (uint8_t)(x + (i * scale));
			uint8_t py = (uint8_t)(y + (j * scale));

			for(uint8_t sy = 0; sy < scale; sy++)
			{
				for(uint8_t sx = 0; sx < scale; sx++)
				{
					Renderer_writePixelFastClipped((int16_t)px + sx, (int16_t)py + sy, colour);
				}
			}

			readMask <<= 1;
			if(readMask == 0)
			{
				readMask = 1;
				read = pgm_read_byte(ptr++);
			}
		}
	}

	for(uint8_t sx = 0; sx < scale; sx++)
	{
		for(uint8_t j = 0; j < (FONT_HEIGHT * scale); j++)
		{
			Renderer_writePixelFastClipped((int16_t)x + (FONT_WIDTH * scale) + sx, (int16_t)y + j, bg);
		}
	}
}
void Renderer_drawStringScaled(Renderer* self, const char* str, uint8_t x, uint8_t y, uint8_t scale, uint8_t fg, uint8_t bg){
	const char* ptr = str;
	char current = 0;
	uint8_t startX = x;
	uint8_t advance;
	uint8_t lineAdvance;

	if(scale == 0)
	{
		scale = 1;
	}
	advance = (uint8_t)((FONT_WIDTH + 1) * scale);
	lineAdvance = (uint8_t)((FONT_HEIGHT + 1) * scale);

	while(1)
	{
		current = pgm_read_byte(ptr++);
		if(current == 0)
		{
			break;
		}
		if(current == '\n')
		{
			x = startX;
			y = (uint8_t)(y + lineAdvance);
			continue;
		}
		if(current >= FIRST_FONT_GLYPH && current <= LAST_FONT_GLYPH)
		{
			Renderer_drawGlyphScaled(self, current - FIRST_FONT_GLYPH, x, y, scale, fg, bg);
		}
		x = (uint8_t)(x + advance);
	}
}
void Renderer_drawString(Renderer* self, const char* str, uint8_t x, uint8_t y){
	const char* ptr = str;
	char current = 0;
	uint8_t startX = x;

	do
	{
		current = pgm_read_byte(ptr);
		ptr++;

		if(current >= FIRST_FONT_GLYPH && current <= LAST_FONT_GLYPH)
		{
			Renderer_drawGlyph(self, current - FIRST_FONT_GLYPH, x, y);
		}

		x += FONT_WIDTH + 1;

		if(current == '\n')
		{
			x = startX;
			y += FONT_HEIGHT + 1;
		}
	} while(current);
}
void Renderer_drawInt(Renderer* self, int8_t val, uint8_t x, uint8_t y){
	unsigned char c, i;

	for(i = 0; i < 3; i++)
	{
		c = val % 10;
		if(val > 0 || i == 0) 
		{
			Renderer_drawGlyph(self, c + '0' - FIRST_FONT_GLYPH, x, y);
		}
		else
		{
			Renderer_drawGlyph(self, ' ' - FIRST_FONT_GLYPH, x, y);
		}
		x -= FONT_WIDTH + 1;
		val = val / 10;
	}
}

bool Renderer_isFrustrumClipped(Renderer* self, int16_t x, int16_t z)
{
	if((self->view.clipCos * (x - self->view.cellX) - self->view.clipSin * (z - self->view.cellZ)) < -FIXED_ONE)
		return true;
	if((self->view.clipSin * (x - self->view.cellX) + self->view.clipCos * (z - self->view.cellZ)) < -FIXED_ONE)
		return true;
	return false;
}

void BitPairReader_init(BitPairReader* self, const uint8_t* ptr, uint16_t offset)
{
	uint16_t byteOffset = offset >> 2;
	self->m_readOffset = (uint8_t)((offset - (byteOffset << 2)) << 1);
	self->m_ptr = ptr + byteOffset;
	self->m_lastRead = pgm_read_byte(self->m_ptr);
}

uint8_t BitPairReader_read(BitPairReader* self)
{
	return BitPairReader_readFast(self);
}
