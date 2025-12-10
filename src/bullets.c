#include "bullets.h"
#include "constants.h"
#include "fighters.h"
#include "sound.h"
#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include "sbullets.h"
#include "asteroids.h"
#include <stdio.h>

// ============================================================================
// CONSTANTS
// ============================================================================

// ============================================================================
// EXTERNAL DEPENDENCIES
// ============================================================================

// Game state from main
extern int16_t player_score;
extern int16_t game_score;

// Sprite configuration addresses
extern unsigned BULLET_CONFIG;

// Lookup tables from definitions.h
extern const int16_t sin_fix[25];
extern const int16_t cos_fix[25];

// Collision check from fighters module
extern bool check_asteroid_hit(int16_t x, int16_t y);

// Collision check from asteroids module
// extern bool check_asteroid_hit(int16_t bx, int16_t by);

// ============================================================================
// MODULE STATE
// ============================================================================

// Player bullets (exported for use by player.c)
Bullet bullets[MAX_BULLETS];
uint8_t current_bullet_index = 0;
int16_t active_bullet_count = 0;  // Track active bullets for optimization (exported)

// Dirty flags: track which sprites need XRAM updates (1 bit per bullet)
static uint8_t bullet_sprite_dirty = 0xFF; // All dirty initially

// Spread shot bullets (internal to this module for now)
static Bullet sbullets[MAX_SBULLETS];
// static uint16_t sbullet_cooldown = 0;
// static uint8_t current_sbullet_index = 0;

// ============================================================================
// FUNCTIONS
// ============================================================================

void init_bullets(void)
{
    active_bullet_count = 0;
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        bullets[i].status = -1;
        bullets[i].x = 0;
        bullets[i].y = 0;
        bullets[i].vx_rem = 0;
        bullets[i].vy_rem = 0;
    }
    bullet_sprite_dirty = 0xFF; // Mark all for initial cleanup
    
    // Note: ebullets initialized in init_fighters()
    
    for (uint8_t i = 0; i < MAX_SBULLETS; i++) {
        sbullets[i].status = -1;
    }
}

void update_bullets(void)
{
    // Early exit if no active bullets
    if (active_bullet_count == 0) {
        // Still need to clean up any dirty sprites
        if (bullet_sprite_dirty != 0) {
            for (uint8_t i = 0; i < MAX_BULLETS; i++) {
                uint8_t mask = 1 << i;
                if (bullet_sprite_dirty & mask) {
                    unsigned ptr = BULLET_CONFIG + i * sizeof(vga_mode4_sprite_t);
                    xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
                    bullet_sprite_dirty &= ~mask;
                }
            }
        }
        return;
    }
    
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        uint8_t mask = 1 << i;
        
        if (bullets[i].status < 0) {
            // Only update sprite if dirty (just became inactive)
            if (bullet_sprite_dirty & mask) {
                unsigned ptr = BULLET_CONFIG + i * sizeof(vga_mode4_sprite_t);
                xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
                bullet_sprite_dirty &= ~mask; // Clear dirty flag
            }
            continue;  // Bullet is inactive
        }
        
        // Check collision with fighters before moving
        if (check_bullet_fighter_collision(bullets[i].x, bullets[i].y, &player_score, &game_score)) {
            // Hit! Remove bullet
            bullets[i].status = -1;
            active_bullet_count--;
            bullet_sprite_dirty |= mask; // Mark for cleanup next frame
            goto next_bullet;  // Skip rest of bullet update
        }

        // Interleaved asteroid collision: Check every other bullet per frame
        // This reduces checks from 8/frame to ~4/frame
        if ((i & 1) == (game_frame & 1)) {
            if (check_asteroid_hit(bullets[i].x, bullets[i].y)) {
                bullets[i].status = -1; // Kill bullet
                active_bullet_count--;
                bullet_sprite_dirty |= mask; // Mark for cleanup
                goto next_bullet; // Move to next bullet
            }
        }
        
        // Get velocity components based on bullet direction
        int16_t bvx = -sin_fix[bullets[i].status];
        int16_t bvy = -cos_fix[bullets[i].status];
        
        // Apply velocity with fixed-point math (divide by 64 for bullet speed)
        int16_t bvx_applied = (bvx + bullets[i].vx_rem) >> 6;
        int16_t bvy_applied = (bvy + bullets[i].vy_rem) >> 6;
        
        // Update remainder
        bullets[i].vx_rem = bvx + bullets[i].vx_rem - (bvx_applied << 6);
        bullets[i].vy_rem = bvy + bullets[i].vy_rem - (bvy_applied << 6);
        
        // Update bullet position
        bullets[i].x += bvx_applied;
        bullets[i].y += bvy_applied;
        
        // Check if bullet is still on screen
        if (bullets[i].x > 0 && bullets[i].x < SCREEN_WIDTH && 
            bullets[i].y > 0 && bullets[i].y < SCREEN_HEIGHT) {
            // Update sprite hardware position (always update active bullets)
            unsigned ptr = BULLET_CONFIG + i * sizeof(vga_mode4_sprite_t);
            xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, bullets[i].x);
            xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, bullets[i].y);
        } else {
            // Bullet went off screen, deactivate it
            bullets[i].status = -1;
            active_bullet_count--;
            bullet_sprite_dirty |= mask; // Mark for cleanup
        }
        
    next_bullet:
        continue;
    }
}
