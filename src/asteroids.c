#include "asteroids.h"
#include "constants.h"      // Needs ASTEROID_M_DATA, game_frame
#include "player.h"         // Needs scroll_dx, scroll_dy
#include "random.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <rp6502.h>
#include <stdlib.h>
#include "explosions.h"    // Needs start_explosion()   
#include "text.h"           // For score display update

// Rotation Tables (Reuse from player.c)
extern const int16_t sin_fix[];
extern const int16_t cos_fix[];

const int16_t t2_fix32[] = {
       0, 1152, 2560, 4064, 5536, 6944, 8128, 9056, 9632, 9856, 
    9632, 9056, 8128, 6944, 5536, 4064, 2560, 1152,    0, -928, 
   -1504, -1728, -1504, -928,    0
};

#define MAX_ROTATION 24

// Globals
asteroid_t ast_l[MAX_AST_L];
asteroid_t ast_m[MAX_AST_M];
asteroid_t ast_s[MAX_AST_S];

// Active counts for early-exit optimization
static uint8_t active_ast_l_count = 0;
static uint8_t active_ast_m_count = 0;
static uint8_t active_ast_s_count = 0;

// Config Addresses (From rpmegafighter.c)
extern unsigned ASTEROID_L_CONFIG;
extern unsigned ASTEROID_M_CONFIG;
extern unsigned ASTEROID_S_CONFIG;


extern void start_explosion(int16_t x, int16_t y);

extern int16_t scroll_dx, scroll_dy;
extern int player_score, enemy_score;
extern int16_t game_score, game_level;

// Asteroid World Boundaries
#define AWORLD_PAD 100  // Extra padding beyond screen edges
#define AWORLD_X1 -AWORLD_PAD  // World boundaries
#define AWORLD_X2 (SCREEN_WIDTH + AWORLD_PAD)  // World boundaries
#define AWORLD_Y1 -AWORLD_PAD  // World boundaries
#define AWORLD_Y2 (SCREEN_HEIGHT + AWORLD_PAD)  // World boundaries
#define AWORLD_X (AWORLD_X2 - AWORLD_X1)  // Total world width
#define AWORLD_Y (AWORLD_Y2 - AWORLD_Y1)  // Total world height

// Inline collision helpers for hot paths
static inline bool box_collision(int16_t dx, int16_t dy, int16_t radius) {
    return (dx > -radius && dx < radius && dy > -radius && dy < radius);
}

static inline bool broad_phase_check(int16_t dx, int16_t dy, int16_t margin) {
    return (dx >= -margin && dx <= margin && dy >= -margin && dy <= margin);
}

// ---------------------------------------------------------
// INITIALIZATION
// ---------------------------------------------------------
void init_asteroids(void) {
    // Reset active counters
    active_ast_l_count = 0;
    active_ast_m_count = 0;
    active_ast_s_count = 0;
    
    // 1. Reset Large (Affine)
    size_t size_l = sizeof(vga_mode4_asprite_t);
    for (int i=0; i<MAX_AST_L; i++) {
        ast_l[i].active = false;
        unsigned ptr = ASTEROID_L_CONFIG + (i * size_l);
        xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, -100); // Hide
    }
    
    // 2. Reset Medium (Standard)
    size_t size_std = sizeof(vga_mode4_sprite_t);
    for (int i=0; i<MAX_AST_M; i++) {
        ast_m[i].active = false;
        unsigned ptr = ASTEROID_M_CONFIG + (i * size_std);
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
    }
    
    // 3. Reset Small (Standard)
    for (int i=0; i<MAX_AST_S; i++) {
        ast_s[i].active = false;
        unsigned ptr = ASTEROID_S_CONFIG + (i * size_std);
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
    }
}

// ---------------------------------------------------------
// SPAWNING
// ---------------------------------------------------------
// Internal helper to setup a specific asteroid
static void activate_asteroid(asteroid_t *a, AsteroidType type, int level) {
    a->active = true;
    a->type = type;
    a->rx = 0; 
    a->ry = 0;
    a->anim_frame = random(0, MAX_ROTATION); // Random start angle

    // 1. Calculate Effective Level (Cap at 20)
    int eff_lvl = (level > 20) ? 20 : level;

    // Spawn at Random World Edge (-512 to +512)
    // 50% chance X-Edge, 50% chance Y-Edge
    if (rand16() & 1) {
        a->x = (rand16() & 1) ? AWORLD_X1 : AWORLD_X2;
        a->y = (int16_t)random(0, AWORLD_Y) + AWORLD_Y1;
    } else {
        a->x = (int16_t)random(0, AWORLD_X) + AWORLD_X1;
        a->y = (rand16() & 1) ? AWORLD_Y1 : AWORLD_Y2;
    }

    // Velocity (Slower for Large, Faster for Small)
    // int speed_base = (type == AST_LARGE) ? 64 : ((type == AST_MEDIUM) ? 128 : 256);

    // 3. Scale Velocity (Base + Scaling)
    // Large:  Starts 24, max 50  (~0.2 px/frame)
    // Medium: Starts 45, max 90  (~0.35 px/frame)
    // Small:  Starts 68, max 140 (~0.55 px/frame)
    int speed_base;
    if (type == AST_LARGE)      speed_base = 64 * eff_lvl;
    else if (type == AST_MEDIUM) speed_base = 128 * eff_lvl;
    else                        speed_base = 256 * eff_lvl;

    // Safety: Ensure speed is at least something moving
    if (speed_base < 32) speed_base = 32;

    // Older gives diagonals only.
    // a->vx = (rand16() & 1) ? speed_base : -speed_base;
    // a->vy = (rand16() & 1) ? speed_base : -speed_base;

    // RANDOMIZE DIRECTION (The Fix)
    // ----------------------------------------------------
    // Instead of using speed_base for both, pick a random magnitude 
    // between 25% and 100% of speed_base for EACH axis independently.
    // This creates various angles (shallow, steep, etc).
    int16_t mag_x = random(speed_base / 4, speed_base);
    int16_t mag_y = random(speed_base / 4, speed_base);


    // INTELLIGENT DIRECTION
    // ----------------------------------------------------
    // If spawned on the Left Edge (-512), force velocity Positive (Right).
    // If spawned on the Right Edge (512), force velocity Negative (Left).
    // Otherwise, randomize.
    
    if (a->x <= AWORLD_X1) {
        a->vx = mag_x; // Move Right
    } else if (a->x >= AWORLD_X2) {
        a->vx = -mag_x; // Move Left
    } else {
        a->vx = (rand16() & 1) ? mag_x : -mag_x; // Random
    }

    if (a->y <= AWORLD_Y1) {
        a->vy = mag_y; // Move Down
    } else if (a->y >= AWORLD_Y2) {
        a->vy = -mag_y; // Move Up
    } else {
        a->vy = (rand16() & 1) ? mag_y : -mag_y; // Random
    }

    // Health
    // if (type == AST_LARGE) a->health = 20;
    // else if (type == AST_MEDIUM) a->health = 10;
    // else a->health = 2;

    // 4. Scale Health
    // Large:  Starts 22, max 40
    // Medium: Starts 7,  max 16
    // Small:  Starts 2,  max 4
    if (type == AST_LARGE)       a->health = 20 * eff_lvl;
    else if (type == AST_MEDIUM) a->health = 6 * eff_lvl;
    else                         a->health = 2 * eff_lvl;

}

static int spawn_timer = 0;

void spawn_asteroid_wave(int level) {
    if (spawn_timer > 0) {
        spawn_timer--;
        return;
    }

    if (rand16() % 100 < 2) {
        for (int i = 0; i < MAX_AST_L; i++) {
            if (!ast_l[i].active) {
                // Pass the level to scaling logic
                activate_asteroid(&ast_l[i], AST_LARGE, level);
                active_ast_l_count++;
                
                printf("Spawned Large Asteroid %d (Lvl %d)\n", i, level);
                spawn_timer = 120; // 2 second cooldown
                break; 
            }
        }
    }
}

// ---------------------------------------------------------
// UPDATE & RENDER
// ---------------------------------------------------------
static void update_single(asteroid_t *a, int index, unsigned base_cfg, int size_bytes) {
    // 1. Movement (Fixed Point)
    // a->rx += a->vx; if (a->rx >= 256) { a->x++; a->rx -= 256; } else if (a->rx <= -256) { a->x--; a->rx += 256; }
    // a->ry += a->vy; if (a->ry >= 256) { a->y++; a->ry -= 256; } else if (a->ry <= -256) { a->y--; a->ry += 256; }

    // 1. MOVEMENT (Fixed Point - High Speed Capable)
    
    // X Axis
    a->rx += a->vx;
    int16_t whole_x = a->rx / 256; // Integer Division (e.g., 600 / 256 = 2)
    if (whole_x != 0) {
        a->x += whole_x;
        a->rx %= 256; // Keep only the remainder (e.g., 600 % 256 = 88)
    }

    // Y Axis
    a->ry += a->vy;
    int16_t whole_y = a->ry / 256;
    if (whole_y != 0) {
        a->y += whole_y;
        a->ry %= 256;
    }

    // 2. World Wrap (AWORLD_X1 to AWORLD_X2) & (AWORLD_Y1 to AWORLD_Y2)
    if (a->x < AWORLD_X1) a->x += AWORLD_X; else if (a->x > AWORLD_X2) a->x -= AWORLD_X;
    if (a->y < AWORLD_Y1) a->y += AWORLD_Y; else if (a->y > AWORLD_Y2) a->y -= AWORLD_Y;

    // Save world position before scrolling (needed for spawning children)
    a->world_x = a->x;
    a->world_y = a->y;

    a->x -= scroll_dx;
    a->y -= scroll_dy;

    // 3. Render
    int sx = a->x;
    int sy = a->y;
    unsigned ptr = base_cfg + (index * size_bytes);

    if (a->type == AST_LARGE) {
        // --- LARGE (Affine Plane 1) ---
        // Rotate every 8th frame
        // printf("Game Frame: %d\n", game_frame);
        if (game_frame % 8 == 0) {
            // Alternate direction based on index (i)
            if (index & 1) {
                a->anim_frame++; // Spin Clockwise
                if (a->anim_frame >= MAX_ROTATION) a->anim_frame = 0;
            } else {
                a->anim_frame--; // Spin Counter-Clockwise
                if (a->anim_frame >= 250) a->anim_frame = MAX_ROTATION - 1; // Handle wrap
            }
            // a->anim_frame--; // Spin Counter-Clockwise
            // if (a->anim_frame >= 250) a->anim_frame = MAX_ROTATION - 1; // Handle wrap
        }
        int r = a->anim_frame; 

        // Update Matrix (Rotation)
        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[0],  cos_fix[r]); // SX
        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[1], -sin_fix[r]); // SHY
        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[3],  sin_fix[r]); // SHX
        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[4],  cos_fix[r]); // SY

        int16_t tx = t2_fix32[r]; 
        
        // TY uses the inverse angle (24 - r)
        // Check bounds just in case r > 24
        int y_idx = (MAX_ROTATION - r);
        if (y_idx < 0) y_idx += MAX_ROTATION; // Safety wrap
        int16_t ty = t2_fix32[y_idx];

        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[2], tx); // TX
        xram0_struct_set(ptr, vga_mode4_asprite_t, transform[5], ty); // TY

        xram0_struct_set(ptr, vga_mode4_asprite_t, x_pos_px, sx);
        xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, sy);
    } 
    else {
        // --- MED/SMALL (Standard Plane 2) ---
        // Just position (no rotation logic yet)
        xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, sx);
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, sy);
        
        // Ensure data ptr is set (simple safeguard)
        uint16_t data = (a->type == AST_MEDIUM) ? ASTEROID_M_DATA : ASTEROID_S_DATA;
        uint8_t lsize = (a->type == AST_MEDIUM) ? 4 : 3;
        
        xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, data);
        xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, lsize);
        xram0_struct_set(ptr, vga_mode4_sprite_t, has_opacity_metadata, false);
    }
}

void move_asteroids_offscreen(void) {
    // Loop through pools
    for(int i=0; i<MAX_AST_L; i++) {
        // update_single(&ast_l[i], i, ASTEROID_L_CONFIG, sizeof(vga_mode4_asprite_t));
        unsigned ptr = ASTEROID_L_CONFIG + (i * sizeof(vga_mode4_asprite_t));
        xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, -100);
    }
    for(int i=0; i<MAX_AST_M; i++) {
        unsigned ptr = ASTEROID_M_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
    }
    for(int i=0; i<MAX_AST_S; i++) {
        unsigned ptr = ASTEROID_S_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
    }
}

void update_asteroids(void) {
    // Loop through pools
    for(int i=0; i<MAX_AST_L; i++) {
        if (ast_l[i].active) update_single(&ast_l[i], i, ASTEROID_L_CONFIG, sizeof(vga_mode4_asprite_t));
    }
    for(int i=0; i<MAX_AST_M; i++) {
        if (ast_m[i].active) update_single(&ast_m[i], i, ASTEROID_M_CONFIG, sizeof(vga_mode4_sprite_t));
    }
    for(int i=0; i<MAX_AST_S; i++) {
        if (ast_s[i].active) update_single(&ast_s[i], i, ASTEROID_S_CONFIG, sizeof(vga_mode4_sprite_t));
    }
}

// ---------------------------------------------------------
// SPLITTING LOGIC
// ---------------------------------------------------------

// Helper to spawn a child asteroid at a specific spot with specific velocity
// If aim_at_player is true, velocity will be calculated to head toward player
static void spawn_child(AsteroidType type, int16_t x, int16_t y, int16_t vx, int16_t vy, bool aim_at_player) {
    asteroid_t *pool;
    int max_count;
    
    // Select Pool
    if (type == AST_MEDIUM) { pool = ast_m; max_count = MAX_AST_M; }
    else { pool = ast_s; max_count = MAX_AST_S; }

    // Find free slot
    for (int i = 0; i < max_count; i++) {
        if (!pool[i].active) {
            pool[i].active = true;
            pool[i].type = type;
            pool[i].x = x;
            pool[i].y = y;
            pool[i].rx = 0; 
            pool[i].ry = 0;
            
            printf("spawn_child: type=%d, x=%d, y=%d, vx=%d, vy=%d\n", type, x, y, vx, vy);
            
            // Calculate velocity - aim at player if requested
            if (aim_at_player) {
                // Calculate direction to player
                int16_t dx = player_x - x;
                int16_t dy = player_y - y;
                
                // Calculate base speed for this asteroid type
                int16_t base_speed = (type == AST_MEDIUM) ? 180 : 280;
                
                // Normalize direction and scale to base_speed
                // Use simple ratio to avoid sqrt
                int16_t abs_dx = (dx < 0) ? -dx : dx;
                int16_t abs_dy = (dy < 0) ? -dy : dy;
                int16_t max_dist = (abs_dx > abs_dy) ? abs_dx : abs_dy;
                
                if (max_dist > 0) {
                    pool[i].vx = (dx * base_speed) / max_dist;
                    pool[i].vy = (dy * base_speed) / max_dist;
                } else {
                    // Fallback if player is at same position
                    pool[i].vx = vx;
                    pool[i].vy = vy;
                }
            } else {
                pool[i].vx = vx;
                pool[i].vy = vy;
            }
            pool[i].anim_frame = 0;
            
            // Set Health
            pool[i].health = (type == AST_MEDIUM) ? 6 : 1;
            
            // Update active counters
            if (type == AST_MEDIUM) {
                active_ast_m_count++;
            } else {
                active_ast_s_count++;
            }
            
            // Set Config Immediately (So it doesn't wait for next update frame)
            // unsigned ptr;
            // if (type == AST_MEDIUM) {
            //     ptr = ASTEROID_M_CONFIG + (i * sizeof(vga_mode4_sprite_t));
            //     xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, ASTEROID_M_DATA);
            //     xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, 4); // 16x16
            // } else {
            //     ptr = ASTEROID_S_CONFIG + (i * sizeof(vga_mode4_sprite_t));
            //     xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, ASTEROID_S_DATA);
            //     xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, 3); // 8x8
            // }
            // xram0_struct_set(ptr, vga_mode4_sprite_t, has_opacity_metadata, false);

            // printf("Spawning Child Type %d at %d,%d (Slot %d)\n", type, x, y, i);
            
            return; // Spawned successfully
        }
    }
    
    // No free slot available - pool is full
    printf("WARNING: Failed to spawn asteroid type %d - pool full!\n", type);
}

// ---------------------------------------------------------
// COLLISION LOGIC
// ---------------------------------------------------------

bool check_asteroid_hit(int16_t bx, int16_t by) {
    // Early exit if no asteroids active
    if (active_ast_l_count == 0 && active_ast_m_count == 0 && active_ast_s_count == 0) {
        return false;
    }

    // 1. Check LARGE Asteroids (Radius ~14px)
    if (active_ast_l_count > 0) {
        for (int i = 0; i < MAX_AST_L; i++) {
            if (!ast_l[i].active) continue;
            
            int16_t dx = ast_l[i].x + 16 - bx;
            int16_t dy = ast_l[i].y + 16 - by;
            
            // Broad-phase then narrow-phase using inline helpers
            if (!broad_phase_check(dx, dy, 20)) continue;
            if (!box_collision(dx, dy, 14)) continue;

            ast_l[i].health--;
            if (ast_l[i].health <= 0) {
                // DESTROY LARGE -> Spawn 2 Mediums
                ast_l[i].active = false;
                active_ast_l_count--;
                start_explosion(ast_l[i].x, ast_l[i].y);
                player_score += 15;
                game_score += 15 * game_level;

                // Split velocities (one diverges, one aims at player)
                spawn_child(AST_MEDIUM, ast_l[i].world_x, ast_l[i].world_y, ast_l[i].vx + 128, ast_l[i].vy - 128, false);
                spawn_child(AST_MEDIUM, ast_l[i].world_x, ast_l[i].world_y, ast_l[i].vx - 128, ast_l[i].vy + 128, true);
                
                // Hide sprite immediately
                unsigned ptr = ASTEROID_L_CONFIG + (i * sizeof(vga_mode4_asprite_t));
                xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, -100);
            }
            return true;
        }
    }

    // 2. Check MEDIUM Asteroids (Radius ~7px)
    if (active_ast_m_count > 0) {
        for (int i = 0; i < MAX_AST_M; i++) {
            if (!ast_m[i].active) continue;

            int16_t dx = ast_m[i].x + 8 - bx;
            int16_t dy = ast_m[i].y + 8 - by;
            
            if (!broad_phase_check(dx, dy, 12)) continue;
            if (!box_collision(dx, dy, 8)) continue;

            ast_m[i].health--;
            if (ast_m[i].health <= 0) {
                // DESTROY MEDIUM -> Spawn 2 Smalls
                ast_m[i].active = false;
                active_ast_m_count--;
                start_explosion(ast_m[i].x, ast_m[i].y);
                player_score += 7;
                game_score += 7 * game_level;

                // Make small ones fast! (one aims at player)
                spawn_child(AST_SMALL, ast_m[i].world_x, ast_m[i].world_y, ast_m[i].vx + 128, ast_m[i].vy + 128, false);
                spawn_child(AST_SMALL, ast_m[i].world_x, ast_m[i].world_y, ast_m[i].vx - 128, ast_m[i].vy - 128, true);

                // Hide sprite
                unsigned ptr = ASTEROID_M_CONFIG + (i * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
            }
            return true;
        }
    }

    // 3. Check SMALL Asteroids (Radius ~4px)
    if (active_ast_s_count > 0) {
        for (int i = 0; i < MAX_AST_S; i++) {
            if (!ast_s[i].active) continue;

            int16_t dx = ast_s[i].x + 4 - bx;
            int16_t dy = ast_s[i].y + 4 - by;
            
            if (!broad_phase_check(dx, dy, 8)) continue;
            if (!box_collision(dx, dy, 4)) continue;

            ast_s[i].health--; // Usually 1 hit kill
            if (ast_s[i].health <= 0) {
                // DESTROY SMALL -> Dust
                ast_s[i].active = false;
                active_ast_s_count--;
                start_explosion(ast_s[i].x, ast_s[i].y);
                player_score += 2;
                game_score += 2 * game_level;

                // Hide sprite
                unsigned ptr = ASTEROID_S_CONFIG + (i * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
            }
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------
// FIGHTER COLLISION LOGIC
// ---------------------------------------------------------

// Returns true if the fighter at (fx, fy) crashed into a rock
bool check_asteroid_hit_fighter(int16_t fx, int16_t fy) {
    // Early exit if no asteroids active
    if (active_ast_l_count == 0 && active_ast_m_count == 0 && active_ast_s_count == 0) {
        return false;
    }
    
    // 1. Calculate Fighter Center
    // Fighter is 4x4, center is +2
    int16_t f_cx = fx + 2;
    int16_t f_cy = fy + 2;

    // -------------------------------------------------
    // 1. Check LARGE Asteroids
    // -------------------------------------------------
    // Collision Radius: Rock(14) + Fighter(2) = 16
    if (active_ast_l_count > 0) {
        for (int i = 0; i < MAX_AST_L; i++) {
            if (!ast_l[i].active) continue;
            
            int16_t a_cx = ast_l[i].x + 16 - f_cx;
            int16_t a_cy = ast_l[i].y + 16 - f_cy;
            
            if (!broad_phase_check(a_cx, a_cy, 20)) continue;
            if (!box_collision(a_cx, a_cy, 16)) continue;

            ast_l[i].health -= 1;
                
                if (ast_l[i].health <= 0) {
                    // Destroy
                    ast_l[i].active = false;
                    active_ast_l_count--;
                    start_explosion(a_cx - 16, a_cy - 16);
                    
                    // Hide sprite
                    unsigned ptr = ASTEROID_L_CONFIG + (i * sizeof(vga_mode4_asprite_t));
                    xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, -100);

                    // Spawn Debris (one aims at player)
                    int16_t spread = 50;
                    spawn_child(AST_MEDIUM, ast_l[i].world_x, ast_l[i].world_y, ast_l[i].vx + spread, ast_l[i].vy - spread, false);
                    spawn_child(AST_MEDIUM, ast_l[i].world_x, ast_l[i].world_y, ast_l[i].vx - spread, ast_l[i].vy + spread, true);
                }
                return true;
        }
    }

    // -------------------------------------------------
    // 2. Check MEDIUM Asteroids
    // -------------------------------------------------
    // Collision Radius: Rock(7) + Fighter(2) = 9
    if (active_ast_m_count > 0) {
        for (int i = 0; i < MAX_AST_M; i++) {
            if (!ast_m[i].active) continue;
            
            int16_t a_cx = ast_m[i].x + 8 - f_cx;
            int16_t a_cy = ast_m[i].y + 8 - f_cy;

            if (!broad_phase_check(a_cx, a_cy, 12)) continue;
            if (!box_collision(a_cx, a_cy, 9)) continue;

            ast_m[i].health -= 1;
                
            if (ast_m[i].health <= 0) {
                ast_m[i].active = false;
                active_ast_m_count--;
                start_explosion(ast_m[i].x, ast_m[i].y);

                // Hide sprite
                unsigned ptr = ASTEROID_M_CONFIG + (i * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);

                int16_t spread = 80;
                spawn_child(AST_SMALL, ast_m[i].world_x, ast_m[i].world_y, ast_m[i].vx + spread, ast_m[i].vy - spread, false);
                spawn_child(AST_SMALL, ast_m[i].world_x, ast_m[i].world_y, ast_m[i].vx - spread, ast_m[i].vy + spread, true);
            }
            return true;
        }
    }

    // -------------------------------------------------
    // 3. Check SMALL Asteroids
    // -------------------------------------------------
    // Collision Radius: Rock(3) + Fighter(2) = 5
    if (active_ast_s_count > 0) {
        for (int i = 0; i < MAX_AST_S; i++) {
            if (!ast_s[i].active) continue;
            
            int16_t a_cx = ast_s[i].x + 4 - f_cx;
            int16_t a_cy = ast_s[i].y + 4 - f_cy;

            if (!broad_phase_check(a_cx, a_cy, 8)) continue;
            if (!box_collision(a_cx, a_cy, 4)) continue;

            ast_s[i].active = false;
                active_ast_s_count--;
                start_explosion(ast_s[i].x, ast_s[i].y);

            // Hide sprite
            unsigned ptr = ASTEROID_S_CONFIG + (i * sizeof(vga_mode4_sprite_t));
            xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);

            return true;
        }
    }

    return false;
}void check_player_asteroid_collision(int16_t px, int16_t py) {
    // 1. Calculate Player Center (8x8 sprite)
    int16_t p_cx = px + 4;
    int16_t p_cy = py + 4;

    // -------------------------------------------------
    // 1. LARGE ASTEROIDS (Radius ~14)
    // -------------------------------------------------
    // Hitbox: 14 (Rock) + 3 (Player) = 17
    for (int i = 0; i < MAX_AST_L; i++) {
        if (!ast_l[i].active) continue;
        
        // Large uses centered coordinates due to Affine offset
        int16_t a_cx = ast_l[i].x + 16;
        int16_t a_cy = ast_l[i].y + 16;

        if (abs(a_cx - p_cx) < 17 && abs(a_cy - p_cy) < 17) {
            // CRASH INTO LARGE -> INSTANT GAME OVER
            // enemy_score = 100; 
            // start_explosion(px, py);
            // printf("GAME OVER: Player hit Large Asteroid\n");
            // return; // No need to check others

            // --- NEW DEATH LOGIC ---
            printf("CRASH! Triggering Death Sequence...\n");
            
            // 1. Start the first big explosion exactly at player position
            start_explosion(px, py);
            
            // 2. Begin the 3-second drama
            trigger_player_death();

            // Use Index 32 (Red in Rainbow Palette) or 0x03 (Standard Red)
            uint8_t text_color = 32; 
            
            // Centering math (approximate)
            // Screen 320 wide. Text ~60px wide.
            draw_text(110, 40, "YOU CRASHED...", text_color);
            draw_text(125, 52, "GAME OVER", text_color);
            
            return;
        }
    }

    // -------------------------------------------------
    // 2. MEDIUM ASTEROIDS (Radius ~7)
    // -------------------------------------------------
    // Hitbox: 7 (Rock) + 3 (Player) = 10
    for (int i = 0; i < MAX_AST_M; i++) {
        if (!ast_m[i].active) continue;
        
        int16_t a_cx = ast_m[i].x + 8;
        int16_t a_cy = ast_m[i].y + 8;

        if (abs(a_cx - p_cx) < 10 && abs(a_cy - p_cy) < 10) {
            // PENALTY: -20 Points
            if (player_score >= 20) player_score -= 20; 
            else player_score = 0;

            // Destroy Rock
            ast_m[i].active = false;
            start_explosion(ast_m[i].x, ast_m[i].y);
            
            // Hide Sprite
            unsigned ptr = ASTEROID_M_CONFIG + (i * sizeof(vga_mode4_sprite_t));
            xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);

                        // Split into Smalls (one aims at player)
            int16_t spread = 80;
            spawn_child(AST_SMALL, ast_m[i].world_x, ast_m[i].world_y, ast_m[i].vx + spread, ast_m[i].vy - spread, false);
            spawn_child(AST_SMALL, ast_m[i].world_x, ast_m[i].world_y, ast_m[i].vx - spread, ast_m[i].vy + spread, true);
            
            // Visual feedback
            start_explosion(px, py);
            return; // Prevent multi-hit in one frame
        }
    }

    // -------------------------------------------------
    // 3. SMALL ASTEROIDS (Radius ~3)
    // -------------------------------------------------
    // Hitbox: 3 (Rock) + 3 (Player) = 6
    for (int i = 0; i < MAX_AST_S; i++) {
        if (!ast_s[i].active) continue;
        
        int16_t a_cx = ast_s[i].x + 4;
        int16_t a_cy = ast_s[i].y + 4;

        if (abs(a_cx - p_cx) < 6 && abs(a_cy - p_cy) < 6) {
            // PENALTY: -10 Points
            if (player_score >= 10) player_score -= 10;
            else player_score = 0;

            // Destroy Rock
            ast_s[i].active = false;
            start_explosion(ast_s[i].x, ast_s[i].y);

            unsigned ptr = ASTEROID_S_CONFIG + (i * sizeof(vga_mode4_sprite_t));
            xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
            
            start_explosion(px, py);
            return;
        }
    }
}

// Same logic as standard hit, but 0 points awarded
bool check_asteroid_hit_no_score(int16_t bx, int16_t by) {
    
    // ---------------------------------------------------------
    // 1. Check LARGE Asteroids
    // ---------------------------------------------------------
    for (int i = 0; i < MAX_AST_L; i++) {
        if (!ast_l[i].active) continue;
        
        // Large uses Centered Coords (Affine offset)
        int16_t a_cx = ast_l[i].x + 16;
        int16_t a_cy = ast_l[i].y + 16;
        
        // Radius 14 + Bullet 2 = 16
        if (abs(a_cx - bx) < 16 && abs(a_cy - by) < 16) {
            ast_l[i].health--; // 1 Damage
            
            if (ast_l[i].health <= 0) {
                // Destroy & Split
                ast_l[i].active = false;
                start_explosion(ast_l[i].x, ast_l[i].y);
                // NO POINTS AWARDED

                unsigned ptr = ASTEROID_L_CONFIG + (i * sizeof(vga_mode4_asprite_t));
                xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, -100);

                int16_t spread = 50;
                spawn_child(AST_MEDIUM, ast_l[i].world_x, ast_l[i].world_y, ast_l[i].vx + spread, ast_l[i].vy - spread, false);
                spawn_child(AST_MEDIUM, ast_l[i].world_x, ast_l[i].world_y, ast_l[i].vx - spread, ast_l[i].vy + spread, true);
            }
            return true;
        }
    }

    // ---------------------------------------------------------
    // 2. Check MEDIUM Asteroids
    // ---------------------------------------------------------
    for (int i = 0; i < MAX_AST_M; i++) {
        if (!ast_m[i].active) continue;
        
        // Medium uses Top-Left Coords
        int16_t a_cx = ast_m[i].x + 8;
        int16_t a_cy = ast_m[i].y + 8;

        // Radius 8 + Bullet 2 = 10
        if (abs(a_cx - bx) < 10 && abs(a_cy - by) < 10) {
            ast_m[i].health--;
            
            if (ast_m[i].health <= 0) {
                ast_m[i].active = false;
                start_explosion(ast_m[i].x, ast_m[i].y);
                
                unsigned ptr = ASTEROID_M_CONFIG + (i * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);

                int16_t spread = 80;
                spawn_child(AST_SMALL, ast_m[i].world_x, ast_m[i].world_y, ast_m[i].vx + spread, ast_m[i].vy - spread, false);
                spawn_child(AST_SMALL, ast_m[i].world_x, ast_m[i].world_y, ast_m[i].vx - spread, ast_m[i].vy + spread, true);
            }
            return true;
        }
    }

    // ---------------------------------------------------------
    // 3. Check SMALL Asteroids
    // ---------------------------------------------------------
    for (int i = 0; i < MAX_AST_S; i++) {
        if (!ast_s[i].active) continue;
        
        // Small uses Top-Left Coords
        int16_t a_cx = ast_s[i].x + 4;
        int16_t a_cy = ast_s[i].y + 4;

        // Radius 4 + Bullet 2 = 6
        if (abs(a_cx - bx) < 6 && abs(a_cy - by) < 6) {
            ast_s[i].health--;
            
            if (ast_s[i].health <= 0) {
                ast_s[i].active = false;
                start_explosion(ast_s[i].x, ast_s[i].y);

                unsigned ptr = ASTEROID_S_CONFIG + (i * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
            }
            return true;
        }
    }

    return false;
}