#include <iostream>
#include "maskgenerator.h"

using namespace std;



struct MaskImage mskImg;
int main()
{
    cout << "Hello World!" << endl;
    double ratio = 1/sqrt(2);
    maskgenerator_circle_twelve_blocks_generate(&mskImg, 360, 480, V4L2_PIX_FMT_YUYV, ratio);
    printf("Width=%d, Height=%d, Block dim=%d\n", mskImg.width, mskImg.height, mskImg.block_dim);
    printf("Buf=%x, length=%d, pixformat=%d\n", mskImg.buf, mskImg.length, mskImg.pixelformat);
    maskgenerator_circle_twelve_blocks_free(&mskImg);
    return 0;
}
