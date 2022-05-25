#ifndef I3LOCK_FANCY_RAPID_I3LOCK_FANCY_RAPID_C_H
#define I3LOCK_FANCY_RAPID_I3LOCK_FANCY_RAPID_C_H

void box_blur_h(unsigned char *dest, unsigned char *src, int height, int width, int radius);

void box_blur(unsigned char *dest, unsigned char *src, int height, int width, int radius, int times);

void pixelate(unsigned char *dest, unsigned char *src, int height, int width, int radius);

#endif //I3LOCK_FANCY_RAPID_I3LOCK_FANCY_RAPID_C_H
