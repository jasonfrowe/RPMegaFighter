#include <rp6502.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "constants.h"
#include "random.h"
#include "graphics.h"
#include "bomber.h"
#include "player.h"

// Bomber State
typedef struct {
    bool active;
    int16_t x, y;       // Integer screen/world coordinates
    int16_t rx, ry;     // Remainders (Accumulators for sub-pixel movement)
    int health;
} bomber_t;

#define BOMBER_SPEED_SUBPIXEL 20

extern int16_t scroll_dx, scroll_dy;
extern int16_t earth_x, earth_y;

bomber_t bomber = { .active = false };

void spawn_bomber(int level) {
    if (bomber.active) return;

    bomber.active = true;
    // Health scales with level (e.g., Level 1 = 10 hits, Level 5 = 30 hits)
    bomber.health = 10 + (level * 5); 

    // Initialize remainders to 0 (center of pixel)
    bomber.rx = 0;
    bomber.ry = 0;

    // Spawn Logic: Pick a random edge of the World (1024x1024)
    // We want it far from Earth so it has to travel.
    // if (rand16() & 1) {
    //     // Spawn Left or Right edge
    //     bomber.x = (rand16() & 1) ? 0 : WORLD_X2; 
    //     bomber.y = random(0, WORLD_Y2);
    // } else {
    //     // Spawn Top or Bottom edge
    //     bomber.x = random(0, WORLD_X2);
    //     bomber.y = (rand16() & 1) ? 0 : WORLD_Y2;
    // }

    if (rand16() & 1) {
        // Option A: Spawn on Left (-512) or Right (+512) Edge
        // 50% chance for Left or Right
        bomber.x = (rand16() & 1) ? -WORLD_X2 : WORLD_X2;
        
        // Y can be anywhere from -512 to +512
        // random(0, 1024) gives 0..1024. Subtract 512 to get -512..+512
        bomber.y = (int16_t)random(0, WORLD_Y) - WORLD_Y2;
        
    } else {
        // Option B: Spawn on Top (-512) or Bottom (+512) Edge
        
        // X can be anywhere from -512 to +512
        bomber.x = (int16_t)random(0, WORLD_X) - WORLD_X2;
        
        // 50% chance for Top or Bottom
        bomber.y = (rand16() & 1) ? -WORLD_Y2 : WORLD_Y2;
    }

    // Initialize Sprite Config (Mode 4 Swarm)
    xram0_struct_set(BOMBER_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, BOMBER_DATA);
    xram0_struct_set(BOMBER_CONFIG, vga_mode4_sprite_t, log_size, 3); // 3 = 8x8
    xram0_struct_set(BOMBER_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);
    
    printf("WARNING: Bomber Spawned at %d, %d\n", (int)bomber.x, (int)bomber.y);
}

void update_bomber(void) {
    if (!bomber.active) {
        xram0_struct_set(BOMBER_CONFIG, vga_mode4_sprite_t, y_pos_px, -100);
        return;
    }

    // ---------------------------------------------------------
    // 1. MOVEMENT LOGIC (Integer + Remainder)
    // ---------------------------------------------------------
    
    // X Axis Movement
    if (bomber.x < earth_x) {
        bomber.rx += BOMBER_SPEED_SUBPIXEL;
        if (bomber.rx >= 256) {
            bomber.x++;
            bomber.rx -= 256;
        }
    } else if (bomber.x > earth_x) {
        bomber.rx -= BOMBER_SPEED_SUBPIXEL;
        if (bomber.rx <= -256) {
            bomber.x--;
            bomber.rx += 256;
        }
    }

    // Y Axis Movement
    if (bomber.y < earth_y) {
        bomber.ry += BOMBER_SPEED_SUBPIXEL;
        if (bomber.ry >= 256) {
            bomber.y++;
            bomber.ry -= 256;
        }
    } else if (bomber.y > earth_y) {
        bomber.ry -= BOMBER_SPEED_SUBPIXEL;
        if (bomber.ry <= -256) {
            bomber.y--;
            bomber.ry += 256;
        }
    }

    // ---------------------------------------------------------
    // 2. APPLY SCROLL (Camera Movement)
    // ---------------------------------------------------------
    // Since everything is integers now, this is perfectly accurate
    bomber.x -= scroll_dx;
    bomber.y -= scroll_dy;

    // ---------------------------------------------------------
    // 3. WORLD WRAPPING
    // ---------------------------------------------------------
    // Wrap Earth horizontally
    if (earth_x <= -WORLD_X2) {
        earth_x += WORLD_X;
        bomber.x += WORLD_X;
    } else if (earth_x > WORLD_X2) {
        earth_x -= WORLD_X;
        bomber.x -= WORLD_X;
    }
    
    // Wrap Earth vertically
    if (earth_y <= -WORLD_Y2) {
        earth_y += WORLD_Y;
        bomber.y += WORLD_Y;
    } else if (earth_y > WORLD_Y2) {
        earth_y -= WORLD_Y;
        bomber.y -= WORLD_Y;
    }

    // printf("Bomber Pos: %d, %d | Earth Pos: %d, %d\n", (int)bomber.x, (int)bomber.y, (int)earth_x, (int)earth_y);

    // ---------------------------------------------------------
    // 4. RENDER
    // ---------------------------------------------------------
    // No casting needed, values are stable integers
    xram0_struct_set(BOMBER_CONFIG, vga_mode4_sprite_t, x_pos_px, bomber.x);
    xram0_struct_set(BOMBER_CONFIG, vga_mode4_sprite_t, y_pos_px, bomber.y);

    // ---------------------------------------------------------
    // 5. COLLISION (Using Earth struct properties)
    // ---------------------------------------------------------
    // Simple box check (Bomber 8x8 vs Earth 32x32)
    // We check center points or overlapping boxes
    
    // Earth center approx (assuming earth_x is top-left + 16)
    // int ex_center = earth_x + 16;
    // int ey_center = earth_y + 16;
    // int bx_center = bomber.x + 4;
    // int by_center = bomber.y + 4;
    
    // if (abs(ex_center - bx_center) < 20 && abs(ey_center - by_center) < 20) {
    //      // Explosion Logic Here
    //      start_explosion(bomber.x, bomber.y); // visual only
    //      bomber.active = false;
         
    //      // Trigger Game Over
    //      game_over = true;
    //      printf("Earth has been destroyed!\n");
    // }
}