/*
 * BSD 3-Clause License
 * 
 * Copyright (c) 2018-2019, The i3lock-fancy-rapid authors
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <omp.h>
#include <string.h>

void box_blur_h(unsigned char *dest, unsigned char *src, int height, int width,
                int radius)
{
    double coeff = 1.0 / (radius * 2 + 1);
#pragma omp parallel for
    for (int i = 0; i < height; ++i) {
        int iwidth = i * width;
        double r_acc = 0.0;
        double g_acc = 0.0;
        double b_acc = 0.0;
        for (int j = -radius; j < width; ++j) {
            if (j - radius - 1 >= 0) {
                int index = (iwidth + j - radius - 1) * 3;
                r_acc -= coeff * src[index];
                g_acc -= coeff * src[index + 1];
                b_acc -= coeff * src[index + 2];
            }
            if (j + radius < width) {
                int index = (iwidth + j + radius) * 3;
                r_acc += coeff * src[index];
                g_acc += coeff * src[index + 1];
                b_acc += coeff * src[index + 2];
            }
            if (j < 0)
                continue;
            int index = (iwidth + j) * 3;
            dest[index] = r_acc + 0.5;
            dest[index + 1] = g_acc + 0.5;
            dest[index + 2] = b_acc + 0.5;
        }
    }
}

static inline void transpose(unsigned char *dest, unsigned char *src, int height, int width) {
    for (int i = 0; i < height; ++i) {
        int iwidth = i * width;
        for (int j = 0; j < width; ++j) {
            int nIndex = 3 * (iwidth + j);
            int tIndex = 3 * (j * height + i);
            dest[tIndex] = src[nIndex];
            dest[tIndex+1] = src[nIndex+1];
            dest[tIndex+2] = src[nIndex+2];
        }
    }
}

void box_blur(unsigned char *dest, unsigned char *src, int height, int width,
              int radius, int times)
{
    for (int i = 0; i < times; ++i) {
        box_blur_h(dest, src, height, width, radius);
        memcpy(src, dest, height * width * 3);
    }
    transpose(src, dest, height, width);
    for (int i = 0; i < times; ++i) {
        box_blur_h(dest, src, width, height, radius);
        memcpy(src, dest, height * width * 3);
    }
    transpose(dest, src, width, height);
}

void pixelate(unsigned char *dest, unsigned char *src, int height,
                   int width, int radius)
{
    radius = radius * 2 + 1;
#pragma omp parallel for
    for (int i = 0; i < height; i += radius) {
        for (int j = 0; j < width; j += radius) {
            int amount = 0;
            int r = 0;
            int g = 0;
            int b = 0;

            for (int k = 0; k < radius; ++k) {
                if (i + k >= height)
                    break;

                for (int l = 0; l < radius; ++l) {
                    if (j + l >= width)
                        break;

                    ++amount;
                    int index = ((i + k) * width + (j + l)) * 3;
                    r += src[index];
                    g += src[index + 1];
                    b += src[index + 2];
                }
            }

            r /= amount;
            g /= amount;
            b /= amount;

            for (int k = 0; k < radius; ++k) {
                if (i + k >= height)
                    break;

                for (int l = 0; l < radius; ++l) {
                    if (j + l >= width)
                        break;

                    int index = ((i + k) * width + (j + l)) * 3;
                    dest[index] = r;
                    dest[index + 1] = g;
                    dest[index + 2] = b;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr,
               "usage: %s radius times [OPTIONS]\n"
               "pass \"pixel\" for times to get pixelation\n",
               argv[0]);
        exit(EXIT_FAILURE);
    }

    Display *display = XOpenDisplay(NULL);
    Window root = XDefaultRootWindow(display);
    XWindowAttributes gwa;
    XGetWindowAttributes(display, root, &gwa);
    int height = gwa.height;
    int width = gwa.width;
    unsigned char *preblur = malloc(height * width * 3);
    XImage *image = XGetImage(display, root, 0, 0, width, height, AllPlanes,
                              ZPixmap);
    for (int i = 0; i < height; ++i) {
        int iwidth = i * width;
        for (int j = 0; j < width; ++j) {
            int index = (iwidth + j) * 3;
            unsigned long pixel = XGetPixel(image, j, i);
            preblur[index] = (pixel & image->red_mask) >> 16;
            preblur[index + 1] = (pixel & image->green_mask) >> 8;
            preblur[index + 2] = pixel & image->blue_mask;
        }
    }
    XDestroyImage(image);
    XDestroyWindow(display, root);
    XCloseDisplay(display);

    unsigned char *postblur = malloc(height * width * 3);
    int radius = atoi(argv[1]);
    if (radius < 0) {
        fprintf(stderr, "Radius has to be non-negative!\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[2], "pixel") == 0) {
        pixelate(postblur, preblur, height, width, radius);
    } else {
        int times = atoi(argv[2]);
        if (times < 0) {
            fprintf(stderr, "Times has to be non-negative!\n");
            exit(EXIT_FAILURE);
        }
        box_blur(postblur, preblur, height, width, radius, times);
    }
    free(preblur);

    int fds[2];
    pipe(fds);
    if (fork()) {
        write(fds[1], postblur, height * width * 3);
        int status;
        wait(&status);
        exit(WEXITSTATUS(status));
    } else {
        dup2(fds[0], STDIN_FILENO);
        char fmt[32];
        snprintf(fmt, sizeof(fmt), "%ix%i:rgb", width, height);
        char *new_argv[argc + 3];
        new_argv[0] = "i3lock";
        new_argv[1] = "-i";
        new_argv[2] = "/dev/stdin";
        new_argv[3] = "--raw";
        new_argv[4] = fmt;
        for (int i = 3; i < argc; ++i)
            new_argv[i + 2] = argv[i];
        new_argv[argc + 2] = NULL;
        execvp(new_argv[0], new_argv);
        exit(EXIT_FAILURE);
    }
}
