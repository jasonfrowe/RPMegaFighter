#include "asteroids.h"
#include "constants.h"      // Needs ASTEROID_M_DATA (We reuse Med data for Large)
#include "player.h"  // Needs scroll_x, scroll_y, WORLD dimensions
#include "random.h"         // Needs rand16(), random()
#include <rp6502.h>
#include <stdlib.h>

// Global Pool
asteroid_t ast_l[MAX_AST_L];

// Extern the Config Address calculated in init_memory_map()
// This points to the start of the Affine Sprite block for Asteroids
extern unsigned ASTEROID_L_CONFIG;

// ---------------------------------------------------------
// 1. INITIALIZATION
// ---------------------------------------------------------
void init_asteroids(void) {
    // 1. Reset CPU State
    for (int i = 0; i < MAX_AST_L; i++) {
        ast_l[i].active = false;
        ast_l[i].x = 0;
        ast_l[i].y = 0;
    }

    // 2. Clear GPU State (Critical for preventing ghosts/corruption)
    // Large Asteroids use 'vga_mode4_asprite_t' (32 bytes)
    size_t struct_size = sizeof(vga_mode4_asprite_t);

    for (int i = 0; i < MAX_AST_L; i++) {
        unsigned ptr = ASTEROID_L_CONFIG + (i * struct_size);
        
        // Move offscreen immediately
        xram0_struct_set(ptr, vga_mode4_asprite_t, x_pos_px, -100);
        xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, -100);
    }
}

// ---------------------------------------------------------
// 2. SPAWNING
// ---------------------------------------------------------
void spawn_asteroids(void) {
    size_t struct_size = sizeof(vga_mode4_asprite_t);

    for (int i = 0; i < MAX_AST_L; i++) {
        // If already active, skip
        if (ast_l[i].active) continue;

        // Activate
        ast_l[i].active = true;
        
        // Spawn at random locations (simple logic for test)
        // Range: -512 to +512
        ast_l[i].x = (int16_t)random(0, 1024) - 512;
        ast_l[i].y = (int16_t)random(0, 1024) - 512;
        ast_l[i].rx = 0;
        ast_l[i].ry = 0;

        // Random slow velocity
        // 20 subpixels approx 0.08 pixels/frame
        ast_l[i].vx = (rand16() & 1) ? 20 : -20; 
        ast_l[i].vy = (rand16() & 1) ? 20 : -20;
        
        ast_l[i].anim_frame = random(0, 3);

        // --- CONFIGURE XRAM ---
        unsigned ptr = ASTEROID_L_CONFIG + (i * struct_size);

        // 1. Point to Pixel Data
        // Optimization: Large reuse Medium Data (0xF480)
        // Cast to uint16_t to prevent compiler warnings
        xram0_struct_set(ptr, vga_mode4_asprite_t, xram_sprite_ptr, (uint16_t)ASTEROID_M_DATA);

        // 2. Set Size (16x16 source)
        xram0_struct_set(ptr, vga_mode4_asprite_t, log_size, 4);

        // 3. Set Scale to 2.0 (Double size -> 32x32)
        // Fixed point 8.8: 0x0100 = 1.0, 0x0200 = 2.0
        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[0], 0x0200); // Scale X
        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[3], 0x0200); // Scale Y
        
        // 4. Reset Rotation/Shear elements to 0
        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[1], 0);
        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[2], 0);
    }
}

// ---------------------------------------------------------
// 3. UPDATE LOOP
// ---------------------------------------------------------
void update_asteroids(void) {
    size_t struct_size = sizeof(vga_mode4_asprite_t);

    for (int i = 0; i < MAX_AST_L; i++) {
        if (!ast_l[i].active) continue;

        // A. Move (Fixed Point Math)
        ast_l[i].rx += ast_l[i].vx;
        ast_l[i].ry += ast_l[i].vy;

        if (ast_l[i].rx >= 256) { ast_l[i].x++; ast_l[i].rx -= 256; }
        else if (ast_l[i].rx <= -256) { ast_l[i].x--; ast_l[i].rx += 256; }

        if (ast_l[i].ry >= 256) { ast_l[i].y++; ast_l[i].ry -= 256; }
        else if (ast_l[i].ry <= -256) { ast_l[i].y--; ast_l[i].ry += 256; }

        ast_l[i].x -= scroll_dx;
        ast_l[i].y -= scroll_dy;

        // B. World Wrap (-WORLD_WIDTH/2 to WORLD_WIDTH/2)
        if (ast_l[i].x < -WORLD_X2) ast_l[i].x += WORLD_X;
        if (ast_l[i].x >  WORLD_X2) ast_l[i].x -= WORLD_X;
        if (ast_l[i].y < -WORLD_Y2) ast_l[i].y += WORLD_Y;
        if (ast_l[i].y >  WORLD_Y2) ast_l[i].y -= WORLD_Y;

        // C. Render
        // int screen_x = ast_l[i].x - scroll_dx;
        // int screen_y = ast_l[i].y - scroll_dy;

        unsigned ptr = ASTEROID_L_CONFIG + (i * struct_size);
        
        xram0_struct_set(ptr, vga_mode4_asprite_t, x_pos_px, ast_l[i].x);
        xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, ast_l[i].y);
        
        // // Optional: Simple Animation (Cycle frames)
        // if ((game_frame % 10) == 0) {
        //     ast_l[i].anim_frame = (ast_l[i].anim_frame + 1) % 4;
        //     // Frame offset: 16x16 pixels = 256 pixels * 2 bytes = 512 bytes
        //     uint16_t offset = ast_l[i].anim_frame * 512;
        //     xram0_struct_set(ptr, vga_mode4_asprite_t, xram_sprite_ptr, (uint16_t)(ASTEROID_M_DATA + offset));
        // }
    }
}