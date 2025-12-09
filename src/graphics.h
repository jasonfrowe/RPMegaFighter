#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "constants.h"
#include <stdlib.h>
#include <rp6502.h>

// Swap macro for line drawing
#define swap(a, b) { uint16_t t = a; a = b; b = t; }

// Routine for placing a single dot on the screen for 8bit-colour depth
static inline void set(int16_t x, int16_t y, uint8_t colour)
{
    RIA.addr0 =  x + (SCREEN_WIDTH * y);
    RIA.step0 = 1;
    RIA.rw0 = colour;
}

// ---------------------------------------------------------------------------
// Draw a straight line from (x0,y0) to (x1,y1) with given color
// using Bresenham's algorithm
// ---------------------------------------------------------------------------
static inline void draw_line(uint16_t colour, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    int16_t dx, dy;
    int16_t err;
    int16_t ystep;
    int16_t steep = abs((int16_t)y1 - (int16_t)y0) > abs((int16_t)x1 - (int16_t)x0);

    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }

    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }

    dx = x1 - x0;
    dy = abs((int16_t)y1 - (int16_t)y0);

    err = dx / 2;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            set(y0, x0, colour);
        } else {
            set(x0, y0, colour);
        }

        err -= dy;

        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

// ---------------------------------------------------------------------------
// Draw a localized explosion flash effect around a point
// Creates radiating debris pattern with minimal pixel writes
// Parameters:
//   cx, cy: center point of explosion
//   radius: size of the effect (typically 8-16)
//   density: number of debris particles (4-8 recommended)
//   color_base: starting color index (will vary for each particle)
// ---------------------------------------------------------------------------
static inline void draw_explosion_flash(int16_t cx, int16_t cy, uint8_t radius, uint8_t density, uint8_t color_base)
{
    // Use a static counter for animation (rotates debris pattern over time)
    static uint8_t anim_offset = 0;
    anim_offset++;
    
    // Draw radiating debris lines from center
    for (uint8_t i = 0; i < density; i++) {
        // Add animation offset to rotate the pattern
        uint8_t dir = ((i * 8) / density + (anim_offset / 4)) % 8; // Map to 8 directions with rotation
        
        // Simple directional offsets (8 directions for efficiency)
        int16_t dx = 0, dy = 0;
        uint8_t is_cardinal = 0; // Flag for vertical/horizontal directions
        
        switch(dir) {
            case 0: dx = radius; dy = 0; is_cardinal = 1; break;           // Right (cardinal)
            case 1: dx = radius; dy = -radius; break;                       // Up-Right (diagonal)
            case 2: dx = 0; dy = -radius; is_cardinal = 1; break;          // Up (cardinal)
            case 3: dx = -radius; dy = -radius; break;                      // Up-Left (diagonal)
            case 4: dx = -radius; dy = 0; is_cardinal = 1; break;          // Left (cardinal)
            case 5: dx = -radius; dy = radius; break;                       // Down-Left (diagonal)
            case 6: dx = 0; dy = radius; is_cardinal = 1; break;           // Down (cardinal)
            case 7: dx = radius; dy = radius; break;                        // Down-Right (diagonal)
        }
        
        // Draw debris streak - cardinal directions (vertical/horizontal) are longer for diffraction effect
        uint8_t base_len = is_cardinal ? 6 : 3; // Cardinals 2x longer than diagonals
        uint8_t streak_len = base_len + ((anim_offset + i) % 3); // Vary streak length
        uint8_t color = color_base + (i * 30); // Vary color for each particle
        int16_t x = cx;
        int16_t y = cy;
        
        for (uint8_t j = 0; j < streak_len; j++) {
            x += dx / 4;
            y += dy / 4;
            
            // Bounds check
            if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                set(x, y, color);
            }
        }
    }
}

#endif // GRAPHICS_H