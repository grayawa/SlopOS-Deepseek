#ifndef SLOPOS_FONT8X16_H
#define SLOPOS_FONT8X16_H

#include "types.h"

/* 8x16 bitmap font indexed by ASCII (32..126). 16 bytes per glyph. */
extern const u8 font8x16[95][16];

#define FONT8X16_WIDTH  8
#define FONT8X16_HEIGHT 16

#endif
