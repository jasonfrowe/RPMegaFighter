#include "asteroids.h"
#include "constants.h"      // Needs ASTEROID_M_DATA (We reuse Med data for Large)
#include "player.h"  // Needs scroll_x, scroll_y, WORLD dimensions
#include "random.h"         // Needs rand16(), random()
#include <rp6502.h>
#include <stdlib.h>
#include <stdio.h>

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
    // size_t struct_size = sizeof(vga_mode4_asprite_t);

    // for (int i = 0; i < MAX_AST_L; i++) {
    //     unsigned ptr = ASTEROID_L_CONFIG + (i * struct_size);
        
    //     // Move offscreen immediately
    //     xram0_struct_set(ptr, vga_mode4_asprite_t, x_pos_px, -100);
    //     xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, -100);
    // }
}

// ---------------------------------------------------------
// 2. SPAWNING
// ---------------------------------------------------------
void spawn_asteroids(void) {
    size_t struct_size = sizeof(vga_mode4_asprite_t);

    for (int i = 0; i < MAX_AST_L; i++) {
        if (ast_l[i].active) continue;

        ast_l[i].active = true;
        // Spawn Randomly
        ast_l[i].x = (int16_t)random(20, SCREEN_WIDTH - 20);
        ast_l[i].y = (int16_t)random(20, SCREEN_HEIGHT - 20);
        ast_l[i].rx = 0;
        ast_l[i].ry = 0;
        ast_l[i].vx = (rand16() & 1) ? 20 : -20; 
        ast_l[i].vy = (rand16() & 1) ? 20 : -20;
        ast_l[i].anim_frame = random(0, 3);

        unsigned ptr = ASTEROID_L_CONFIG + (i * struct_size);

        // Update sprite position (transform matrix already set in init_graphics)
        xram0_struct_set(ptr, vga_mode4_asprite_t, x_pos_px, ast_l[i].x);
        xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, ast_l[i].y);

        // xram0_struct_set(ptr, vga_mode4_asprite_t, transform[0], 0x0080);  // cos = 1.0
        // xram0_struct_set(ptr, vga_mode4_asprite_t, transform[1], 0);  // -sin = 0
        // xram0_struct_set(ptr, vga_mode4_asprite_t, transform[2], 0);  // x_offset (8 pixels for 16x16)
        // xram0_struct_set(ptr, vga_mode4_asprite_t, transform[3], 0);  // sin = 0
        // xram0_struct_set(ptr, vga_mode4_asprite_t, transform[4], 0x0080);  // cos = 1.0
        // xram0_struct_set(ptr, vga_mode4_asprite_t, transform[5], 0);  // y_offset (8 pixels for 16x16)
        
        printf("Spawned Asteroid %d at %d, %d (Size 16x16)\n", i, ast_l[i].x, ast_l[i].y);
    }
}

// ---------------------------------------------------------
// 3. UPDATE LOOP
// ---------------------------------------------------------
void update_asteroids(void) {
    size_t struct_size = sizeof(vga_mode4_asprite_t);

    // for (int i = 0; i < MAX_AST_L; i++) {
    for (int i = 0; i < 1; i++) {
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
        int screen_x = ast_l[i].x;
        int screen_y = ast_l[i].y;

        // unsigned ptr = ASTEROID_L_CONFIG + (i * struct_size);
        
        // xram0_struct_set(ptr, vga_mode4_asprite_t, x_pos_px, ast_l[i].x);
        // xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, ast_l[i].y);

        // Update sprite position
        unsigned ptr = ASTEROID_L_CONFIG + i * sizeof(vga_mode4_asprite_t);
        RIA.step0 = sizeof(vga_mode4_asprite_t);
        RIA.step1 = sizeof(vga_mode4_asprite_t);
        RIA.addr0 =  ptr + 12;
        RIA.addr1 =  ptr + 13;
        
        RIA.rw0 = screen_x & 0xFF;
        RIA.rw1 = (screen_x >> 8) & 0xFF;
        
        RIA.addr0 = ptr + 14;
        RIA.addr1 = ptr + 15;
        RIA.rw0 = screen_y & 0xFF;
        RIA.rw1 = (screen_y >> 8) & 0xFF;

        // printf("Asteroid %d at %d, %d\n", i, ast_l[i].x, ast_l[i].y);
        
        // // Optional: Simple Animation (Cycle frames)
        // if ((game_frame % 10) == 0) {
        //     ast_l[i].anim_frame = (ast_l[i].anim_frame + 1) % 4;
        //     // Frame offset: 16x16 pixels = 256 pixels * 2 bytes = 512 bytes
        //     uint16_t offset = ast_l[i].anim_frame * 512;
        //     xram0_struct_set(ptr, vga_mode4_asprite_t, xram_sprite_ptr, (uint16_t)(ASTEROID_M_DATA + offset));
        // }
    }
}