#ifndef MASKGENERATOR_H
#define MASKGENERATOR_H
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <cmath>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

struct MaskImage {
    void *buf;
    int length;
    int width;
    int height;
    uint32_t pixelformat;
    int block_dim;
};

int maskgenerator_circle_twelve_blocks_generate(struct MaskImage *maskprofile, int width, int height, uint32_t pixfmt, double in_r_ratio);
int maskgenerator_circle_twelve_blocks_free(struct MaskImage *maskprofile);

#endif // MASKGENERATOR_H
