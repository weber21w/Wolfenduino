#ifndef RENDERER_H_
#define RENDERER_H_

#include "Defines.h"
#include "FixedMath.h"
#include "SpriteFrame.h"

#define NULL_QUEUE_ITEM 0xff
#define RENDER_QUEUE_CAPACITY 8

typedef struct RenderQueueItem
{
	const SpriteFrame* frame;
	const uint8_t* data;
	uint8_t x, w;
	uint8_t scaleDiv;
	uint8_t next;
} RenderQueueItem;

typedef struct Renderer
{
	int8_t damageIndicator;
	uint8_t hudHealthFlash;
	uint8_t hudAmmoFlash;
	struct
	{
		int16_t x, z;
		int16_t cellX, cellZ;
		int16_t rotCos, rotSin;
		int16_t clipCos, clipSin;
	} view;
	uint8_t wbuffer[DISPLAYWIDTH];
#ifdef DEFER_RENDER
	uint8_t texbuffer[DISPLAYWIDTH];
	uint8_t ubuffer[DISPLAYWIDTH];
#if WOLF3D_FAST_DEFERRED_WALL_BOUNDS
	uint8_t wallFirstX;
	uint8_t wallLastX;
#endif
#endif
	uint8_t renderQueueHead;
	RenderQueueItem renderQueue[RENDER_QUEUE_CAPACITY];
} Renderer;

typedef struct BitPairReader
{
	const uint8_t* m_ptr;
	uint8_t m_lastRead;
	uint8_t m_readOffset;
} BitPairReader;

void Renderer_init(Renderer* self);
void Renderer_drawFrame(Renderer* self);
void Renderer_queueSprite(Renderer* self, const SpriteFrame* frame, const uint8_t* sprite, int16_t x, int16_t z);
void Renderer_queueSpriteScaled(Renderer* self, const SpriteFrame* frame, const uint8_t* sprite, int16_t x, int16_t z, uint8_t scaleDiv);
void Renderer_drawGlyph(Renderer* self, char glyph, uint8_t x, uint8_t y);
void Renderer_drawString(Renderer* self, const char* str, uint8_t x, uint8_t y);
void Renderer_drawGlyphScaled(Renderer* self, char glyph, uint8_t x, uint8_t y, uint8_t scale, uint8_t fg, uint8_t bg);
void Renderer_drawStringScaled(Renderer* self, const char* str, uint8_t x, uint8_t y, uint8_t scale, uint8_t fg, uint8_t bg);
void Renderer_drawInt(Renderer* self, int8_t val, uint8_t x, uint8_t y);
void Renderer_drawDeferredFrame(Renderer* self);
bool Renderer_isFrustrumClipped(Renderer* self, int16_t x, int16_t z);
void Renderer_initWBuffer(Renderer* self);
void Renderer_drawFloorAndCeiling(Renderer* self);
void Renderer_drawCell(Renderer* self, int8_t cellX, int8_t cellZ);
void Renderer_drawStrip(Renderer* self, int16_t x, int16_t w, int8_t u, uint8_t textureId);
void Renderer_drawWall_impl(Renderer* self, int16_t _x1, int16_t _z1, int16_t _x2, int16_t _z2, uint8_t textureId, int8_t _u1, int8_t _u2);
#define Renderer_drawWall_6(self, _x1, _z1, _x2, _z2, textureId) Renderer_drawWall_impl((self), (_x1), (_z1), (_x2), (_z2), (textureId), 0, 15)
#define Renderer_drawWall_8(self, _x1, _z1, _x2, _z2, textureId, _u1, _u2) Renderer_drawWall_impl((self), (_x1), (_z1), (_x2), (_z2), (textureId), (_u1), (_u2))
#define RENDERER_WALL_CHOOSER(_1,_2,_3,_4,_5,_6,_7,_8,NAME,...) NAME
#define Renderer_drawWall(...) RENDERER_WALL_CHOOSER(__VA_ARGS__, Renderer_drawWall_8, bad_arg_count, Renderer_drawWall_6)(__VA_ARGS__)
void Renderer_drawFrustumCells(Renderer* self);
void Renderer_drawBufferedCells(Renderer* self);
void Renderer_drawDoors(Renderer* self);
void Renderer_drawQueuedSprite(Renderer* self, uint8_t id);
void Renderer_drawWeapon(Renderer* self);
void Renderer_drawDamage(Renderer* self);
void Renderer_drawHud(Renderer* self);
void Renderer_drawMap(Renderer* self);
#if WOLF3D_SEEN_WALL_BITSET
void Renderer_updateSeenWalls(Renderer* self);
#endif

void BitPairReader_init(BitPairReader* self, const uint8_t* ptr, uint16_t offset);
uint8_t BitPairReader_read(BitPairReader* self);

#endif
