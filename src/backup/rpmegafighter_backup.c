/*
 * RPMegaFighter - A port of Mega Super Fighter Challenge to the RP6502
 * Based on the Sega Genesis game by Jason Rowe
 * 
 * Platform: RP6502 Picocomputer
 * Graphics: VGA Mode 3 (320x180 bitmap) + Mode 4 (sprites)
 */

#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "usb_hid_keys.h"
#include "definitions.h"
#include "random.h"
#include "graphics.h"

// ============================================================================
// GAME CONSTANTS
// ============================================================================

// Screen dimensions
#define SCREEN_WIDTH        320
#define SCREEN_HEIGHT       180
#define SCREEN_WIDTH_D2     160
#define SCREEN_HEIGHT_D2    90

// World/map dimensions
#define MAP_SIZE            1024
#define MAP_SIZE_M1         1023
#define MAP_SIZE_D2         512
#define MAP_SIZE_NEG        -1024
#define MAP_SIZE_NEG_D2     -512

// Scroll boundaries
#define BOUNDARY_X          100
#define BOUNDARY_Y          80

// Player/Ship properties
#define SHIP_ROTATION_STEPS 24    // Number of rotation angles (matches sin/cos table)
#define SHIP_ROT_SPEED      3     // Frames between rotation updates

// Bullet properties
#define MAX_BULLETS         8
#define BULLET_COOLDOWN     8
#define MAX_EBULLETS        8     // Enemy bullets
#define EBULLET_COOLDOWN    8
#define MAX_SBULLETS        3     // Spread shot bullets
#define SBULLET_COOLDOWN    45

// Enemy fighter properties
#define MAX_FIGHTERS        30
#define FIGHTER_SPAWN_RATE  128   // Frames between fighter spawns

// Scoring
#define SCORE_TO_WIN        100
#define SCORE_BASIC_KILL    1
#define SCORE_MINE_KILL     5
#define SCORE_SHIELD_KILL   5
#define SCORE_MINE_HIT      -10

// ============================================================================
// GAME STRUCTURES
// ============================================================================

// Bullet structure
typedef struct {
    int16_t x, y;           // Position
    int16_t status;         // -1 = inactive, 0-23 = active with direction
    int16_t vx_rem, vy_rem; // Velocity remainder for sub-pixel movement
} Bullet;

// Fighter/Enemy structure  
typedef struct {
    int16_t x, y;           // Position
    int16_t vx, vy;         // Velocity
    int16_t vx_i, vy_i;     // Initial velocity (for randomization)
    int16_t vx_rem, vy_rem; // Velocity remainder
    int16_t status;         // 0 = dead, 1+ = alive (various states)
    int16_t dx, dy;         // Delta position for scrolling
    int16_t frame;          // Animation frame
    int16_t lx1, ly1;       // Line draw positions (for tractor beam effect)
    int16_t lx2, ly2;
} Fighter;

// ============================================================================
// GLOBAL GAME STATE
// ============================================================================

// Player state
static int16_t player_x = SCREEN_WIDTH_D2;
static int16_t player_y = SCREEN_HEIGHT_D2;
static int16_t player_vx = 0, player_vy = 0;
static int16_t player_vx_applied = 0, player_vy_applied = 0;
static int16_t player_x_rem = 0, player_y_rem = 0;
static int16_t player_rotation = 0;         // 0 to SHIP_ROTATION_STEPS-1
static int16_t player_rotation_frame = 0;   // Frame counter for rotation speed
static int16_t player_thrust_x = 0;         // Momentum
static int16_t player_thrust_y = 0;
static int16_t player_thrust_delay = 0;
static int16_t player_thrust_count = 0;
static bool player_shield_active = false;
static bool player_boost_active = false;

// Scrolling
static int16_t scroll_dx = 0;
static int16_t scroll_dy = 0;

// Scores and game state
static int16_t player_score = 0;
static int16_t enemy_score = 0;
static int16_t game_score = 0;     // Skill-based score
static int16_t game_level = 1;
static uint16_t game_frame = 0;    // Frame counter (0-59)
static bool game_paused = false;
static bool game_over = false;

// Control mode (from SGDK version)
static uint8_t control_mode = 0;   // 0 = rotational, 1 = directional

// Bullet pools
static Bullet bullets[MAX_BULLETS];
static Bullet ebullets[MAX_EBULLETS];
static Bullet sbullets[MAX_SBULLETS];
static uint16_t bullet_cooldown = 0;
static uint16_t ebullet_cooldown = 0;
static uint16_t sbullet_cooldown = 0;
static uint8_t current_bullet_index = 0;
static uint8_t current_ebullet_index = 0;
static uint8_t current_sbullet_index = 0;

// Fighter pool
static Fighter fighters[MAX_FIGHTERS];
static int16_t active_fighter_count = 0;
static int16_t fighter_speed_1 = 128;
static int16_t fighter_speed_2 = 256;

// Input state
static uint8_t keystates[KEYBOARD_BYTES] = {0};
static bool start_button_pressed = false;

// ============================================================================
// SINE/COSINE LOOKUP TABLES (24 steps for rotation)
// ============================================================================
// TODO: Import these from the original definitions.h or create new ones
// For now, using placeholders - need actual lookup tables for smooth rotation

// ============================================================================
// INITIALIZATION FUNCTIONS
// ============================================================================

/**
 * Initialize bullet pools - mark all bullets as inactive
 */
static void init_bullets(void)
{
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        bullets[i].status = -1;
        bullets[i].x = 0;
        bullets[i].y = 0;
        bullets[i].vx_rem = 0;
        bullets[i].vy_rem = 0;
    }
    
    for (uint8_t i = 0; i < MAX_EBULLETS; i++) {
        ebullets[i].status = -1;
    }
    
    for (uint8_t i = 0; i < MAX_SBULLETS; i++) {
        sbullets[i].status = -1;
    }
}

/**
 * Initialize fighter/enemy pools
 */
static void init_fighters(void)
{
    for (uint8_t i = 0; i < MAX_FIGHTERS; i++) {
        fighters[i].status = 0;  // Dead
        fighters[i].x = -100;    // Off screen
        fighters[i].y = -100;
        fighters[i].vx = 0;
        fighters[i].vy = 0;
        fighters[i].vx_i = 128;  // Default speed
        fighters[i].vy_i = 128;
    }
    active_fighter_count = 0;
}

/**
 * Initialize graphics system
 * Sets up bitmap mode for background and sprite mode for entities
 */
static void init_graphics(void)
{
    // TODO: Set up VGA Mode 3 (320x180 bitmap) for background
    // TODO: Set up VGA Mode 4 (sprites) for player, enemies, bullets
    
    printf("Graphics initialization - TODO\n");
}

/**
 * Initialize game state
 */
static void init_game(void)
{
    // Reset scores
    player_score = 0;
    enemy_score = 0;
    game_score = 0;
    game_level = 1;
    game_frame = 0;
    game_paused = false;
    game_over = false;
    
    // Reset player position and state
    player_x = SCREEN_WIDTH_D2;
    player_y = SCREEN_HEIGHT_D2;
    player_vx = 0;
    player_vy = 0;
    player_rotation = 0;
    player_thrust_x = 0;
    player_thrust_y = 0;
    player_shield_active = false;
    
    // Initialize entity pools
    init_bullets();
    init_fighters();
    
    printf("Game initialized\n");
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

/**
 * Read keyboard and gamepad input
 */
static void handle_input(void)
{
    // TODO: Read keyboard state
    // TODO: Read gamepad state
    // TODO: Handle pause button
    
    // Placeholder - will implement full input handling
}

// ============================================================================
// GAME LOGIC UPDATES
// ============================================================================

/**
 * Update player ship position, rotation, and physics
 */
static void update_player(void)
{
    // TODO: Handle player rotation
    // TODO: Handle thrust/acceleration
    // TODO: Apply momentum and friction
    // TODO: Update player position
    // TODO: Handle screen boundaries
    
    // Placeholder
}

/**
 * Update all active bullets
 */
static void update_bullets(void)
{
    // TODO: Move bullets based on their direction
    // TODO: Check for collisions with enemies
    // TODO: Remove bullets that go off-screen
    
    // Placeholder
}

/**
 * Update all active enemy fighters
 */
static void update_fighters(void)
{
    // TODO: AI movement toward player
    // TODO: Collision detection with player
    // TODO: Tractor beam effect
    
    // Placeholder
}

/**
 * Fire a bullet from the player ship
 */
static void fire_bullet(void)
{
    // TODO: Create bullet at player position
    // TODO: Set bullet direction based on player rotation
    // TODO: Respect cooldown timer
    
    // Placeholder
}

// ============================================================================
// RENDERING
// ============================================================================

/**
 * Draw the HUD (score, health, etc.)
 */
static void draw_hud(void)
{
    // TODO: Display player score
    // TODO: Display enemy score  
    // TODO: Display health/energy bars
    // TODO: Display game level
    
    // Placeholder
}

/**
 * Render all game entities
 */
static void render_game(void)
{
    // TODO: Update sprite positions
    // TODO: Draw bullets
    // TODO: Draw background/stars
    
    // Placeholder
}

// ============================================================================
// MAIN GAME LOOP
// ============================================================================

int main(void)
{
    printf("\n=== RPMegaFighter ===\n");
    printf("Port of Mega Super Fighter Challenge to RP6502\n\n");
    
    // Initialize systems
    init_graphics();
    init_game();
    
    // Enable keyboard input
    xregn(0, 0, 0, 1, KEYBOARD_INPUT);
    
    // Enable gamepad input
    xregn(0, 0, 2, 1, GAMEPAD_INPUT);
    
    printf("Starting game loop...\n");
    printf("Press ESC to quit\n\n");
    
    uint8_t vsync_last = RIA.vsync;
    
    // Main game loop
    while (!game_over) {
        // Wait for vertical sync (60 Hz)
        if (RIA.vsync == vsync_last)
            continue;
        vsync_last = RIA.vsync;
        
        // Read input
        handle_input();
        
        // Check for ESC key to exit
        RIA.addr0 = KEYBOARD_INPUT;
        RIA.step0 = 1;
        uint8_t key_esc = RIA.rw0;
        if (key_esc & 0x01) {  // ESC key in first byte
            if (key(KEY_ESC)) {
                printf("Exiting game...\n");
                break;
            }
        }
        
        // Skip updates if paused
        if (game_paused) {
            continue;
        }
        
        // Update game logic
        update_player();
        update_fighters();
        update_bullets();
        
        // Render frame
        render_game();
        draw_hud();
        
        // Increment frame counter
        game_frame++;
        if (game_frame >= 60) {
            game_frame = 0;
        }
        
        // Check win condition
        if (player_score >= SCORE_TO_WIN) {
            printf("\n*** PLAYER WINS! ***\n");
            game_over = true;
        }
        if (enemy_score >= SCORE_TO_WIN) {
            printf("\n*** ENEMY WINS! ***\n");
            game_over = true;
        }
    }
    
    printf("\nGame Over!\n");
    printf("Player Score: %d\n", player_score);
    printf("Enemy Score: %d\n", enemy_score);
    printf("Game Score: %d\n", game_score);
    
    return 0;
}


    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, x_pos_px, earth_x);
    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, y_pos_px, earth_y);
    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, EARTH_DATA);
    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, log_size, 5);
    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    xregn(1, 0, 1, 5, 4, 0, EARTH_CONFIG, 1, 0);
}

static void enemy_setup()
{
    STATION_CONFIG = EARTH_CONFIG + sizeof(vga_mode4_sprite_t);

    nsprites = 0;

    for (uint8_t i = 0; i < nstation; i++) {

        unsigned ptr = STATION_CONFIG + i * sizeof(vga_mode4_sprite_t);

        station_x[i] = random(1, MAPSIZEM1);
        station_y[i] = random(1, MAPSIZEM1);
        station_status[i] = 1;
        if (station_x[i] > MAPSIZED2){
            station_x[i] -= MAPSIZE;
        }
        if (station_y[i] > MAPSIZED2){
            station_y[i] -= MAPSIZE;
        }

        xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, station_x[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, station_y[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, STATION_DATA);
        xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, 4);
        xram0_struct_set(ptr, vga_mode4_sprite_t, has_opacity_metadata, false);

    }
    nsprites += nstation;

    BATTLE_CONFIG = STATION_CONFIG + nstation * sizeof(vga_mode4_sprite_t);

    for (uint8_t i = 0; i < nbattle; i++) {

        ebullet_timer[i] = 1 << i;

        unsigned ptr = BATTLE_CONFIG + i * sizeof(vga_mode4_sprite_t);

        battle_status[i] = 1;
        battle_x[i] = random(1, MAPSIZEM1);
        battle_y[i] = random(1, MAPSIZEM1);
        if (battle_x[i] > MAPSIZED2){
            battle_x[i] -= MAPSIZE;
        }
        if (battle_y[i] > MAPSIZED2){
            battle_y[i] -= MAPSIZE;
        }

        xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, battle_x[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, battle_y[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, BATTLE_DATA);
        xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, 3);
        xram0_struct_set(ptr, vga_mode4_sprite_t, has_opacity_metadata, false);
    }
    nsprites += nbattle;

    FIGHTER_CONFIG = BATTLE_CONFIG + nbattle * sizeof(vga_mode4_sprite_t);

    for (uint8_t i = 0; i < nfighter; i++) {

        unsigned ptr = FIGHTER_CONFIG + i * sizeof(vga_mode4_sprite_t);

        fighter_x[i] = random(1, MAPSIZEM1);
        fighter_y[i] = random(1, MAPSIZEM1);
        fighter_vxi[i] = random(16, 256);
        fighter_vyi[i] = random(16, 256);
        fighter_status[i] = 1;
        if (fighter_x[i] > MAPSIZED2){
            fighter_x[i] -= MAPSIZE;
        }
        if (fighter_y[i] > MAPSIZED2){
            fighter_y[i] -= MAPSIZE;
        }

        xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, fighter_x[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, fighter_y[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, FIGHTER_DATA);
        xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, 2);
        xram0_struct_set(ptr, vga_mode4_sprite_t, has_opacity_metadata, false);
    }
    nsprites += nfighter;

    xregn(1, 0, 1, 5, 4, 0, STATION_CONFIG, nsprites, 1);


    // Initialize status of bullets
    for (uint8_t i = 0; i < NBULLET; i++) {
        bullet_status[i] = -1;
    }

    for (uint8_t i = 0; i < NEBULLET; i++) {
        ebullet_status[i] = -1;
    }

}


static void text_setup()
{

    TEXT_CONFIG = FIGHTER_CONFIG + nfighter * sizeof(vga_mode4_sprite_t); //Config address for text mode
    text_message_addr = TEXT_CONFIG + NTEXT * sizeof(vga_mode1_config_t); //address to store text message

    for (uint8_t i = 0; i < NTEXT; i++) {

        unsigned ptr = TEXT_CONFIG + i * sizeof(vga_mode1_config_t);

        xram0_struct_set(ptr, vga_mode1_config_t, x_wrap, 0);
        xram0_struct_set(ptr, vga_mode1_config_t, y_wrap, 0);
        xram0_struct_set(ptr, vga_mode1_config_t, x_pos_px, 1); //Bug: first char duplicated if not set to zero
        xram0_struct_set(ptr, vga_mode1_config_t, y_pos_px, 1);
        xram0_struct_set(ptr, vga_mode1_config_t, width_chars, MESSAGE_LENGTH);
        xram0_struct_set(ptr, vga_mode1_config_t, height_chars, 1);
        xram0_struct_set(ptr, vga_mode1_config_t, xram_data_ptr, text_message_addr);
        xram0_struct_set(ptr, vga_mode1_config_t, xram_palette_ptr, 0xFFFF);
        xram0_struct_set(ptr, vga_mode1_config_t, xram_font_ptr, 0xFFFF);
    }

    // 4 parameters: text mode, 8-bit, config, plane
    xregn(1, 0, 1, 4, 1, 3, TEXT_CONFIG, 2);


    snprintf(message, MESSAGE_LENGTH, " SCORE %05d ", score);
 
    RIA.addr0 = text_message_addr;
    RIA.step0 = 1;
    for (uint8_t i = 0; i < sizeof(message); i++)
    {
         if (i > 13 && i < 22){
            RIA.rw0 = 0xDB; //block
            RIA.rw0 = 0xD0; 
            RIA.rw0 = 0x10;
        } else if (i > 22 && i < 31) {
            RIA.rw0 = 0xDB; // block
            RIA.rw0 = 0xB2;
            RIA.rw0 = 0x10;
        } else if (i > 32) {
            RIA.rw0 = 0x99; //silly bomb symbol
            RIA.rw0 = 0xA2;
            RIA.rw0 = 0x00;
        } else {
            RIA.rw0 = message[i];
            RIA.rw0 = 0xE0;
            RIA.rw0 = 0x00;
        }
    }

}

static void update_score()
{

    snprintf(message, MESSAGE_LENGTH, " SCORE %05d ", score);

    if (energy < 0){
        energy = 255;
    }

    if (health < 0){
        health = 255;
    }

    RIA.addr0 = text_message_addr;
    RIA.step0 = 1;

    uint8_t i;
    uint8_t j;

    for (i = 0; i < 14; i++){
        RIA.rw0 = message[i];
        RIA.rw0 = 0xE0;
        RIA.rw0 = 0x00;
    }

    j = (health >> 5) + 1;
    for (i = 14; i < 14 + j; i++){
        RIA.rw0 = 0xDB; //block for health
        RIA.rw0 = 0xD0; 
        RIA.rw0 = 0x10;
    }

    for (i = 14 + j; i < 22; i++){
        RIA.rw0 = 0xDB; //block for no health
        RIA.rw0 = 0x12; 
        RIA.rw0 = 0x10;
    }

    RIA.rw0 = message[22];
    RIA.rw0 = 0xE0;
    RIA.rw0 = 0x00;

    j = (energy >> 5) + 1;
    for (i = 23; i < 23 + j; i++){
        RIA.rw0 = 0xDB; //block for energy
        RIA.rw0 = 0xB2; 
        RIA.rw0 = 0x10;
    }

    for (i = 23 + j; i < 31; i++){
        RIA.rw0 = 0xDB; //block for no energy
        RIA.rw0 = 0x12; 
        RIA.rw0 = 0x10;
    }

    RIA.rw0 = message[31];
    RIA.rw0 = 0xE0;
    RIA.rw0 = 0x00;

    RIA.rw0 = message[32];
    RIA.rw0 = 0xE0;
    RIA.rw0 = 0x00;

    for (i = 33; i < MESSAGE_LENGTH; i++){
        RIA.rw0 = 0x99; //block for energy
        RIA.rw0 = 0xA2; 
        RIA.rw0 = 0x00;
    }

}

static void display_pause_message(bool show_paused)
{
    // Display PAUSED in center of screen using direct pixel writes
    // Simple text at approximately center screen (y=90)
    const uint8_t pause_color = 0xFF;
    const uint16_t center_x = 120;
    const uint16_t center_y = 85;
    
    if (show_paused) {
        // Draw "PAUSED" using simple block letters
        // P (x=120-145)
        for (uint16_t x = center_x; x < center_x + 3; x++) {
            for (uint16_t y = center_y; y < center_y + 12; y++) {
                set(x, y, pause_color);
            }
        }
        for (uint16_t x = center_x; x < center_x + 8; x++) {
            set(x, center_y, pause_color);
            set(x, center_y + 6, pause_color);
        }
        for (uint16_t y = center_y; y < center_y + 7; y++) {
            set(center_x + 8, y, pause_color);
        }
        
        // A (x=132-145)
        for (uint16_t y = center_y + 3; y < center_y + 12; y++) {
            set(center_x + 12, y, pause_color);
            set(center_x + 20, y, pause_color);
        }
        for (uint16_t x = center_x + 12; x < center_x + 21; x++) {
            set(x, center_y + 3, pause_color);
            set(x, center_y + 7, pause_color);
        }
        
        // U (x=148-161)
        for (uint16_t y = center_y; y < center_y + 12; y++) {
            set(center_x + 24, y, pause_color);
            set(center_x + 32, y, pause_color);
        }
        for (uint16_t x = center_x + 24; x < center_x + 33; x++) {
            set(x, center_y + 11, pause_color);
        }
        
        // S (x=164-177)
        for (uint16_t x = center_x + 36; x < center_x + 44; x++) {
            set(x, center_y, pause_color);
            set(x, center_y + 6, pause_color);
            set(x, center_y + 11, pause_color);
        }
        for (uint16_t y = center_y; y < center_y + 7; y++) {
            set(center_x + 36, y, pause_color);
        }
        for (uint16_t y = center_y + 6; y < center_y + 12; y++) {
            set(center_x + 44, y, pause_color);
        }
        
        // E (x=180-193)
        for (uint16_t y = center_y; y < center_y + 12; y++) {
            set(center_x + 48, y, pause_color);
        }
        for (uint16_t x = center_x + 48; x < center_x + 56; x++) {
            set(x, center_y, pause_color);
            set(x, center_y + 6, pause_color);
            set(x, center_y + 11, pause_color);
        }
        
        // D (x=196-209)
        for (uint16_t y = center_y; y < center_y + 12; y++) {
            set(center_x + 60, y, pause_color);
        }
        for (uint16_t x = center_x + 60; x < center_x + 67; x++) {
            set(x, center_y, pause_color);
            set(x, center_y + 11, pause_color);
        }
        for (uint16_t y = center_y + 1; y < center_y + 11; y++) {
            set(center_x + 67, y, pause_color);
        }
    } else {
        // Clear PAUSED text by drawing black
        for (uint16_t x = center_x; x < center_x + 68; x++) {
            for (uint16_t y = center_y; y < center_y + 12; y++) {
                set(x, y, 0x00);
            }
        }
    }
}

static void battle_update()
{

    RIA.step0 = sizeof(vga_mode4_sprite_t);
    RIA.step1 = sizeof(vga_mode4_sprite_t);
    RIA.addr0 = BATTLE_CONFIG;
    RIA.addr1 = BATTLE_CONFIG + 1;
    

    for (uint8_t i = 0; i < nbattle; i++) {

        battle_x[i] += -dx + battle_dx[i];
        if (battle_x[i] <= MMAPSIZED2){
            battle_x[i] += MAPSIZE;
        }
        if (battle_x[i] > MAPSIZED2){
            battle_x[i] -= MAPSIZE;
        }

        RIA.rw0 = battle_x[i] & 0xff;
        RIA.rw1 = (battle_x[i] >> 8) & 0xff;

    }

    RIA.addr0 = BATTLE_CONFIG + 2;
    RIA.addr1 = BATTLE_CONFIG + 3;

    for (uint8_t i = 0; i < nbattle; i++) {

        battle_y[i] += -dy + battle_dy[i];
        if (battle_y[i] <= MMAPSIZED2){
            battle_y[i] += MAPSIZE;
        }
        if (battle_y[i] > MAPSIZED2){
            battle_y[i] -= MAPSIZE;
        }    

        RIA.rw0 = battle_y[i] & 0xff;
        RIA.rw1 = (battle_y[i] >> 8) & 0xff;

    }
}

static void station_update(int16_t dx, int16_t dy)
{

    RIA.step0 = sizeof(vga_mode4_sprite_t);
    RIA.step1 = sizeof(vga_mode4_sprite_t);
    RIA.addr0 = STATION_CONFIG;
    RIA.addr1 = STATION_CONFIG + 1;
    

    for (uint8_t i = 0; i < nstation; i++) {

        station_x[i] -= dx;
        if (station_x[i] <= MMAPSIZED2){
            station_x[i] += MAPSIZE;
        }
        if (station_x[i] > MAPSIZED2){
            station_x[i] -= MAPSIZE;
        }

        RIA.rw0 = station_x[i] & 0xff;
        RIA.rw1 = (station_x[i] >> 8) & 0xff;
        
    }

    RIA.addr0 = STATION_CONFIG + 2;
    RIA.addr1 = STATION_CONFIG + 3;

    for (uint8_t i = 0; i < nstation; i++) {

        station_y[i] -= dy;
        if (station_y[i] <= MMAPSIZED2){
            station_y[i] += MAPSIZE;
        }
        if (station_y[i] > MAPSIZED2){
            station_y[i] -= MAPSIZE;
        }    

        RIA.rw0 = station_y[i] & 0xff;
        RIA.rw1 = (station_y[i] >> 8) & 0xff;

    }
}

static void fighter_update()
{

    RIA.step0 = sizeof(vga_mode4_sprite_t);
    RIA.step1 = sizeof(vga_mode4_sprite_t);
    RIA.addr0 = FIGHTER_CONFIG;
    RIA.addr1 = FIGHTER_CONFIG + 1;
    

    for (uint8_t i = 0; i < nfighter; i++) {

        if (fighter_status[i]){
            fighter_x[i] += -dx + fighter_dx[i];
        }

        if (fighter_x[i] <= MMAPSIZED2){
            fighter_x[i] += MAPSIZE;
        }
        if (fighter_x[i] > MAPSIZED2){
            fighter_x[i] -= MAPSIZE;
        }

        RIA.rw0 = fighter_x[i] & 0xff;
        RIA.rw1 = (fighter_x[i] >> 8) & 0xff;
    }

    RIA.addr0 = FIGHTER_CONFIG + 2;
    RIA.addr1 = FIGHTER_CONFIG + 3;

    for (uint8_t i = 0; i < nfighter; i++) {

        if (fighter_status[i]){
            fighter_y[i] += -dy + fighter_dy[i];
        }

        if (fighter_y[i] <= MMAPSIZED2){
            fighter_y[i] += MAPSIZE;
        }
        if (fighter_y[i] > MAPSIZED2){
            fighter_y[i] -= MAPSIZE;
        }    

        RIA.rw0 = fighter_y[i] & 0xff;
        RIA.rw1 = (fighter_y[i] >> 8) & 0xff;

    }

}

static void bullet_spaceship(uint8_t b_id)
{
    if (x      < ebullet_x[b_id] + 2 &&
        x + 10 > ebullet_x[b_id]     &&
        y      < ebullet_y[b_id] + 2 &&
        y + 10 > ebullet_y[b_id])
    {
        ebullet_status[b_id] = -1;
        energy -= 30;
        health -= 30;
        update_score();
    }
}

void battle_bullets()
{
    //Update bullets
    for (uint8_t i = 0; i < NEBULLET; i++) {
        if (ebullet_status[i] >= 0){
            set(ebullet_x[i], ebullet_y[i], 0x00);
            // //Check for collision
            bullet_spaceship(i);
        }

        if (ebullet_status[i] >= 0){
            bvx = sin_fix[ebullet_status[i]];
            bvy = cos_fix[ebullet_status[i]];
            bvxapp = ( (bvx + ebvxrem[i]) >> 7);
            bvyapp = ( (bvy + ebvyrem[i]) >> 7);
            ebvxrem[i] = bvx + ebvxrem[i] - bvxapp * 128; 
            ebvyrem[i] = bvy + ebvyrem[i] - bvyapp * 128;
            ebullet_x[i] += bvxapp - dx;
            ebullet_y[i] += bvyapp - dy;

            if (ebullet_x[i] > 0 && ebullet_x[i] < 320 && ebullet_y[i] > 0 && ebullet_y[i] < 180){
                set(ebullet_x[i], ebullet_y[i], 0xFF);
            } else {
                ebullet_status[i] = -1;
            }
        }
    }
}

uint8_t battle_attack()
{
    uint8_t attack = 0;

    int16_t fdx; //Position of fighter relative to Earth
    int16_t fdy;
    int16_t bavx; //battle station velocity
    int16_t bavy;
    int16_t bavxapp;
    int16_t bavyapp;

    for (uint8_t i = 0; i < nbattle; i++) {

        
        // Get diffs for space ship
        fdx = x - battle_x[i] + (16 * vxapp);
        fdy = y - battle_y[i] + (16 * vyapp);

        //Fire ze missiles!
        if (battle_x[i] > 1 && battle_x[i] < SWIDTH && battle_y[i] > 1 && battle_y[i] < SHEIGHT){
            // printf("Battle time..");
            if (ebullet_timer[i] > NEBULLET_TIMER_MAX){
                ebullet_timer[i] = 0;

                //Calculate angle with dxdy lookup table
                uint16_t afdx8 = abs(fdx) >> 3;
                uint16_t afdy8 = abs(fdy) >> 3;
                uint8_t rib = dxdy_table[afdx8 + (afdy8 * 41)];
                if (fdx < 0 && fdy > 0){
                    rib = 24 - rib;
                } else if (fdx > 0 && fdy < 0){
                    rib = 12 - rib;
                } else if (fdx < 0 && fdy <0){
                    rib +=12;
                } 
                if (rib > 24){
                    rib -= 24;
                }

                if (ebullet_status[ebullet_c] < 0){
                    // printf("Bullet away.. %d %d %d %d %d \n",fdx,fdy,afdx8,afdy8,rib);
                    ebullet_status[ebullet_c] = rib;
                    ebullet_x[ebullet_c] = battle_x[i]+4;
                    ebullet_y[ebullet_c] = battle_y[i]+4;
                    ebullet_c += 1;
                    if (ebullet_c >= NEBULLET){
                        ebullet_c = 0;
                    }
                }
            }
        }

        // Get diffs for Earth
        fdx = earth_x - battle_x[i];
        fdy = earth_y - battle_y[i];

        if (abs(fdx) < 50 && abs(fdy) < 50) {
                battle_dx[i] =  0;
                battle_dy[i] =  0;
                battle_xrem[i] = 0;
                battle_yrem[i] = 0;
                bavx = 0;
                bavy = 0;

                attack = +1;

            } else {

                if (fdx > 0){
                    bavx = 32; 
                } else {
                    bavx = -32;
                }

                if (fdy > 0){
                    bavy = 32; 
                } else {
                    bavy = -32;
                }

            }

        bavxapp = (bavx + battle_xrem[i]) >> 8;
        bavyapp = (bavy + battle_yrem[i]) >> 8;
        battle_xrem[i] = bavx + battle_xrem[i] - bavxapp * 256;
        battle_yrem[i] = bavy + battle_yrem[i] - bavyapp * 256;
        battle_dx[i] = bavxapp;
        battle_dy[i] = bavyapp;

    }

    battle_update();
    battle_bullets();

    return attack;
}

uint8_t fighter_attack()
{
    
    int16_t fdx; //Position of fighter relative to space ship
    int16_t fdy;
    int16_t fvxapp; //Applied velocity to space ship.
    int16_t fvyapp;
    uint8_t attack = 0;

    for (uint8_t i = 0; i < nfighter; i++) {

        if (fighter_status[i] > 0){

            fdx = x - fighter_x[i];
            fdy = y - fighter_y[i];

            if (abs(fdx) < 30 && abs(fdy) < 30) {
                fighter_dx[i] =  0;
                fighter_dy[i] =  0;
                fighter_xrem[i] = 0;
                fighter_yrem[i] = 0;

                attack = +1; // Slow down the good-guy space craft

                if (fighter_status[i] == 1) {
                    energy -= 2; // Remove energy
                    health -= 1; // Remove health
                    update_score(); 

                    draw_line(0xBE, x+4, y+4, fighter_x[i]+2, fighter_y[i]+2);
                    fighter_lx1old[i] = x + 4;
                    fighter_ly1old[i] = y + 4;
                    fighter_lx2old[i] = fighter_x[i] + 2;
                    fighter_ly2old[i] = fighter_y[i] + 2;
                    fighter_status[i] = 2;
                } else if (fighter_status[i] == 2) {
                    draw_line(0x00, fighter_lx1old[i], fighter_ly1old[i], fighter_lx2old[i], fighter_ly2old[i]);
                    fighter_status[i] = 3;
                } else {
                    fighter_status[i] += 1;
                    if (fighter_status[i] > 32){
                        fighter_status[i] = 1;
                    }
                }
                

            } else {
                if (update_sch%30 == 0){ 
                    if ((rand() >> 15) +1){
                        if (fdx > 0){
                            fighter_vx[i] = fighter_vxi[i]; 
                        } else {
                            fighter_vx[i] = -fighter_vxi[i];
                        }
                    }

                    if ((rand() >> 15) +1){
                        if (fdy > 0){
                            fighter_vy[i] = fighter_vyi[i]; 
                        } else {
                            fighter_vy[i] = -fighter_vyi[i];
                        }
                    }
                    
                }

                fvxapp = (fighter_vx[i] + fighter_xrem[i]) >> 8;
                fvyapp = (fighter_vy[i] + fighter_yrem[i]) >> 8;
                fighter_xrem[i] = fighter_vx[i] + fighter_xrem[i] - fvxapp * 256;
                fighter_yrem[i] = fighter_vy[i] + fighter_yrem[i] - fvyapp * 256;
                fighter_dx[i] = fvxapp;
                fighter_dy[i] = fvyapp;

            }
        }

    }

    fighter_update();

    if (attack > 4){
        attack = 4;
    }

    return attack;

}

static void bullet_fighter(uint8_t b_id)
{
    for (uint8_t i = 0; i < nfighter; i++) {
        if (fighter_x[i] < bullet_x[b_id] + 2 &&
            fighter_x[i] + 6 > bullet_x[b_id] &&
            fighter_y[i] < bullet_y[b_id] + 2 &&
            fighter_y[i] + 6 > bullet_y[b_id])
        {
                // Catch case of fighter destroyed before we remove beam from screen
                if (fighter_status[i] == 2) {
                    draw_line(0x00, fighter_lx1old[i], fighter_ly1old[i], fighter_lx2old[i], fighter_ly2old[i]);
                }
                bullet_status[b_id] = -1;
                fighter_status[i] = 0;
                fighter_x[i] = -10; //Move sprite off-screen
                fighter_y[i] = -10;
                score += 5;
                update_score();
        }
    } 
}

static void create_new_fighter()
{
    for (uint8_t i = 0; i < nfighter; i++) {
        if (fighter_status[i] == 0){
            for (uint8_t j = 0; j < nstation; j++) {
                if(station_status[j]){
                    fighter_status[i] = 1;
                    fighter_x[i] = station_x[j];
                    fighter_y[i] = station_y[j];
                }
            }
            break;
        }
    }
}

static void earth_update(int16_t dx, int16_t dy)
{
    // Update positions

    earth_x = earth_x - dx;
    if (earth_x <= MMAPSIZED2){
        earth_x += MAPSIZE;
    }
    if (earth_x > MAPSIZED2){
        earth_x -= MAPSIZE;
    }

    earth_y = earth_y - dy;
    if (earth_y <= MMAPSIZED2){
        earth_y += MAPSIZE;
    }
    if (earth_y > MAPSIZED2){
        earth_y -= MAPSIZE;
    }


    RIA.step0 = sizeof(vga_mode4_sprite_t);
    RIA.step1 = sizeof(vga_mode4_sprite_t);
    RIA.addr0 = EARTH_CONFIG;
    RIA.addr1 = EARTH_CONFIG + 1;

    RIA.rw0 = earth_x & 0xff;
    RIA.rw1 = (earth_x >> 8) & 0xff;

    RIA.addr0 = EARTH_CONFIG + 2;
    RIA.addr1 = EARTH_CONFIG + 3;

    RIA.rw0 = earth_y & 0xff;
    RIA.rw1 = (earth_y >> 8) & 0xff;
}

static void ship_update()
{
    // Copy positions during vblank
    RIA.step0 = sizeof(vga_mode4_asprite_t);
    RIA.step1 = sizeof(vga_mode4_asprite_t);
    RIA.addr0 = SPACECRAFT_CONFIG + 12;
    RIA.addr1 = SPACECRAFT_CONFIG + 13;

    //val = x;
    RIA.rw0 = x & 0xff;
    RIA.rw1 = (x >> 8) & 0xff;

    RIA.addr0 = SPACECRAFT_CONFIG + 14;
    RIA.addr1 = SPACECRAFT_CONFIG + 15;

    //val = y;
    RIA.rw0 = y & 0xff;
    RIA.rw1 = (y >> 8) & 0xff;

    // Update rotation
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[0],  cos_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[1], -sin_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[2],  t2_fix4[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[3],  sin_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[4],  cos_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[5],  
        t2_fix4[ri_max - ri + 1]);
}

static void setup()
{

    BITMAP_CONFIG = VGA_CONFIG_START;  //Bitmap Config - Planet 0

    // xregn : address 1, 0 ,0 then we are setting '1' bit.  That bit will be set to 2
    xregn(1, 0, 0, 1, 2); // set 320x180 canvas, last number selects Canvas size 

    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, x_pos_px, 0);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, y_pos_px, 0);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, width_px, 320);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, height_px, 180);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, xram_data_ptr, 0);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);

    // xregn : address 1, 0 ,1 then we are setting '3' bits.  Thoses bits will be set to 3, 3, 0xFF00
    xregn(1, 0, 1, 4, 3, 3, BITMAP_CONFIG, 1); // Mode 3, 4-bit colour.  2nd last is bit depth, last is address config

    SPACECRAFT_CONFIG =  BITMAP_CONFIG + sizeof(vga_mode3_config_t);

    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[0],  cos_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[1], -sin_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[2],  0);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[3],  sin_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[4],  cos_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[5],  0);

    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, x_pos_px, x);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, y_pos_px, y);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, xram_sprite_ptr, SPACESHIP_DATA);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, log_size, 3);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, has_opacity_metadata, false);

    xregn(1, 0, 1, 5, 4, 1, SPACECRAFT_CONFIG, 1, 2);

    // Clear out extended memory for bit-map video
    
    RIA.addr0 = 0;
    RIA.step0 = 1;
    for (unsigned i = vlen; i--;)
        RIA.rw0 = 0;    

}

void setup_stars() //Set up random positions and colours for our stars.
{
    for (uint8_t i = 0; i < NSTAR; i++) {

        star_x[i] = random(1, STARFIELD_X);
        star_y[i] = random(1, STARFIELD_Y);
        star_colour[i] = random(1, 255);
        star_x_old[i] = star_x[i];
        star_y_old[i] = star_y[i];

    }
}

void plot_stars(int16_t dx, int16_t dy)
{
    for (uint8_t i = 0; i < NSTAR; i++){

        // Clear previous stars
        if (star_x_old[i] > 0 && star_x_old[i] < 320 && star_y_old[i] > 0 && star_y_old[i] < 180){
            set(star_x_old[i], star_y_old[i], 0x00);
        }

        star_x[i] = star_x_old[i] - dx;
        if (star_x[i] <= 0){
            star_x[i] += STARFIELD_X;
        }
        if (star_x[i] > STARFIELD_X){
            star_x[i] -= STARFIELD_X;
        }
        star_x_old[i] = star_x[i];
        
        star_y[i] = star_y_old[i] - dy;
        if (star_y[i] <= 0){
            star_y[i] += STARFIELD_Y;
        }
        if (star_y[i] > STARFIELD_Y){
            star_y[i] -= STARFIELD_Y;
        }
        star_y_old[i] = star_y[i];

         if (star_x[i] > 0 && star_x[i] < 320 && star_y[i] > 0 && star_y[i] < 180){
            set(star_x[i], star_y[i], star_colour[i]);
            // printf("stars %d, %d, %d \n", star_x[i], star_y[i], star_colour[i]);
        } 

    }
}

int main(void)
{

    setup(); //Set up Graphics
    setup_stars(); // Set up stars
    earth_setup(); // Set up Earth.
    enemy_setup(); // Set up Enemies
    text_setup();  // Set up text  

    // Motion of the screen
    dx = 0;
    dy = 0;

    //Plot stars
    plot_stars(dx, dy);

    // Makes rotation of ship slower
    uint16_t iframe = 0;
    uint16_t iframe_old = SHIP_ROT_SPEED;

    uint8_t v; //Used to test for V-sync

    int xtry = x; //For displaying Space ship
    int ytry = y;

    int xrem = 0; //Tracking remainder for smooth motion of space ship
    int yrem = 0;

    // int vxapp = 0; //Applied motion to the space ship position
    // int vyapp = 0;

    uint8_t attack = 0; //enemy stopping beam

    int val; //used for updating sprites

    uint8_t tdelay = 0;     // Counter for thrust/momentum/friction 
    uint8_t tdelay_max = 8; // momentum
    uint8_t tcount = 0;

    int thrust_x = 0;       // Initialize amount of thrust applied (acts like momentum)
    int thrust_y = 0;

    int16_t thx = 0; //Checking thrust for max allowed values.
    int16_t thy = 0;

    VIAp.ddra = 0; //set GPIO as input (probably only need to do once...)

    // Turn on USB keyboard I/O
    // xreg_ria_keyboard(KEYBOARD_INPUT);
    xregn( 0, 0, 0, 1, KEYBOARD_INPUT);

    // Turn on USB gamepad I/O (device 0, channel 0, address 2)
    xregn( 0, 0, 2, 1, GAMEPAD_INPUT);

    while (1) { //Infinite Game Loop

        if (RIA.vsync == v) //Run updates on V-sync.  Ideally this is 1/60 s 
            continue;
        v = RIA.vsync; 

        vx=0; // Velocity to apply to Spacecraft.
        vy=0;

        ship_update();

        if (update_sch%FIGHTER_RATE == 0){
            create_new_fighter();
        }

        // Health and energy regeneration rate
        if (update_sch % 32 == 0){
            if (health < 255){
                health += 1;
            }
            if (energy < 255){
                energy += 2;
            }
            update_score();
        }

        // ===== Keyboard Controls ===== //
        RIA.addr0 = KEYBOARD_INPUT;
        RIA.step0 = 2;
        keystates[0] = RIA.rw0;
        RIA.step0 = 1;
        keystates[2] = RIA.rw0;
        RIA.step0 = 2; 
        keystates[3] = RIA.rw0;                                                
        RIA.step0 = 4;                                                
        keystates[5] = RIA.rw0;
        RIA.step0 = 0;                        
        keystates[9] = RIA.rw0;
        // don't knpw why but have to reset address or add delay to make it (reading 10) work 
        RIA.addr0 = KEYBOARD_INPUT + 10;                                                   
        keystates[10] = RIA.rw0;

        // ===== Read Gamepad Data ===== //
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

        // ===== Gamepad Diagnostic Output ===== //
        // Print gamepad state when any button is pressed on gamepad 0
        // if (gamepad[0].dpad & GP_CONNECTED) {
        //     if (gamepad[0].dpad || gamepad[0].btn0 || gamepad[0].btn1) {
        //         // Only print occasionally to avoid flooding the terminal
        //         if (update_sch % 30 == 0) {
        //             printf("GP0: DPAD=%02X STICKS=%02X BTN0=%02X BTN1=%02X ", 
        //                    gamepad[0].dpad, gamepad[0].sticks, gamepad[0].btn0, gamepad[0].btn1);
                    
        //             // Decode DPAD
        //             if (gamepad[0].dpad & GP_DPAD_UP) printf("UP ");
        //             if (gamepad[0].dpad & GP_DPAD_DOWN) printf("DN ");
        //             if (gamepad[0].dpad & GP_DPAD_LEFT) printf("LF ");
        //             if (gamepad[0].dpad & GP_DPAD_RIGHT) printf("RT ");
                    
        //             // Decode BTN0
        //             if (gamepad[0].btn0 & GP_BTN_A) printf("A ");
        //             if (gamepad[0].btn0 & GP_BTN_B) printf("B ");
        //             if (gamepad[0].btn0 & GP_BTN_C) printf("C ");
        //             if (gamepad[0].btn0 & GP_BTN_X) printf("X ");
        //             if (gamepad[0].btn0 & GP_BTN_Y) printf("Y ");
        //             if (gamepad[0].btn0 & GP_BTN_Z) printf("Z ");
        //             if (gamepad[0].btn0 & GP_BTN_L1) printf("L1 ");
        //             if (gamepad[0].btn0 & GP_BTN_R1) printf("R1 ");
                    
        //             // Decode BTN1
        //             if (gamepad[0].btn1 & GP_BTN_L2) printf("L2 ");
        //             if (gamepad[0].btn1 & GP_BTN_R2) printf("R2 ");
        //             if (gamepad[0].btn1 & GP_BTN_SELECT) printf("SEL ");
        //             if (gamepad[0].btn1 & GP_BTN_START) printf("START ");
        //             if (gamepad[0].btn1 & GP_BTN_HOME) printf("HOME ");
        //             if (gamepad[0].btn1 & GP_BTN_L3) printf("L3 ");
        //             if (gamepad[0].btn1 & GP_BTN_R3) printf("R3 ");
                    
        //             printf("\n");
        //         }
        //     }
        // }

        // ===== Check for Pause (Start button = BTN1 0x02) ===== //
        if (gamepad[0].dpad & GP_CONNECTED) {
            if (gamepad[0].btn1 & 0x02) {  // Start button pressed
                if (!start_button_pressed) {
                    // Button just pressed - toggle pause
                    game_paused = !game_paused;
                    display_pause_message(game_paused);
                    start_button_pressed = true;
                }
            } else {
                // Button released
                start_button_pressed = false;
            }
        }

        // Skip game updates if paused
        if (game_paused) {
            // Check for A+C buttons pressed together to exit (BTN0: A=0x04, C=0x20)
            if ((gamepad[0].btn0 & 0x04) && (gamepad[0].btn0 & 0x20)) {
                break;  // Exit the game loop
            }
            continue;
        }

        // =====  ATARI 7800 Arcade Stick via GPIO ===== //

        // We only periodically sample left/right to make rotation easier to control
        if (iframe >= iframe_old){
            iframe = 0;

            // Check keyboard
            if (!(keystates[0] & 1)) {
                if (key(KEY_LEFT)){ //Rotate left                      
                    // update rotation
                    if (ri == ri_max){
                        ri = 0;
                    } else {
                        ri += 1;
                    } 
                }

                if (key(KEY_RIGHT)){ // Rotate right
                    // update rotation
                    if (ri == 0){
                        ri = ri_max;
                    } else {
                        ri -= 1;
                    }
                }

            }

            // Check gamepad (use first connected gamepad)
            if (gamepad[0].dpad & GP_CONNECTED) {
                // D-pad left (mapped to STICKS bit 0x04)
                if (gamepad[0].sticks & GP_LSTICK_LEFT) {
                    // update rotation
                    if (ri == ri_max){
                        ri = 0;
                    } else {
                        ri += 1;
                    }
                }

                // D-pad right (mapped to STICKS bit 0x08)
                if (gamepad[0].sticks & GP_LSTICK_RIGHT) {
                    // update rotation
                    if (ri == 0){
                        ri = ri_max;
                    } else {
                        ri -= 1;
                    }
                }
            }

            // // Check joystick controller
            // if (VIAp.pa > 0){

            //     //Rotate counter
            //     if ((VIAp.pa & 0x04) >> 2){ //Rotate left
            //         // update rotation
            //         if (ri == ri_max){
            //             ri = 0;
            //         } else {
            //             ri += 1;
            //         } 
            //     }

            //     if ((VIAp.pa & 0x08) >> 3){ //Rotate right
            //         // update rotation
            //         if (ri == 0){
            //             ri = ri_max;
            //         } else {
            //             ri -= 1;
            //         }
            //     }

            // }
        }


        iframe+=1;

        if (!(keystates[0] & 1)) {

            if (key(KEY_ESC)){
                exit(0);
            }

            if (key(KEY_UP)){ // Up direction -- applies thrust..
                vx = -sin_fix[ri];
                vy = -cos_fix[ri];
                tdelay = 0;
            }

            // Left fire button -- Bullets 
            if (key(KEY_SPACE)){
                if (bullet_timer > NBULLET_TIMER_MAX){
                    bullet_timer = 0;
                    if (bullet_status[bullet_c] < 0){
                        bullet_status[bullet_c] = ri;
                        bullet_x[bullet_c] = x+4;
                        bullet_y[bullet_c] = y+4;
                        bullet_c += 1;
                        if (bullet_c >= NBULLET){
                            bullet_c = 0;
                        }
                    }
                }
            } 

        }

        // Check gamepad controls (use first connected gamepad)
        if (gamepad[0].dpad & GP_CONNECTED) {
            // Thrust: D-pad up (mapped to STICKS bit 0x01)
            if (gamepad[0].sticks & GP_LSTICK_UP) {
                vx = -sin_fix[ri];
                vy = -cos_fix[ri];
                tdelay = 0;
            }

            // Fire bullets: A button (BTN0 bit 0x04) or C button (BTN0 bit 0x20)
            // Note: Your gamepad maps A=0x04, B=0x02, C=0x20, Start=BTN1 0x02
            if ((gamepad[0].btn0 & 0x04) || (gamepad[0].btn0 & 0x20)) {
                if (bullet_timer > NBULLET_TIMER_MAX){
                    bullet_timer = 0;
                    if (bullet_status[bullet_c] < 0){
                        bullet_status[bullet_c] = ri;
                        bullet_x[bullet_c] = x+4;
                        bullet_y[bullet_c] = y+4;
                        bullet_c += 1;
                        if (bullet_c >= NBULLET){
                            bullet_c = 0;
                        }
                    }
                }
            }
        }

        // if (VIAp.pa > 0){ //test if joystick has input
        //     // Up direction -- applies thrust..
        //     if (VIAp.pa & 0x01){
        //         vx = -sin_fix[ri];
        //         vy = -cos_fix[ri];
        //         tdelay = 0;
        //     }
        //     // Down direction
        //     if ((VIAp.pa & 0x02) >> 1){
        //         // Will apply Shield with Down
        //     }

        //     // Left fire button -- Bullets 
        //     if ((VIAp.pa & 0x50) == 0x50){
        //         if (bullet_timer > NBULLET_TIMER_MAX){
        //             bullet_timer = 0;
        //             if (bullet_status[bullet_c] < 0){
        //                 bullet_status[bullet_c] = ri;
        //                 bullet_x[bullet_c] = x+4;
        //                 bullet_y[bullet_c] = y+4;
        //                 bullet_c += 1;
        //                 if (bullet_c >= NBULLET){
        //                     bullet_c = 0;
        //                 }
        //             }
        //         }
        //     } 
        //     // else { // Does not work?? 
        //     //     bullet_timer = NBULLET_TIMER_MAX; //If button is released we can immediately fire again.
        //     // }

        //     // Right fire button -- EMP
        //     if ((VIAp.pa & 0x50) == 0x30){
        //         // Will apply BIG BOMB here.
        //     }

        // }

        bullet_timer += 1;
        for (uint8_t ii = 0; ii < NBATTLE_MAX; ii++){
            ebullet_timer[ii] += 1;
        }
        

        //Update fighters and track attacks
        attack = fighter_attack();
        if (attack > 0){
            vx = vx >> attack;
            vy = vy >> attack;
            thrust_x = thrust_x >> attack;
            thrust_y = thrust_y >> attack;
        }

        //Update Battle stations
        attack = battle_attack();
        
        //Update position
        vxapp = ( (vx + xrem + thrust_x ) >> 9); //Apply velocity, remainder and momentum 
        vyapp = ( (vy + yrem + thrust_y ) >> 9); // 9 and 512 must balance (e.g., 2^9 = 512)
        xrem = vx + xrem + thrust_x - vxapp * 512; //Update remainder
        yrem = vy + yrem + thrust_y - vyapp * 512;
        xtry = x + vxapp; //Update ship position
        ytry = y + vyapp;

        //Update thrust if joystick is held.
        thx = thrust_x + (vx >> 4);
        if (thx < 1024 && thx > -1024){
            thrust_x = thx;
        }
        thy = thrust_y + (vy >> 4);
        if (thy < 1024 && thy > -1024){
            thrust_y = thy;
        }

        //Update momentum by applying friction 
        if (tdelay < tdelay_max && tcount > 50){
            tdelay += 1;
            tcount = 0;
            if (vx == 0){
                thrust_x = thrust_x >> 1;
            }
            if (vy == 0){
                thrust_y = thrust_y >> 1;
            }
        }
        if (tdelay >= tdelay_max){
            thrust_x = 0;
            thrust_y = 0;
        }
        tcount += 1;

        // Keep spacecraft in bounds
        if (xtry > BX1 && xtry < BX2){
            x = xtry;
            dx = 0;
        } else {
            dx = (xtry - x);
        }
            
        if (ytry > BY1 && ytry < BY2){
            y = ytry;
            dy = 0;
        } else {
            dy = (ytry - y);
        }

        //Update stars and stationary sprites for screen scroll 
        if (dx != 0 || dy != 0){

            plot_stars(dx, dy);

            earth_update(dx, dy);

            station_update(dx, dy);
    
        }

        

        //Update bullets
        for (uint8_t ii = 0; ii < NBULLET; ii++) {
            if (bullet_status[ii] >= 0){
                set(bullet_x[ii], bullet_y[ii], 0x00);
                //Check for collision
                bullet_fighter(ii);
            }

            if (bullet_status[ii] >= 0){
                bvx = -sin_fix[bullet_status[ii]];
                bvy = -cos_fix[bullet_status[ii]];
                bvxapp = ( (bvx + bvxrem[ii]) >> 6);
                bvyapp = ( (bvy + bvyrem[ii]) >> 6);
                bvxrem[ii] = bvx + bvxrem[ii] - bvxapp * 64; 
                bvyrem[ii] = bvy + bvyrem[ii] - bvyapp * 64;
                bullet_x[ii] += bvxapp;
                bullet_y[ii] += bvyapp;

                if (bullet_x[ii] > 0 && bullet_x[ii] < 320 && bullet_y[ii] > 0 && bullet_y[ii] < 180){
                    set(bullet_x[ii], bullet_y[ii], 0xFF);
                } else {
                    bullet_status[ii] = -1;
                }
            }
        }

    update_sch += 1;
            

    } //end of infinite game loop
}// end of main