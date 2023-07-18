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

#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080

struct illumination_profile illumination_profiles[] = {
    {
        .desc = "RGGB_A",
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .bits = 8,
        .bayer_pattern = BAYER_PATTERN_RGGB,
        .r  = 159,
        .g1 = 180,
        .g2 = 180,
        .b  =  84,
    },
    {
        .desc = "RGGB_F11",
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .bits = 8,
        .bayer_pattern = BAYER_PATTERN_RGGB,
        .r  = 166,
        .g1 = 214,
        .g2 = 214,
        .b  = 116,
    },
    {
        .desc = "RGGB_F2",
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .bits = 8,
        .bayer_pattern = BAYER_PATTERN_RGGB,
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
        .bayer_pattern = BAYER_PATTERN_RGGB,
        .r  = 167,
        .g1 = 228,
        .g2 = 228,
        .b  = 138,
    },
    {
        .desc = "RGGB_D65",
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .bits = 8,
        .bayer_pattern = BAYER_PATTERN_RGGB,
        .r  = 110,
        .g1 = 163,
        .g2 = 163,
        .b  = 109,
    },
};


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
            illumination_profiles[i].r  = (uint16_t)(illumination_profiles[i].r *16*scaleFactor);
            illumination_profiles[i].g1 = (uint16_t)(illumination_profiles[i].g1*16*scaleFactor);
            illumination_profiles[i].g2 = (uint16_t)(illumination_profiles[i].g2*16*scaleFactor);
            illumination_profiles[i].b  = (uint16_t)(illumination_profiles[i].b *16*scaleFactor);
        }

        outfile_size = illumination_profiles[i].width*illumination_profiles[i].height*2;

        image_buf = malloc(outfile_size);
        if(image_buf==NULL) {
            printf("Error allocate buffer with size = %ld\n", outfile_size);
            break;
        }
        if(fillBayerPattern(image_buf, &illumination_profiles[i])!=0) {
            printf("Error fill pattern\n");
        }
        
        sprintf(filename, "output_RAW12_%s.raw", illumination_profiles[i].desc);                
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