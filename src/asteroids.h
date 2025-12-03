#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#include <stdint.h>
#include <stdbool.h>

// Simplify: Just one type for now (Large)
typedef struct {
    bool active;
    int16_t x, y;       // World position
    int16_t rx, ry;     // Sub-pixel remainders (for smooth movement)
    int16_t vx, vy;     // Velocity
    uint8_t anim_frame;
} asteroid_t;

// We defined space for 2 Large Asteroids in the memory map
#define MAX_AST_L 2

// Global Array
extern asteroid_t ast_l[MAX_AST_L];

// Basic Interface
void init_asteroids(void);
void spawn_asteroids(void); // Spawns the initial 2
void update_asteroids(void); // Move and Render

#endif