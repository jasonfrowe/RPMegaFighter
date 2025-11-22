/*
 * Gamepad Button Test for RP6502
 * Displays all gamepad button and stick states in real-time
 */

#include <rp6502.h>
#include <stdio.h>
#include <stdbool.h>
#include "definitions.h"
#include "usb_hid_keys.h"

// Gamepad input structure
static gamepad_t gamepad[GAMEPAD_COUNT];

int main(void)
{
    printf("\n=== RP6502 Gamepad Button Test ===\n\n");
    printf("Press buttons to see which ones are detected\n");
    printf("Press ESC to exit\n\n");
    
    // Enable keyboard for ESC
    xregn(0, 0, 0, 1, KEYBOARD_INPUT);
    
    // Enable gamepad input
    xregn(0, 0, 2, 1, GAMEPAD_INPUT);
    
    uint8_t vsync_last = RIA.vsync;
    uint8_t keystates[6];
    
    // Previous button states for edge detection
    uint8_t prev_dpad = 0;
    uint8_t prev_sticks = 0;
    uint8_t prev_btn0 = 0;
    uint8_t prev_btn1 = 0;
    bool connected_shown = false;
    
    while (true) {
        // Wait for vsync
        if (RIA.vsync == vsync_last)
            continue;
        vsync_last = RIA.vsync;
        
        // Check for ESC
        RIA.addr0 = KEYBOARD_INPUT;
        RIA.step0 = 2;
        keystates[0] = RIA.rw0;
        RIA.step0 = 1;
        keystates[2] = RIA.rw0;
        
        if (keystates[0] == KEY_ESC) {
            printf("\nExiting...\n");
            break;
        }
        
        // Read gamepad data
        RIA.addr0 = GAMEPAD_INPUT;
        RIA.step0 = 1;
        for (uint8_t i = 0; i < GAMEPAD_COUNT; i++) {
            gamepad[i].dpad = RIA.rw0;
            gamepad[i].sticks = RIA.rw0;
            gamepad[i].btn0 = RIA.rw0;
            gamepad[i].btn1 = RIA.rw0;
            gamepad[i].lx = RIA.rw0;
            gamepad[i].ly = RIA.rw0;
            gamepad[i].rx = RIA.rw0;
            gamepad[i].ry = RIA.rw0;
            gamepad[i].l2 = RIA.rw0;
            gamepad[i].r2 = RIA.rw0;
        }
        
        if (gamepad[0].dpad & GP_CONNECTED) {
            if (!connected_shown) {
                printf("Gamepad connected! Press buttons to test...\n\n");
                connected_shown = true;
            }
            
            // D-Pad - only print on change
            if (gamepad[0].dpad != prev_dpad) {
                if (gamepad[0].dpad & GP_DPAD_UP) printf("D-PAD UP pressed\n");
                if (gamepad[0].dpad & GP_DPAD_DOWN) printf("D-PAD DOWN pressed\n");
                if (gamepad[0].dpad & GP_DPAD_LEFT) printf("D-PAD LEFT pressed\n");
                if (gamepad[0].dpad & GP_DPAD_RIGHT) printf("D-PAD RIGHT pressed\n");
                prev_dpad = gamepad[0].dpad;
            }
            
            // Sticks - only print on change
            if (gamepad[0].sticks != prev_sticks) {
                if (gamepad[0].sticks & GP_LSTICK_UP) printf("LEFT STICK UP\n");
                if (gamepad[0].sticks & GP_LSTICK_DOWN) printf("LEFT STICK DOWN\n");
                if (gamepad[0].sticks & GP_LSTICK_LEFT) printf("LEFT STICK LEFT\n");
                if (gamepad[0].sticks & GP_LSTICK_RIGHT) printf("LEFT STICK RIGHT\n");
                if (gamepad[0].sticks & GP_RSTICK_UP) printf("RIGHT STICK UP\n");
                if (gamepad[0].sticks & GP_RSTICK_DOWN) printf("RIGHT STICK DOWN\n");
                if (gamepad[0].sticks & GP_RSTICK_LEFT) printf("RIGHT STICK LEFT\n");
                if (gamepad[0].sticks & GP_RSTICK_RIGHT) printf("RIGHT STICK RIGHT\n");
                prev_sticks = gamepad[0].sticks;
            }
            
            // Button Group 0 - only print on change
            if (gamepad[0].btn0 != prev_btn0) {
                printf("BTN0 changed: 0x%02X -> ", gamepad[0].btn0);
                if (gamepad[0].btn0 & GP_BTN_A) printf("A ");
                if (gamepad[0].btn0 & GP_BTN_B) printf("B ");
                if (gamepad[0].btn0 & GP_BTN_X) printf("X ");
                if (gamepad[0].btn0 & GP_BTN_Y) printf("Y ");
                if (gamepad[0].btn0 & GP_BTN_L1) printf("L1 ");
                if (gamepad[0].btn0 & GP_BTN_R1) printf("R1 ");
                if (gamepad[0].btn0 & GP_BTN_L2) printf("L2 ");
                if (gamepad[0].btn0 & GP_BTN_R2) printf("R2 ");
                printf("\n");
                
                // Show Sega mapping
                printf("  Sega: ");
                if (gamepad[0].btn0 & 0x01) printf("A ");
                if (gamepad[0].btn0 & 0x02) printf("B ");
                if (gamepad[0].btn0 & 0x04) printf("C ");
                printf("\n");
                
                prev_btn0 = gamepad[0].btn0;
            }
            
            // Button Group 1 - only print on change
            if (gamepad[0].btn1 != prev_btn1) {
                printf("BTN1 changed: 0x%02X -> ", gamepad[0].btn1);
                if (gamepad[0].btn1 & GP_BTN_SELECT) printf("SELECT ");
                if (gamepad[0].btn1 & GP_BTN_START) printf("START ");
                if (gamepad[0].btn1 & GP_BTN_HOME) printf("HOME ");
                printf("\n");
                prev_btn1 = gamepad[0].btn1;
            }
            
        } else {
            if (connected_shown) {
                printf("Gamepad disconnected\n");
                connected_shown = false;
            }
        }
    }
    
    return 0;
}
