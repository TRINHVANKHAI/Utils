#include <iostream>
#include <arm_neon.h>
#include <cstring>

using namespace std;
#define FRACBIT    12
#define LSHIFTBIT (15-FRACBIT)
#define IMAGE_WIDTH  1920
#define IMAGE_HEIGHT 1080

/*
 * This function apply uGain, and vGain to the source buffer, then save the result to destination buffer
 * vdupq_n_s16: set gains vector, the FRACBIT gives the precision to the gain factor, hence we use 12 bit
 *              for the fraction bits
 * vqrdmulhq_s16 : Compute vector multiplication and right shift 15 bit, for that reason, we need to make
 *                 sure that LSHIFTBIT + FRACBIT = 15 to get correct result
 */
int imageApplyUVGainsNV12(uint8_t *pSrcBuf, uint8_t *pDstBuf, int width, int height, float uGain, float vGain)
{
    size_t i;
    size_t imageBufSize = width*height/2;
    size_t uvOffset     = width*height;
    int16x8_t uGains = vdupq_n_s16(uGain*(1<<FRACBIT));
    int16x8_t vGains = vdupq_n_s16(vGain*(1<<FRACBIT));
    uint8x8x2_t pixData;
    int16x8x2_t tmpData;
    uint8_t *pUvSrcBuf = pSrcBuf+uvOffset;
    uint8_t *pUvDstBuf = pDstBuf+uvOffset;
    for(i=0; i<imageBufSize; i+=16)
    {
        pixData = vld2_u8(pUvSrcBuf+i);
        tmpData.val[0] = (int16x8_t)vshll_n_u8(pixData.val[0], LSHIFTBIT);
        tmpData.val[1] = (int16x8_t)vshll_n_u8(pixData.val[1], LSHIFTBIT);
        tmpData.val[0] = vqrdmulhq_s16(tmpData.val[0], uGains);
        tmpData.val[1] = vqrdmulhq_s16(tmpData.val[1], vGains);
        pixData.val[0] = vqmovun_s16(tmpData.val[0]);
        pixData.val[1] = vqmovun_s16(tmpData.val[1]);
        vst2_u8(pUvDstBuf+i, pixData);
    }

    return 0;
}

int main(int argc, char* argv[])
{

    float uGain;
    float vGain;
    int i;
    int ret;
    struct timespec timeStart, timeEnd;
    long int timeDiff;
    FILE *pSrcImageFile;
    FILE *pDstImageFile;
    const char *srcImageFileName = "/opt/neonTest/source_image_1920x1080_nv12.yuv";
    const char *dstImageFileName = "/opt/neonTest/result_image_1920x1080_nv12.yuv";
    size_t imageSize = IMAGE_WIDTH*IMAGE_HEIGHT*3/2;
    uint8_t *pSrcImage = new uint8_t[imageSize];
    uint8_t *pDstImage = new uint8_t[imageSize];


    if(argc!=3)
    {
        printf("USAGE: %s <uGain> <vGain>\n", argv[0]);
        delete []pSrcImage;
        delete []pDstImage;
        return -1;
    }

    uGain = atof(argv[1]);
    vGain = atof(argv[2]);

    pSrcImageFile = fopen(srcImageFileName, "rb");
    if(pSrcImageFile == nullptr)
    {
        delete []pSrcImage;
        delete []pDstImage;
        printf("No such file: %s\n", srcImageFileName);
        return -1;
    }

    fseek(pSrcImageFile, 0, SEEK_END);
    ret = ftell(pSrcImageFile);
    if (ret < (int)imageSize)
    {
        delete []pSrcImage;
        delete []pDstImage;
        fclose(pSrcImageFile);
        printf("Image file size check failed, size=%d < %ld\n", ret, imageSize);
        return -1;
    }

    fseek(pSrcImageFile, 0, SEEK_SET);
    fread(pSrcImage, imageSize, 1, pSrcImageFile);
    fclose(pSrcImageFile);

    memcpy(pDstImage, pSrcImage, imageSize);
    clock_gettime(CLOCK_MONOTONIC, &timeStart);
    for(i=0;i<60;i++)
    {
        imageApplyUVGainsNV12(pSrcImage, pDstImage, IMAGE_WIDTH, IMAGE_HEIGHT, uGain, vGain);
    }
    clock_gettime(CLOCK_MONOTONIC, &timeEnd);



    pDstImageFile = fopen(dstImageFileName, "wb");
    fwrite(pDstImage, imageSize, 1, pDstImageFile);
    fclose(pDstImageFile);




    timeDiff = (timeEnd.tv_sec*1000000000L + timeEnd.tv_nsec) - (timeStart.tv_sec*1000000000L + timeStart.tv_nsec);
    printf("Computing Time per frame=%ldus\n", timeDiff/60/1000);
    delete []pSrcImage;
    delete []pDstImage;
    return 0;
}




