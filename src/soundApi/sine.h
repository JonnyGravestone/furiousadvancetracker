#ifndef _SOUND_API_SINELUT_
#define _SOUND_API_SINELUT_

#define SIN_LUT_SIZE 256
#define SIN_PHASE_MAX 255

const u8 sinLUT[SIN_LUT_SIZE] = {
    32,32,33,34,35,35,36,37,
    38,38,39,40,41,41,42,43,
    44,44,45,46,46,47,48,48,
    49,50,50,51,51,52,53,53,
    54,54,55,55,56,56,57,57,
    58,58,59,59,59,60,60,60,
    61,61,61,61,62,62,62,62,
    62,63,63,63,63,63,63,63,
    63,63,63,63,63,63,63,63,
    62,62,62,62,62,61,61,61,
    61,60,60,60,59,59,59,58,
    58,57,57,56,56,55,55,54,
    54,53,53,52,51,51,50,50,
    49,48,48,47,46,46,45,44,
    44,43,42,41,41,40,39,38,
    38,37,36,35,35,34,33,32,
    32,31,30,29,28,28,27,26,
    25,25,24,23,22,22,21,20,
    19,19,18,17,17,16,15,15,
    14,13,13,12,12,11,10,10,
    9,9,8,8,7,7,6,6,
    5,5,4,4,4,3,3,3,
    2,2,2,2,1,1,1,1,
    1,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    1,1,1,1,1,2,2,2,
    2,3,3,3,4,4,4,5,
    5,6,6,7,7,8,8,9,
    9,10,10,11,12,12,13,13,
    14,15,15,16,17,17,18,19,
    19,20,21,22,22,23,24,25,
    25,26,27,28,28,29,30,31
};

#endif