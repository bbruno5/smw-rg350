#include "scaler.h"

// Upscale from 160x120 16-bit to 320x120 16-bit
void upscale1(Uint32 *to, Uint32 *from) {
    Uint32 tmp;
    int i;
    for (i=0; i<160*120/2; i++) {
        tmp = from[i];
        *to++ = (tmp & 0xffff) | (tmp << 16);
        *to++ = (tmp & 0xffff0000) | (tmp >> 16);
    }
}


// Upscale from 160x120 16-bit to 320x240 16-bit
void upscale2(Uint32 *to, Uint32 *from) {
    Uint32 tmp, pixel;
    int i=0, endline;

    do {
        endline = 160+i;

        do {
            tmp = from[i++];
            pixel = (tmp & 0xffff) | (tmp << 16);
            *(to + 320/2) = pixel;
            *to++ = pixel;
            pixel = (tmp & 0xffff0000) | (tmp >> 16);
            *(to + 320/2) = pixel;
            *to++ = pixel;
        } while(i < endline);
        to += 320/2;

    } while(i<160*120/2);
}


/*
// Upscale from 160x120 8-bit to 320x240 16-bit
void upscale3(Uint32 *to, Uint8 *from, Uint32 *palette) {
    Uint32 pixel;
    Uint8 *color;
    int i=0, endline;

    do {
        endline = 160+i;

        do {
            color = (Uint8*)palette + from[i++] + 2;
            pixel = (Uint32)(*color--) >> 3;
            pixel = ((pixel << 6) | (Uint32)(*color--) >> 2);
            pixel = ((pixel << 5) | (Uint32)(*color) >> 3);
            pixel|= pixel << 16;
            *(to + 320) = pixel;
            *to++ = pixel;
        } while(i < endline);
        to += 320;

    } while(i<160*120/2);
}
*/


// Convert a color to 16-bit color
static inline Uint16 to_16bits(Uint32 color24bits) {
//    return (color24bits >> 19) | ((color24bits & 0xfc00) >> (10 - 5)) | ((color24bits & 0xf8) << (11 - 3));
    return ((color24bits & 0xf80000) >> 19) | ((color24bits & 0xfc00) >> (10 - 5)) | ((color24bits & 0xf8) << (11 - 3));
}


// Convert colors 24-bit palette to 16-bit palette
static inline void convert_palette(Uint16 *to, Uint32 *from, unsigned int nb) {
    while(nb--)
      *to++ = to_16bits(*from++);
}


// Upscale from 160x120 8-bit to 320x240 16-bit (new one)
void upscale4(Uint32 *to, Uint8 *from, Uint32 *palette, unsigned int nb_colors) {
    Uint32 pixel;
    int i=0, endline;

    Uint16 new_palette[nb_colors];
    convert_palette(new_palette, palette, nb_colors);

    do {
        endline = 160+i;

        do {
            pixel = (Uint32)new_palette[from[i++]];
            pixel |= pixel << 16;
            *(to + 320/2) = pixel;
            *to++ = pixel;
        } while(i < endline);
        to += 320/2;

    } while(i<160*120);
}


// Downscale from 640x480 16-bit to 320x240 16-bit
void downscale(Uint32 *to, Uint32 *from) {
    Uint32 first, second;
    unsigned int i, j;

    for (i=0; i<480/2; i++) {
        for (j=0; j<640/4; j++) {
            first = from[0];
            second = from[1];
            *to++ = (first & 0xffff) | (second << 16);
            from += 2;
        }
        from += 320;
    }
}
