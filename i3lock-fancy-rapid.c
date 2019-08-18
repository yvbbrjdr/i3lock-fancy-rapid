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

void box_blur_v(unsigned char *dest, unsigned char *src, int height, int width,
                int radius)
{
    double coeff = 1.0 / (radius * 2 + 1);
#pragma omp parallel for
    for (int j = 0; j < width; ++j) {
        double r_acc = 0.0;
        double g_acc = 0.0;
        double b_acc = 0.0;
        for (int i = -radius; i < height; ++i) {
            if (i - radius - 1 >= 0) {
                int index = ((i - radius - 1) * width + j) * 3;
                r_acc -= coeff * src[index];
                g_acc -= coeff * src[index + 1];
                b_acc -= coeff * src[index + 2];
            }
            if (i + radius < height) {
                int index = ((i + radius) * width + j) * 3;
                r_acc += coeff * src[index];
                g_acc += coeff * src[index + 1];
                b_acc += coeff * src[index + 2];
            }
            if (i < 0)
                continue;
            int index = (i * width + j) * 3;
            dest[index] = r_acc + 0.5;
            dest[index + 1] = g_acc + 0.5;
            dest[index + 2] = b_acc + 0.5;
        }
    }
}

void box_blur_once(unsigned char *dest, unsigned char *src, int height,
                   int width, int radius)
{
    unsigned char *tmp = malloc(height * width * 3);
    box_blur_h(tmp, src, height, width, radius);
    box_blur_v(dest, tmp, height, width, radius);
    free(tmp);
}

void box_blur(unsigned char *dest, unsigned char *src, int height, int width,
              int radius, int times)
{
    box_blur_once(dest, src, height, width, radius);
    for (int i = 0; i < times - 1; ++i) {
        memcpy(src, dest, height * width * 3);
        box_blur_once(dest, src, height, width, radius);
    }

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
    if (strcmp(argv[2], "pixel") == 0) {
        pixelate(postblur, preblur, height, width, atoi(argv[1]));
    } else {
        box_blur(postblur, preblur, height, width, atoi(argv[1]), atoi(argv[2]));
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
