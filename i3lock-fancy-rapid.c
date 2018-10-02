#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <omp.h>
#include "lodepng/lodepng.h"

void gaussian_blur(unsigned char *dest, unsigned char *src, int height,
                   int width, int radius, double sigma)
{
    int kernel_width = radius * 2 + 1;
    double kernel[kernel_width * kernel_width];
    double scale2 = 2.0 * sigma * sigma;
    double scale1 = 1.0 / (scale2 * M_PI);
    for (int i = 0; i < kernel_width; ++i) {
        int dx = i - radius;
        for (int j = 0; j < kernel_width; ++j) {
            int dy = j - radius;
            kernel[kernel_width * i + j] = scale1 * exp(-(dx * dx + dy * dy)
                                                        / scale2);
        }
    }
#pragma omp parallel for
    for (int i = 0; i < height; ++i) {
        int iwidth = i * width;
        for (int j = 0; j < width; ++j) {
            int index = (iwidth + j) * 3;
            double r = 0.0;
            double g = 0.0;
            double b = 0.0;
            for (int i_k = 0; i_k < kernel_width; ++i_k) {
                int x = i + i_k - radius;
                if (x < 0 || x >= height)
                    continue;
                int xwidth = x * width;
                int i_kkernel_width = i_k * kernel_width;
                for (int j_k = 0; j_k < kernel_width; ++j_k) {
                    int y = j + j_k - radius;
                    if (y < 0 || y >= width)
                        continue;
                    int dindex = (xwidth + y) * 3;
                    int kindex = i_kkernel_width + j_k;
                    r += src[dindex] * kernel[kindex];
                    g += src[dindex + 1] * kernel[kindex];
                    b += src[dindex + 2] * kernel[kindex];
                }
            }
            dest[index] = r + 0.5;
            dest[index + 1] = g + 0.5;
            dest[index + 2] = b + 0.5;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s radius sigma\n", argv[0]);
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
    gaussian_blur(postblur, preblur, height, width, atoi(argv[1]),
                  atof(argv[2]));
    free(preblur);
    LodePNGState state;
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_RGB;
    state.encoder.zlibsettings.btype = 0;
    unsigned char *data;
    size_t data_len;
    lodepng_encode(&data, &data_len, postblur, width, height, &state);
    free(postblur);
    lodepng_state_cleanup(&state);
    char filename[] = "/tmp/tmp.XXXXXX.png";
    int fd = mkstemps(filename, 4);
    write(fd, data, data_len);
    free(data);
    close(fd);
    if (fork()) {
        int status;
        wait(&status);
        remove(filename);
        exit(WEXITSTATUS(status));
    } else {
        char *new_argv[] = {"i3lock", "-i", filename, NULL};
        execvp(new_argv[0], new_argv);
        exit(EXIT_FAILURE);
    }
}
