/* 
 * File:   oscillator.h
 * Author: cyril
 *
 * Created on 15 décembre 2011, 11:09
 */

#ifndef OSCILLATOR_H
#define	OSCILLATOR_H

s32 sinTableSize = 101;
u8 sinLUT[101] = {0x00, 0x08, 0x10, 0x17, 0x1F, 0x27, 0x2F, 0x36, 0x3D, 0x44, 0x4B, 0x51, 0x57, 0x5D, 0x62, 0x67, 0x6B, 0x70, 0x73, 0x76,
    0x79, 0x7B, 0x7D, 0x7E, 0x7F, 0x7F, 0x7F, 0x7F, 0x7D, 0x7C, 0x79, 0x77, 0x74, 0x70, 0x6C, 0x67, 0x63, 0x5D, 0x58, 0x52,
    0x4B, 0x45, 0x3E, 0x37, 0x2F, 0x28, 0x20, 0x18, 0x10, 0x08, 0x00, 0xF8, 0xF0, 0xE8, 0xE1, 0xD9, 0xD1, 0xCA, 0xC3, 0xBC,
    0xB5, 0xAF, 0xA9, 0xA3, 0x9E, 0x99, 0x94, 0x90, 0x8C, 0x89, 0x86, 0x84, 0x82, 0x81, 0x80, 0x80, 0x80, 0x80, 0x82, 0x83,
    0x85, 0x88, 0x8B, 0x8F, 0x93, 0x97, 0x9C, 0xA1, 0xA7, 0xAD, 0xB3, 0xBA, 0xC0, 0xC7, 0xCF, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6,
    0xFE};

typedef struct OSCILLATOR {
    s32 phase;
    s32 increment;
    u16 frequency;
    u8 amplitude;
} oscillator;

oscillator oscillators;

oscillator * snd_initOscillator(s32 frequency) {
    oscillators.phase = 0;
    oscillators.amplitude = 0;
    oscillators.frequency = frequency;
    //FIXED 20.12
    oscillators.increment = 1;//FIXED_FROMFLOAT(((float) ((frequency * sinTableSize) / 16384)));
    //oscillators.increment = FIXED_FROMFLOAT(1.0); //FIXED_FROMFLOAT(0.5F);
    //hel_BgTextPrintF(1, 15, 13, 0, "INCREMENT %x", firstOsc.increment);
    return &oscillators;
};

inline u8 snd_playOscillator(oscillator * osc) {
    u8 sinValue = sinLUT [osc->phase ];
    osc->phase = (osc->phase + osc->increment)%101;
    
    //    if (FIXED_TOINT(osc->phase) >= 524287 - FIXED_TOINT(osc->increment) || osc->phase >= FIXED_FROMINT(sinTableSize)) {
    //        osc->phase = 0; // osc->phase - FIXED_FROMINT(sinTableSize);
    //    } else {
    //    }
    //    if (checkForOvflow & 0x80000000) {
    //        osc->phase = osc->phase - FIXED_FROMINT(sinTableSize);
    //    } else {

    //osc->phase = (osc->phase + osc->increment) % FIXED_FROMINT(sinTableSize);
    //        if (osc->phase >= FIXED_FROMINT(sinTableSize)) {
    //            osc->phase = osc->phase - FIXED_FROMINT(sinTableSize);
    //        }
    //}
    return sinValue;
}

#endif	/* OSCILLATOR_H */

