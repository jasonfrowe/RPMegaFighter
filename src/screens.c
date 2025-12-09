#include "screens.h"
#include "constants.h"
#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "powerup.h"
#include "sbullets.h"
#include "input.h"
#include "asteroids.h"
#include "player.h"
#include "bkgstars.h"
#include "explosions.h"

// External references
extern void draw_text(int16_t x, int16_t y, const char* text, uint8_t color);
extern void clear_rect(int16_t x, int16_t y, int16_t width, int16_t height);
extern void move_fighters_offscreen(void);
extern void move_ebullets_offscreen(void);
extern void reset_player_position(void);
extern int8_t check_high_score(int16_t score);
extern void get_player_initials(char* initials);
extern void insert_high_score(int8_t position, const char* initials, int16_t score);
extern void save_high_scores(void);
extern void start_end_music(void);
extern void update_music(void);
extern void stop_music(void);
extern void move_asteroids_offscreen(void);

extern void draw_stars(int16_t scroll_dx, int16_t scroll_dy);

extern int16_t game_level;
extern int16_t game_score;
extern const uint16_t vlen;

// Bullet structure
typedef struct {
    int16_t x, y;
    int16_t vx, vy;
    int16_t status;
} Bullet;

extern Bullet bullets[];

// extern gamepad_t gamepad[GAMEPAD_COUNT];
extern uint8_t keystates[KEYBOARD_BYTES];

// Key macro
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))

// Sprite configuration addresses
// extern unsigned BULLET_CONFIG;

/**
 * Display level up message and wait for START button
 */
void show_level_up(void)
{
    const uint8_t blue_color = 0x1F;
    const uint8_t white_color = 0xFF;
    const uint16_t center_x = 120;
    const uint16_t center_y = 80;
        
    // Draw "LEVEL UP" message
    draw_text(center_x, center_y, "LEVEL UP", blue_color);
    // Changed text to match the Action, not a specific button
    draw_text(center_x - 45, center_y + 15, "PRESS FIRE TO CONTINUE", white_color);
    
    printf("\n*** LEVEL UP! Now on level %d ***\n", game_level);
    
    uint8_t vsync_last = RIA.vsync;
    
    // ---------------------------------------------------------
    // PHASE 1: Wait for FIRE to be RELEASED
    // (Prevents accidental skipping if button was held down)
    // ---------------------------------------------------------
    while (true) {
        if (RIA.vsync == vsync_last)
            continue;
        vsync_last = RIA.vsync;
        
        // Update input state
        handle_input();

        // Break only when FIRE is NOT pressed
        if (!is_action_pressed(0, ACTION_FIRE)) {
            break;
        }
    }
    
    // ---------------------------------------------------------
    // PHASE 2: Wait for FIRE to be PRESSED
    // (The actual user interaction)
    // ---------------------------------------------------------
    while (true) {
        if (RIA.vsync == vsync_last)
            continue;
        vsync_last = RIA.vsync;
        
        handle_input();
        
        // Break the moment FIRE is pressed
        if (is_action_pressed(0, ACTION_FIRE)) {
            break; 
        }
    }

    // ---------------------------------------------------------
    // PHASE 3: Wait for FIRE to be RELEASED Again
    // (Prevents the ship from firing immediately when game resumes)
    // ---------------------------------------------------------
    while (true) {
        if (RIA.vsync == vsync_last)
            continue;
        vsync_last = RIA.vsync;
        
        handle_input();
        
        if (!is_action_pressed(0, ACTION_FIRE)) {
            break; 
        }
    }
    
    // Clear the message area
    clear_rect(center_x - 45, center_y, 150, 25);
}

/**
 * Display game over screen and wait for fire button
 */
void show_game_over(void)
{
    const uint16_t center_x = 130;  // Better centered for text
    uint16_t color_timer = 0;  // For rainbow cycling

    // Clear crash text around player position (40x40 box centered on player)
    // Player sprite is 8x8, so center is at player_x+4, player_y+4
    // int16_t clear_x = player_x + 4 - 20;  // Center - half width
    // int16_t clear_y = player_y + 4 - 20;  // Center - half height
    // clear_rect(clear_x, clear_y, 60, 60);
    clear_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // Clear entire screen for simplicity
    draw_stars(scroll_dx, scroll_dy); // Redraw stars in background
    
    // Start end music
    start_end_music();
    
    // Move entities offscreen
    move_fighters_offscreen();
    move_sbullets_offscreen();
    move_ebullets_offscreen();
    move_asteroids_offscreen();
    
    // Move all bullets offscreen
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].status >= 0) {
            unsigned ptr = BULLET_CONFIG + i * sizeof(vga_mode4_sprite_t);
            xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, -100);
            xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -100);
            bullets[i].status = -1;
        }
    }

    // reset power-up state
    powerup.active = false;
    // Move power-up sprite offscreen
    xram0_struct_set(POWERUP_CONFIG, vga_mode4_sprite_t, x_pos_px, -100);
    xram0_struct_set(POWERUP_CONFIG, vga_mode4_sprite_t, y_pos_px, -100);
    
    // Reset player position to center
    reset_player_position();

    // Check if player got a high score
    int8_t high_score_pos = check_high_score(game_score);
    if (high_score_pos >= 0) {
        // Get player initials
        char initials[4];
        get_player_initials(initials);
        
        // Insert into high score table
        insert_high_score(high_score_pos, initials, game_score);
        
        // Save to file
        save_high_scores();
    }
    
    printf("\n*** GAME OVER ***\n");
    printf("Final Level: %d\n", game_level);
    printf("Final Score: %d\n", game_score);
    
    uint8_t vsync_last = RIA.vsync;
    bool fire_initially_released = false;
    
    unsigned timeout_frames = 30 * 60; // 30 seconds
    unsigned frame_count = 0;

    while (frame_count < timeout_frames) {
        if (RIA.vsync == vsync_last)
            continue;
        vsync_last = RIA.vsync;

        frame_count++;
        update_music();
        
        // Update explosions so they animate
        update_explosions();
        
        // Random explosions every few frames for visual effect
        if ((frame_count % 8) == 0) {  // Trigger every 8 frames
            int16_t exp_x = rand() % 160 + 160;
            int16_t exp_y = rand() % 90 + 90;
            start_explosion(exp_x, exp_y);
        }
        
        // Rainbow color cycling (similar to pause screen)
        color_timer++;
        uint8_t game_over_color = 32 + ((color_timer / 2) % 224);
        uint8_t continue_color = 32 + (((color_timer / 2) + 112) % 224);  // Offset for variety
        
        // Draw "GAME OVER" message with rainbow color
        draw_text(center_x + 7, 50, "GAME OVER", game_over_color);
        draw_text(center_x - 20, 70, "PRESS FIRE TO CONTINUE", continue_color);
        
        // Update inputs
        handle_input();
        
        bool fire_pressed = is_action_pressed(0, ACTION_FIRE);
        
        // Logic: Wait for the button to be released at least once...
        if (!fire_pressed) {
            fire_initially_released = true;
        } 
        // ...then wait for it to be pressed again.
        else if (fire_initially_released) {
            printf("Fire button pressed - continuing...\n");
            break; // Exit loop
        }
        
        // Check Global Exit
        if (key(KEY_ESC)) {
            printf("ESC pressed - exiting...\n");
            stop_music();
            exit(0); // Or break to return to title
        }
    }
    
    // 5. Cleanup and Return
    if (frame_count >= timeout_frames) {
        printf("Timeout reached - continuing...\n");
    }

    stop_music();
    
    // Fast Screen Clear (Wipe VRAM)
    RIA.addr0 = 0;
    RIA.step0 = 1;
    for (unsigned i = vlen; i--;) {
        RIA.rw0 = 0;
    }
}
