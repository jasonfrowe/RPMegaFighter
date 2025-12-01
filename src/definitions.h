/**
 * Misc left over definitions that should be moved to appropriate modules
 */

#include "input.h"
#include "constants.h"
#include <string.h>

// Text configs
#define NTEXT 1
static char score_message[6] = "SCORE ";
static char score_value[6] = "00000";
#define MESSAGE_WIDTH 36
#define MESSAGE_HEIGHT 2
#define MESSAGE_LENGTH (MESSAGE_WIDTH * MESSAGE_HEIGHT)
static char message[MESSAGE_LENGTH]; 
static char level_message[5] = "LEVEL";

// Extended Memory space for bitmap graphics (320x180 @ 8-bits)
const uint16_t vlen = 57600; 

// ============================================================================
// SINE/COSINE LOOKUP TABLES (24 steps for rotation)
// ============================================================================
// The lookup tables are defined in definitions.h:
// - sin_fix[25]: 255 * sin(theta) for 24 rotation angles (0-23) + wrap
// - cos_fix[25]: 255 * cos(theta) for 24 rotation angles (0-23) + wrap  
// - t2_fix4[25]: Affine transform offsets for sprite rotation
//
// These provide smooth rotation in 15-degree increments (360/24 = 15 degrees)
// Values are scaled by 255 for fixed-point math without floating point

// Pre-calulated Angles: 255*sin(theta)
const int16_t sin_fix[] = {
    0, 65, 127, 180, 220, 246, 255, 246, 220, 180, 127, 65, 
    0, -65, -127, -180, -220, -246, -255, -246, -220, -180, 
    -127, -65, 0
};

// Pre-calulated Angles: 255*cos(theta)
const int16_t cos_fix[] = {
    255, 246, 220, 180, 127, 65, 0, -65, -127, -180, -220, -246, 
    -255, -246, -220, -180, -127, -65, 0, 65, 127, 180, 
    220, 246, 255
};

// // Pre-calulated Affine offsets: 181*sin(theta - pi/4) + 127
// static const int16_t t2_fix8[] = {
//     0, 576, 1280, 2032, 2768, 3472, 4064, 4528, 4816, 4928, 
//     4816, 4528, 4064, 3472, 2768, 2032, 1280, 576, 0, -464, 
//     -752, -864, -752, -464, 0
// };

const int16_t t2_fix4[] = {
    0, 288, 640, 1016, 1384, 1736, 2032, 2264, 2408, 2464, 
    2408, 2264, 2032, 1736, 1384, 1016, 640, 288, 0, -232, 
    -376, -432, -376, -232, 0
};
