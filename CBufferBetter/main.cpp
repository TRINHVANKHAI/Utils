#include <iostream>
#include <CBufferManager.h>
#include <string.h>
#include <unistd.h>
using namespace std;

char *pSrcBuffer=nullptr;
char *pDstBuffer=nullptr;
void *bufferGetThread(void* args){
    CBufferManager *pAllocator = (CBufferManager *)args;

    int ret;
    size_t size;
    int i;
    unsigned long long int checkOkCount = 0;
    int checkFailedCount = 0;
    printf("bufferGetStarted\n");
    for(i=0;i<102400;) {
        memset(pDstBuffer, 0, 4096);
        ret = pAllocator->getBuffer(pDstBuffer, &size);
        if(ret==0) {
            if(size!=4096) {
                printf("Error get buffer size=%zu!=4096\n", size);
                return NULL;
            }
            if (memcmp(pDstBuffer, pSrcBuffer, 4096)!=0) {
                printf("Data integrity is not guaranty\n");
                return NULL;
            } else {
                checkOkCount++;
                if(checkOkCount%1024==0) {
                    printf("good count=%lld\n", checkOkCount);
                }
            }
            i++;
        }

    }



    printf("bufferGetThread test passed exit, free resource\n");

    printf("bufferGetThread exit success!\n");
    return NULL;
}

void *bufferSetThread(void*args)
{
    int ret;
    int i;
    CBufferManager *pAllocator = (CBufferManager *)args;
    printf("bufferSetStarted\n");

    for(i=0;i<4096;i++) {
        pSrcBuffer[i] = i%0xff;
    }
    for(i=0;i<102400;) {
        ret = pAllocator->setBuffer(pSrcBuffer, 4096);
        if(ret==0) {
            i++;
        }

    }
    printf("bufferSetThread test passed exit, free resource\n");

    printf("bufferSetThread exit success!\n");
    return NULL;
}


int main()
{
    cout << "Hello World!" << endl;

    pthread_t getThread, setThread;
    int i;
    int ret;
    pSrcBuffer = new char [4096];
    pDstBuffer = new char [4096];

    CBufferManager *pAllocator=new CBufferManager(4096, 4096);

    pthread_create(&setThread, NULL, bufferSetThread, pAllocator);

    pthread_create(&getThread, NULL, bufferGetThread, pAllocator);


    pthread_join(setThread, NULL);

    pthread_join(getThread, NULL);
    printf("Main Test passed exit, free resource\n");
    delete pAllocator;
    delete []pDstBuffer;
    delete []pSrcBuffer;
    printf("Main exit success!\n");
    return 0;
}
