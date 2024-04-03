#include "maskgenerator.h"


static int maskgenerator_write_to_file(struct MaskImage *maskprofile, const char *filename)
{
    FILE *pFile;
    char fileprop[512];

    if(maskprofile == nullptr) {
        printf("Error: maskprofile can not be null\n");
        return -1;
    }
    if(maskprofile->length<=0) {
        printf("buffer length must be greater than 0\n");
        return -1;
    }
    if(maskprofile->buf == nullptr) {
        printf("Error: target image buffer is null\n");
        return -1;
    }

    snprintf(fileprop, sizeof(fileprop), "ouput_%dx%d__%s", maskprofile->width, maskprofile->height, filename);
    pFile = fopen(fileprop, "wb");
    if(pFile == nullptr) {
        printf("Error: Can not open file to write : %s\n", filename);
        return -1;
    }

    fwrite (maskprofile->buf, maskprofile->length, 1, pFile);
    fflush(pFile);
    fclose(pFile);
    return 0;
}

/*
 *
 *    ----------------------------------
 *    |          |          |          |
 *    |     1    |     0    |     5    |
 *    |          |          |          |
 *    ----------------------------------
 *    |          |          |          |
 *    |     2    |     3    |     4    |
 *    |          |          |          |
 *    ----------------------------------
 *    |          |          |          |
 *    |     7    |     8    |     9    |
 *    |          |          |          |
 *    ----------------------------------
 *    |          |          |          |
 *    |     6    |    11    |    10    |
 *    |          |          |          |
 *    ----------------------------------
 *
 */
static int maskgenerator_circle_twelve_blocks_fill_yuv(struct MaskImage *maskprofile, double in_r_ratio)
{
    unsigned char *buf = (unsigned char *)maskprofile->buf;
    int buflen         = maskprofile->length;
    int owidth     = maskprofile->width;
    int oheight    = maskprofile->height;
    int oblock_dim = maskprofile->block_dim;

    int x, y;
    int tX_off, tY_off;
    int bytes_per_pixel=2;
    int lof, pof;

    double out_radius = oblock_dim;
    double in_radius  = in_r_ratio * out_radius;
    double Ox       = (double)owidth/2;
    double Oy       = (double)oheight/2;
    double mX, mY;

    double angle;
    double radius;
    double first_angle    = (0*60)*M_PI/180.0;
    double second_angle   = (1*60)*M_PI/180.0;
    double third_angle    = (2*60)*M_PI/180.0;
    double fourth_angle   = (3*60)*M_PI/180.0;
    double fifth_angle    = (4*60)*M_PI/180.0;
    double sixth_angle    = (5*60)*M_PI/180.0;
    double seventh_angle  = (6*60)*M_PI/180.0;

    for(y=0;y<oheight;y++)
    {
        for(x=0;x<owidth;x++)
        {
            mX =  1*((double)x - Ox);
            mY = -1*((double)y - Oy); //Revert Y
            /*
             * dot = x1*x2 + y1*y2
             * det = x1*y2 - y1*x2
             * angle = atan2(det, dot);
             * angle_0to2pi = atan2(-det, -dot) + pi;
             * handle the negative angle to range [0 2pi]
             */
            angle  = atan2(mY, mX);
            if(angle<0) {
                angle = angle+2*M_PI;
            }
            radius = sqrt(mX*mX+mY*mY);

            /*
             * Generate block number 4
             */
            if((radius<=in_radius) &&
               (angle>=first_angle) &&
               (angle<second_angle))
            {
                tX_off = oblock_dim/2;
                tY_off = 0;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }

            /*
             * Generate block number 5
             */
            if((radius>in_radius)&&
                (radius<=out_radius) &&
                (angle>=first_angle) &&
                (angle<second_angle))
            {
                tX_off = oblock_dim/2;
                tY_off = -1*oblock_dim;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }


            /*
             * Generate block number 3
             */
            if((radius<=in_radius) &&
                (angle>=second_angle) &&
                (angle<third_angle))
            {
                tX_off = 0;
                tY_off = 0;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }

            /*
             * Generate block number 0
             */
            if((radius>in_radius)&&
                (radius<=out_radius) &&
                (angle>=second_angle) &&
                (angle<third_angle))
            {
                tX_off = 0;
                tY_off = -1*oblock_dim;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }


            /*
             * Generate block number 2
             */
            if((radius<=in_radius) &&
                (angle>=third_angle) &&
                (angle<fourth_angle))
            {
                tX_off = -1*(oblock_dim/2);
                tY_off = 0;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }


            /*
             * Generate block number 1
             */
            if((radius>in_radius)&&
                (radius<=out_radius) &&
                (angle>=third_angle) &&
                (angle<fourth_angle))
            {
                tX_off = -1*(oblock_dim/2);
                tY_off = -1*oblock_dim;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }


            /*
             * Generate block number 7
             */
            if((radius<=in_radius) &&
                (angle>=fourth_angle) &&
                (angle<fifth_angle))
            {
                tX_off = -1*(oblock_dim/2);
                tY_off = 0;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }

            /*
             * Generate block number 6
             */
            if((radius>in_radius)&&
                (radius<=out_radius) &&
                (angle>=fourth_angle) &&
                (angle<fifth_angle))
            {
                tX_off = -1*(oblock_dim/2);
                tY_off =  1*oblock_dim;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }


            /*
             * Generate block number 8
             */
            if((radius<=in_radius) &&
                (angle>=fifth_angle) &&
                (angle<sixth_angle))
            {
                tX_off = 0;
                tY_off = 0;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }

            /*
             * Generate block number 11
             */
            if((radius>in_radius)&&
                (radius<=out_radius) &&
                (angle>=fifth_angle) &&
                (angle<sixth_angle))
            {
                tX_off = 0;
                tY_off = oblock_dim;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }


            /*
             * Generate block number 9
             */
            if((radius<=in_radius) &&
                (angle>=sixth_angle) &&
                (angle<seventh_angle))
            {
                tX_off = oblock_dim/2;
                tY_off = 0;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }

            /*
             * Generate block number 10
             */
            if((radius>in_radius)&&
                (radius<=out_radius) &&
                (angle>=sixth_angle) &&
                (angle<seventh_angle))
            {
                tX_off = oblock_dim/2;
                tY_off = oblock_dim;
                lof = (y+tY_off)*owidth*bytes_per_pixel;
                pof = (x+tX_off)*bytes_per_pixel;
                if((lof+pof)<buflen)
                {
                    /*Y element*/
                    *(buf+lof+pof+0) = 0xff;
                    /*UV element*/
                    *(buf+lof+pof+1) = 0;
                } else {
                    printf("buff length over: %d, %d\n", buflen, lof+pof);
                }
            }

        }
    }

    maskgenerator_write_to_file(maskprofile, "image_out_test.yuv");
    return 0;
}

int maskgenerator_circle_twelve_blocks_generate(struct MaskImage *maskprofile, int width, int height, uint32_t pixfmt, double in_r_ratio)
{
    unsigned char *buf;
    int owidth, oheight;
    int obuf_size;
    int imin_dimension   = height < width ? height : width;
    int oblock_dimension = imin_dimension/2;

    if(maskprofile==nullptr) {
        printf("maskprofile is null\n");
        return -1;
    }
    if(oblock_dimension<4) {
        printf("Error: invalid image dimensions: %dx%d\n", width, height);
    }
    owidth  = oblock_dimension*3;
    oheight = oblock_dimension*4;
    switch (pixfmt) {
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
        obuf_size = owidth*oheight*2;
        break;
    default:
        obuf_size = owidth*oheight*2;
        break;
    }
    buf = (unsigned char *)malloc(obuf_size);
    memset(buf, 0, obuf_size);

    maskprofile->buf = buf;
    maskprofile->length = obuf_size;
    maskprofile->width  = owidth;
    maskprofile->height = oheight;
    maskprofile->pixelformat = pixfmt;
    maskprofile->block_dim = oblock_dimension;

    maskgenerator_circle_twelve_blocks_fill_yuv(maskprofile, in_r_ratio);

    return 0;
}

int maskgenerator_circle_twelve_blocks_free(struct MaskImage *maskprofile)
{
    if(maskprofile==nullptr) {
        printf("maskprofile is null\n");
        return -1;
    }
    free(maskprofile->buf);
    return 0;
}
