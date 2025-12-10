#ifndef BULLETS_H
#define BULLETS_H

#include <stdint.h>
#include "constants.h"

/**
 * bullets.h - Player bullet management system
 * 
 * Handles player bullet firing, movement, and collision detection
 */

// Bullet structure
typedef struct {
    int16_t x, y;           // Position
    int16_t status;         // -1 = inactive, 0-23 = active with direction
    int16_t vx_rem, vy_rem; // Velocity remainder for sub-pixel movement
} Bullet;

/**
 * Initialize player bullet system
 */
void init_bullets(void);

/**
 * Update all active player bullets
 * - Move bullets based on direction
 * - Check collisions with enemies
 * - Remove off-screen bullets
 */
void update_bullets(void);

// Exported for use by player.c
extern Bullet bullets[MAX_BULLETS];
extern uint8_t current_bullet_index;
extern int16_t active_bullet_count;

#endif // BULLETS_H
