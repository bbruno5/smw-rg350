#include <SDL/SDL.h>
//#define Uint32 unsigned long
//#define Uint16 unsigned short
//#define Uint8  unsigned char

#define SCALE_FROM_160x120_8(to, from) \
  upscale4(to->pixels, (Uint8*)from->pixels, (Uint32*)from->format->palette->colors, from->format->palette->ncolors)

#define SCALE_FROM_640x480_16(to, from) \
  downscale((Uint32*)to->pixels, (Uint32*)from->pixels)


// Upscale from 160x120 16-bit to 320x120 16-bit
void upscale1(Uint32 *to, Uint32 *from);

// Upscale from 160x120 16-bit to 320x240 16-bit
void upscale2(Uint32 *to, Uint32 *from);

// Upscale from 160x120 8-bit to 320x240 16-bit
void upscale4(Uint32 *to, Uint8 *from, Uint32 *palette, unsigned int nb_colors);

// Downscale from 640x480 16-bit to 320x240 16-bit
void downscale(Uint32 *to, Uint32 *from);
