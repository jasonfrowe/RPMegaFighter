#ifndef SBULLETS_H
#define SBULLETS_H

#include <stdint.h>
#include <stdbool.h>


// Spread shot bullets
#define MAX_SBULLETS                3      // Spread shot bullets
#define SBULLET_COOLDOWN_MAX      120      // Frames between super bullet shots
#define SBULLET_COOLDOWN_MIN       40      // Minimum cooldown for super bullets
#define SBULLET_COOLDOWN_DECREASE  10      // Decrease per power-up
#define SBULLET_SPEED_SHIFT         6      // Divide by 64 for bullet speed (~4 pixels/frame)
#define SBULLET_LIFETIME_FRAMES    20      // Lifetime of a super bullet in frames

/**
 * sbullets.h - Player super bullet (spread shot) management system
 * 
 * Handles super bullet firing (3-bullet spread), movement, and collision detection
 * Fires when button C is pressed   
 */

// Super bullet structure (same as regular bullets)
typedef struct {
    int16_t x, y;           // Position
    int16_t status;         // -1 = inactive, 0-23 = active with direction
    int16_t vx_rem, vy_rem; // Velocity remainder for sub-pixel movement
} SBullet;

/**
 * Move all super bullets offscreen (for game over)
 */
void move_sbullets_offscreen(void);

/**
 * Initialize super bullet system
 */
void init_sbullets(void);

/**
 * Fire a spread of 3 super bullets (left, center, right of player rotation)
 * Returns true if bullets were fired, false if on cooldown
 */
bool fire_sbullet(uint8_t player_rotation);

/**
 * Update all active super bullets
 * - Move bullets based on direction
 * - Check collisions with enemies
 * - Remove off-screen bullets
 */
void update_sbullets(void);

// Exposed cooldown value so other modules may read/set it
extern int16_t sbullet_cooldown;

#endif // SBULLETS_H
