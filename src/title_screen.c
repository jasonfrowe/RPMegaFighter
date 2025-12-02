#include "title_screen.h"
#include "constants.h"
#include "sbullets.h"
#include "music.h"
#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "random.h"
#include "input.h"

// External references
extern void draw_text(uint16_t x, uint16_t y, const char *str, uint8_t colour);
extern void clear_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
extern void draw_high_scores(void);
extern const uint16_t vlen;
extern bool demo_mode_active;

extern uint8_t keystates[KEYBOARD_BYTES];
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))

void show_title_screen(void)
{
    const uint8_t red_color = 0x03;      // Pure red
    const uint8_t blue_color = 0x1F;     // Blue
    const uint16_t center_x = 90;       // X position for centered text
    
    // Clear any remaining bullets from previous game
    init_sbullets();
    
    // Start title music
    start_title_music();
    

    
    // Draw high scores on right side
    draw_high_scores();
    
    uint8_t vsync_last = RIA.vsync;
    // Demo idle detection (frames)
    const unsigned DEMO_IDLE_FRAMES = 60 * 60; // 60 seconds
    unsigned idle_frames = 0;
    uint16_t flash_counter = 0;
    bool press_start_visible = true;
    const uint16_t flash_interval = 30;  // 0.5 seconds at 60 Hz
    uint8_t current_color = red_color;
    
    // SAVE ORIGINAL COLOR (Index 11)
    // Palette starts at 0xF000. Index 11 is at 0xF000 + (11 * 2) = 0xF016
    RIA.addr0 = 0xF016;
    RIA.step0 = 1;
    uint8_t orig_color_low = RIA.rw0;
    uint8_t orig_color_high = RIA.rw0;

    uint16_t color_cycle_timer = 0; // Timer for color cycling
    
    printf("Title screen displayed. Press START to begin...\n");
    
    // Title screen loop - wait for START button
    bool start_button_was_pressed = false;  // Track button state for edge detection
    uint16_t highscore_counter = 0;
    while (true) {
        // Wait for vertical sync
        if (RIA.vsync == vsync_last)
            continue;
        vsync_last = RIA.vsync;

        // Increment seed counter for randomness
        seed_counter++;

        // Handle input
        handle_input();
        
        // --- CYCLE INDEX 11 ---
        // Cycle speed: Update every 4th frame
        color_cycle_timer++;
        if ((color_cycle_timer % 4) == 0) {
            // Pick a color from our Rainbow Range (Indices 32 to 255)
            // Total rainbow colors = 224
            uint8_t source_index = 32 + ((color_cycle_timer / 4) % 224);
            
            // Calculate address of the source color
            unsigned source_addr = 0xF000 + (source_index * 2);
            
            // Read the rainbow color
            RIA.addr0 = source_addr;
            RIA.step0 = 1;
            uint8_t r_low = RIA.rw0;
            uint8_t r_high = RIA.rw0;
            
            // Write it to Index 11
            RIA.addr0 = 0xF016;
            RIA.rw0 = r_low;
            RIA.rw0 = r_high;
        }

        // Update high score display periodically to rotate colours
        highscore_counter++;
        if (highscore_counter >= 15) {
            highscore_counter = 0;
            draw_high_scores();
        }
        
        // Update music
        update_music();
                
        // Check for keyboard ENTER or gamepad START button to start game
        bool start_pressed = false;
                
        // Check gamepad START button (BTN1 bit 0x08)
        if (is_action_pressed(0, ACTION_PAUSE)) {
            start_pressed = true;
        }
        
        // Handle start with edge detection
        if (start_pressed) {
            if (!start_button_was_pressed) {
                // This is a new press (edge detection)
                start_button_was_pressed = true;
                // Stop music
                stop_music();
                // Clear entire screen before exiting
                RIA.addr0 = 0;
                RIA.step0 = 1;
                for (unsigned i = vlen; i--;) {
                    RIA.rw0 = 0;
                }
                printf("START/ENTER pressed - beginning game!\n");
                
                // Wait for button/key to be released before exiting
                while (true) {
                    if (RIA.vsync == vsync_last)
                        continue;
                    vsync_last = RIA.vsync;

                    handle_input(); 
                                        
                    // Exit loop when both ENTER and START are released
                    if (!is_action_pressed(0, ACTION_PAUSE)) {
                        break;
                    }
                }

                // Initialize LFSR seed based on time spent on title screen
                lfsr = seed_counter;
                if (lfsr == 0) lfsr = 0xACE1; // Seed must never be 0
                printf("LFSR initialized with seed: 0x%04X\n", lfsr);

                // --- RESTORE COLOR BEFORE EXIT ---
                RIA.addr0 = 0xF016;
                RIA.step0 = 1;
                RIA.rw0 = orig_color_low;
                RIA.rw0 = orig_color_high;
             // ---------------------------------
                
                return;  // Exit title screen
            }
        } else {
            // Button/key is not pressed
            start_button_was_pressed = false;
        }

        // Demo countdown: always increment and start demo after timeout
        idle_frames++;
        if (idle_frames >= DEMO_IDLE_FRAMES) {

            // --- RESTORE COLOR BEFORE EXIT ---
            RIA.addr0 = 0xF016;
            RIA.step0 = 1;
            RIA.rw0 = orig_color_low;
            RIA.rw0 = orig_color_high;
            // ---------------------------------

            demo_mode_active = true; // Set demo mode flag

            // Clear entire screen before exiting
            RIA.addr0 = 0;
            RIA.step0 = 1;
            for (unsigned i = vlen; i--;) {
                RIA.rw0 = 0;
            }

            return;  // Exit title screen to start demo mode
        }
        
        // Check for ESC to exit game
        if (key(KEY_ESC)) {
            printf("ESC pressed - exiting...\n");
            exit(0);
        }
        
        flash_counter++;
        if (flash_counter >= flash_interval) {
            flash_counter = 0;
            press_start_visible = !press_start_visible;
            
            // if (!press_start_visible) {
            //     // Clear the text area immediately when turning off
            //     // "PRESS START" is 11 chars * 4px width = 44px
            //     clear_rect(center_x - 10, 100, 44, 5);
            // }
        }

        // 2. Render Text (Rainbow Cycle)
        if (press_start_visible) {
            // Calculate Rainbow Color
            // Range: 32 to 255 (224 colors)
            // Use seed_counter (which increments every frame) to drive the cycle
            uint8_t rainbow_color = 32 + (seed_counter % 224);
            
            // Draw text with the new color
            draw_text(center_x - 10, 100, "PRESS START", rainbow_color);
        }

    }
}
