#ifndef SLOPOS_FONT8X8_H
#define SLOPOS_FONT8X8_H

#include "types.h"

/* 8x8 bitmap font indexed by ASCII (32..126). 8 bytes per glyph. */
extern const u8 font8x8[95][8];

#define FONT8X8_WIDTH  8
#define FONT8X8_HEIGHT 8

#endif
