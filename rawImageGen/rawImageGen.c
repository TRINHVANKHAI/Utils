/*
 * Author: TRINH VAN KHAI mrkhai@live.com
 * Description: This code use for synthesizing bayer pattern based on
 * the input value
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef enum _bayer_pattern {
    BAYER_PATTERN_BGGR,
    BAYER_PATTERN_GBRG,
    BAYER_PATTERN_GRBG,
    BAYER_PATTERN_RGGB
} bayer_pattern_t;

struct illumination_profile {
    const char *desc;
    int width;
    int height;
    int bits;
    bayer_pattern_t bayer_pattern;
    uint16_t r;
    uint16_t g1;
    uint16_t g2;
    uint16_t b;
};


#define IMAGE_WIDTH 3840
#define IMAGE_HEIGHT 2160
#define BAYER_PATTERN BAYER_PATTERN_GBRG
#define OUTPUT_BITS 10

struct illumination_profile illumination_profiles[] = {
    {
        .desc = "RGGB_A",
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .bits = 8,
        .bayer_pattern = BAYER_PATTERN,
        .r  = 150,
        .g1 = 166,
        .g2 = 166,
        .b  =  51,
    },
    {
        .desc = "RGGB_F11",
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .bits = 8,
        .bayer_pattern = BAYER_PATTERN,
        .r  = 136,
        .g1 = 176,
        .g2 = 176,
        .b  = 64,
    },
    {
        .desc = "RGGB_F2",
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .bits = 8,
        .bayer_pattern = BAYER_PATTERN,
        .r  = 174,
        .g1 = 218,
        .g2 = 218,
        .b  = 120,
    },
    {
        .desc = "RGGB_D50",
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .bits = 8,
        .bayer_pattern = BAYER_PATTERN,
        .r  = 89,
        .g1 = 140,
        .g2 = 140,
        .b  = 71,
    },
    {
        .desc = "RGGB_D65",
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .bits = 8,
        .bayer_pattern = BAYER_PATTERN,
        .r  = 88,
        .g1 = 155,
        .g2 = 155,
        .b  = 84,
    },
};

/*
*
*/

int fillBayerPattern(uint16_t *pbuf, struct illumination_profile *iProfile) {
    int x,y;
    int lof;
    int ret;
 
    switch (iProfile->bayer_pattern) {

        case BAYER_PATTERN_RGGB: {
            /* RGRG: even lines */
            for(y=0; y<iProfile->height; y+=2) {
                lof = y*iProfile->width;
                for(x=0; x<iProfile->width; x+=2) {
                    *(pbuf+lof+x+0) = iProfile->r;
                    *(pbuf+lof+x+1) = iProfile->g1;
                }
            }
            /* GBGB: odd lines */
            for(y=1; y<iProfile->height; y+=2) {
                lof = y*iProfile->width;
                for(x=0; x<iProfile->width; x+=2) {
                    *(pbuf+lof+x+0) = iProfile->g2;
                    *(pbuf+lof+x+1) = iProfile->b;
                }
            }
            ret = 0;
            break;
        }
        
        case BAYER_PATTERN_BGGR:{
            /* BGBG: even lines */
            for(y=0; y<iProfile->height; y+=2) {
                lof = y*iProfile->width;
                for(x=0; x<iProfile->width; x+=2) {
                    *(pbuf+lof+x+0) = iProfile->b;
                    *(pbuf+lof+x+1) = iProfile->g1;
                }
            }
            /* GRGR: odd lines */
            for(y=1; y<iProfile->height; y+=2) {
                lof = y*iProfile->width;
                for(x=0; x<iProfile->width; x+=2) {
                    *(pbuf+lof+x+0) = iProfile->g2;
                    *(pbuf+lof+x+1) = iProfile->r;
                }
            }
            ret = 0;
            break;
        }
        
        case BAYER_PATTERN_GBRG:{
            /* GBGB: even lines */
            for(y=0; y<iProfile->height; y+=2) {
                lof = y*iProfile->width;
                for(x=0; x<iProfile->width; x+=2) {
                    *(pbuf+lof+x+0) = iProfile->g1;
                    *(pbuf+lof+x+1) = iProfile->b;
                }
            }
            /* RGRG: odd lines */
            for(y=1; y<iProfile->height; y+=2) {
                lof = y*iProfile->width;
                for(x=0; x<iProfile->width; x+=2) {
                    *(pbuf+lof+x+0) = iProfile->r;
                    *(pbuf+lof+x+1) = iProfile->g2;
                }
            }
            ret = 0;
            break;
        }
        
        case BAYER_PATTERN_GRBG: {
            /* GRGR: even lines */
            for(y=0; y<iProfile->height; y+=2) {
                lof = y*iProfile->width;
                for(x=0; x<iProfile->width; x+=2) {
                    *(pbuf+lof+x+0) = iProfile->g1;
                    *(pbuf+lof+x+1) = iProfile->r;
                }
            }
            /* BGBG: odd lines */
            for(y=1; y<iProfile->height; y+=2) {
                lof = y*iProfile->width;
                for(x=0; x<iProfile->width; x+=2) {
                    *(pbuf+lof+x+0) = iProfile->b;
                    *(pbuf+lof+x+1) = iProfile->g2;
                }
            }
            ret = 0;
            break;
        }
        
        
        default: {
            fprintf(stderr, "Specified bayer pattern does not supported\n");
            ret = -1;
            break;
        }
    }
    return ret;
}


int main(int argc, char* argv[]) {
    int i;
    char filename[512];
    double scaleFactor;
    uint16_t maxValue;
    unsigned char bitsScale = OUTPUT_BITS > illumination_profiles[i].bits ? 
                              OUTPUT_BITS - illumination_profiles[i].bits : 0;
    unsigned char bytesPerPixel = (OUTPUT_BITS % 8 == 0) ?
                                   OUTPUT_BITS/8 : OUTPUT_BITS/8+1;
    FILE *outfile;
    uint16_t *image_buf;
    ssize_t outfile_size;
    for(i=0; i<5; i++) {
        if (illumination_profiles[i].bits == 8) {
            /*
            Normalize to max of 210 and scale to 12bits only for current 8bits setting
            If the illumination_profiles already in 12 bits representation, this step 
            must be skipped
            */
            maxValue = illumination_profiles[i].g1 > illumination_profiles[i].g2 ?
                       illumination_profiles[i].g1 : illumination_profiles[i].g2;
            if(maxValue==0) maxValue = 210;
            scaleFactor = 210/(double)maxValue;
            illumination_profiles[i].r  = (uint16_t)(illumination_profiles[i].r *(1<<bitsScale)*scaleFactor);
            illumination_profiles[i].g1 = (uint16_t)(illumination_profiles[i].g1*(1<<bitsScale)*scaleFactor);
            illumination_profiles[i].g2 = (uint16_t)(illumination_profiles[i].g2*(1<<bitsScale)*scaleFactor);
            illumination_profiles[i].b  = (uint16_t)(illumination_profiles[i].b *(1<<bitsScale)*scaleFactor);
        }

        outfile_size = illumination_profiles[i].width*illumination_profiles[i].height*bytesPerPixel;

        image_buf = malloc(outfile_size);
        if(image_buf==NULL) {
            printf("Error allocate buffer with size = %ld\n", outfile_size);
            break;
        }
        if(fillBayerPattern(image_buf, &illumination_profiles[i])!=0) {
            printf("Error fill pattern\n");
        }
        
        sprintf(filename, "output_%dx%d_RAW%d_%s.raw",
        illumination_profiles[i].width,
        illumination_profiles[i].height,
        OUTPUT_BITS,
        illumination_profiles[i].desc);
        
        outfile = fopen(filename, "wb");
        fwrite(image_buf, outfile_size, 1, outfile);
        fflush(outfile);
        fclose(outfile);
        free(image_buf);
        printf("Generated file: %s [r=%d, g1=%d, g2=%d, b=%d]\n",
                filename,
                illumination_profiles[i].r,
                illumination_profiles[i].g1,
                illumination_profiles[i].g2,
                illumination_profiles[i].b
                );
    }
    
    
    return 0;
}
