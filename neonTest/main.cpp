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


int imageApplyUVGainsNV12x2(uint8_t *pSrcBuf, uint8_t *pDstBuf, int width, int height, float uGain, float vGain)
{
    size_t i;
    size_t imageBufSize = width*height/2;
    size_t uvOffset     = width*height;
    int16x8_t uGains = vdupq_n_s16(uGain*(1<<FRACBIT));
    int16x8_t vGains = vdupq_n_s16(vGain*(1<<FRACBIT));
    uint8x8x4_t pixData;
    int16x8x4_t tmpData;
    uint8_t *pUvSrcBuf = pSrcBuf+uvOffset;
    uint8_t *pUvDstBuf = pDstBuf+uvOffset;
    for(i=0; i<imageBufSize; i+=32)
    {
        pixData = vld4_u8(pUvSrcBuf+i);
        tmpData.val[0] = (int16x8_t)vshll_n_u8(pixData.val[0], LSHIFTBIT);
        tmpData.val[1] = (int16x8_t)vshll_n_u8(pixData.val[1], LSHIFTBIT);
        tmpData.val[2] = (int16x8_t)vshll_n_u8(pixData.val[2], LSHIFTBIT);
        tmpData.val[3] = (int16x8_t)vshll_n_u8(pixData.val[3], LSHIFTBIT);

        tmpData.val[0] = vqrdmulhq_s16(tmpData.val[0], uGains);
        tmpData.val[1] = vqrdmulhq_s16(tmpData.val[1], vGains);
        tmpData.val[2] = vqrdmulhq_s16(tmpData.val[2], uGains);
        tmpData.val[3] = vqrdmulhq_s16(tmpData.val[3], vGains);

        pixData.val[0] = vqmovun_s16(tmpData.val[0]);
        pixData.val[1] = vqmovun_s16(tmpData.val[1]);
        pixData.val[2] = vqmovun_s16(tmpData.val[2]);
        pixData.val[3] = vqmovun_s16(tmpData.val[3]);

        vst4_u8(pUvDstBuf+i, pixData);
    }

    return 0;
}


int imageAvgStridex32(uint16_t *pSrcBuf, size_t length, uint16_t &outAvg)
{
    size_t i;
    uint64_t sum=0;
    uint16x8x4_t vPixData;

    if(((length%32)!=0) || (length==0))
    {
        return -1;
    }
    for(i=0; i<length; i+=32)
    {
        vPixData = vld4q_u16(pSrcBuf+i);
        sum += vaddvq_u16(vPixData.val[0]);
        sum += vaddvq_u16(vPixData.val[1]);
        sum += vaddvq_u16(vPixData.val[2]);
        sum += vaddvq_u16(vPixData.val[3]);
    }
    outAvg = sum / length;
    return 0;
}
#if 0
int imageAvgStridex16(uint16_t *pSrcBuf, size_t length, uint16_t &outAvg)
{
    size_t i;
    uint64_t sum=0;
    uint16x4x4_t vPixData;

    if(length%16)
    {
        return -1;
    }
    for(i=0; i<length; i+=16)
    {
        vPixData = vld4_u16(pSrcBuf+i);
        sum += vaddv_u16(vPixData.val[0]);
        sum += vaddv_u16(vPixData.val[1]);
        sum += vaddv_u16(vPixData.val[2]);
        sum += vaddv_u16(vPixData.val[3]);
    }
    outAvg = sum / (length+1);
    return 0;
}
#else

/*
 * Taking source buffer and return average
 * The buffer length should be multiple of 16
 * x0: sum
 * x1: src
 * x2: len
 * x3: tmp: using to move from neon register h0 to x3
 * x4: for loop increment
 * v0: SIMD vector of uint16x8_t
 * h0: result of vector addv
 */
int imageAvgStridex16(uint16_t *pSrcBuf, size_t length, uint16_t &outAvg)
{
    uint64_t sum=0;
    if(((length%32)!=0) || (length==0))
    {
        return -1;
    }
    asm volatile (
        "mov  x4, %0 \n"
        "mov  %[sum], #0\n"
        "L1:   \n"
        "ld1  {v0.8h}, [%[src]], #16 \n" //Load from source 16 bytes or uint16x8_t
        "addv h0, v0.8h \n"             //Sum all vector to scalar h0
        "umov x3, v0.d[0] \n"           //Move from neon h0 register to arm GP regiser x3
        "add  %[sum], %[sum], x3 \n"    //Add the result of addv (h0) to total sum x0
        "add  x4, x4, #8 \n"            //x4 is for for loop iteration
        "cmp  x4, %[len] \n"
        "bne  L1 \n"
        : [sum] "+r" (sum)
        : [src] "r" (pSrcBuf), [len] "r" (length)
        : "memory", "v0"
        );

    outAvg = sum / length;
    return 0;
}

#endif
void add_float_neon3(float* dst, float* src1, float* src2, int64_t count)
{
    asm volatile (
        "1: \n"
        "ld1 {v0.4s}, [%[src1]], #16 \n"
        "ld1 {v1.4s}, [%[src2]], #16 \n"
        "fadd v0.4s, v0.4s, v1.4s \n"
        "subs %[count], %[count], #4 \n"
        "st1 {v0.4s}, [%[dst]], #16 \n"
        "bgt 1b \n"
        : [dst] "+r" (dst)
        : [src1] "r" (src1), [src2] "r" (src2), [count] "r" (count)
        : "memory", "v0", "v1"
    );
}


#if 0
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
        imageApplyUVGainsNV12x2(pSrcImage, pDstImage, IMAGE_WIDTH, IMAGE_HEIGHT, uGain, vGain);
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


#else

int main (int argc, char *argv[])
{
    int bufLen = 1920*1080;
    uint16_t *imageBuf = new uint16_t [bufLen];
    uint16_t imageAvg=0;
    int i;
    struct timespec timeStart, timeEnd;
    long int timeDiff;

    for(i=0; i<bufLen; i++)
    {
        imageBuf[i] = (i%4096)+1;
    }

    clock_gettime(CLOCK_MONOTONIC, &timeStart);
    for(i=0;i<1000;i++)
    {
        imageAvgStridex16(imageBuf, bufLen, imageAvg);
    }
    clock_gettime(CLOCK_MONOTONIC, &timeEnd);



    printf("AVG=%d\n", imageAvg);
    timeDiff = (timeEnd.tv_sec*1000000000L + timeEnd.tv_nsec) - (timeStart.tv_sec*1000000000L + timeStart.tv_nsec);
    printf("Computing Time per frame=%ldus\n", timeDiff/1000/1000);
    delete []imageBuf;
}
#endif


