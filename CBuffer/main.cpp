#include <iostream>
#include <CBufferManager.h>
#include <string.h>
#include <unistd.h>
using namespace std;

static char *pCmpBuffer=nullptr;
void *bufferGetThread(void* args){
    CBufferManager *pAllocator = (CBufferManager *)args;
    char *pDstBuffer = new char [4096];
    int ret;
    size_t size;
    unsigned long long int checkOkCount = 0;
    int checkFailedCount = 0;
    while(1) {
        memset(pDstBuffer, 0, 4096);
        ret = pAllocator->getBuffer(pDstBuffer, &size);
        if(size!=4096) {
            printf("Error get buffer size=%zu!=4096\n", size);
            return NULL;
        }
        if(ret==0) {
            if (memcmp(pDstBuffer, pCmpBuffer, 4096)!=0) {
                printf("Data integrity is not guaranty\n");
                return NULL;
            } else {
                //printf(".");
                checkOkCount++;
                if(checkOkCount%102400==0) {
                    printf("good count=%lld\n", checkOkCount);
                }
            }
        }
    }



    printf("bufferGetThread test passed exit, free resource\n");
    delete []pDstBuffer;
    printf("bufferGetThread exit success!\n");
    return NULL;
}
int main()
{
    cout << "Hello World!" << endl;
    char *pSrcBuffer = new char [4096];
    pCmpBuffer = pSrcBuffer;
    pthread_t getThread;
    int i;
    int ret;
    for(i=0;i<4096;i++) {
        pSrcBuffer[i] = i%0xff;
    }

    CBufferManager *pAllocator=new CBufferManager(4096, 4096);

    pthread_create(&getThread, NULL, bufferGetThread, pAllocator);

    while(1) {
        ret = pAllocator->setBuffer(pSrcBuffer, 4096);
        if(ret!=0) {
            printf("Main coppy error ret=%d\n", ret);
        }
        //usleep(10000);
    }
    pthread_join(getThread, NULL);
    printf("Main Test passed exit, free resource\n");
    delete pAllocator;
    delete []pSrcBuffer;
    printf("Main exit success!\n");
    return 0;
}
