#ifndef CBUFFERMANAGER_H
#define CBUFFERMANAGER_H
/*
 * Copyright (C) 2022 Hino Engineering all right reserved.
 *
 * Author: TRINH VAN KHAI <kai@hinoeng.co.jp>
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the PG_ORGANIZATION nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY	THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS-IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <pthread.h>

typedef enum EBufferAllocStatus
{
    EBufferAllocStatusDealocated,
    EBufferAllocStatusAllocated,
    EBufferAllocStatusReady,
    EBufferAllocStatusFree,
    EBufferAllocStatusPending,
    EBufferAllocStatusOverflow,
    EBufferAllocStatusIoBusy
} EBufferAllocStatus;

typedef struct SBufferElement
{
    struct SBufferElement *next;
    struct SBufferElement *prev;
    pthread_mutex_t lock;
    EBufferAllocStatus status;
    void *pBuff;
    size_t size;
} SBufferElement_t;

typedef struct SBufferAllocator
{
    size_t size;
    unsigned int count;
    SBufferElement_t *pHead;
    SBufferElement_t *pQueue;
    SBufferElement_t *pDequeue;
    pthread_mutex_t mlock;
} SBufferAllocator_t;

class CBufferManager
{
public:
    CBufferManager(size_t in_size, unsigned int in_count);
    ~CBufferManager();
    int setBuffer(void *in_pBuff, size_t in_size);
    int getBuffer(void *in_pBuff, size_t *out_size);
private:
    SBufferAllocator_t *m_pAllocator;
    int allocBufferAllocator(size_t in_size, unsigned int in_count);
    void deallocBufferAllocator();
    int initBuffer(SBufferElement_t **out_pBuffer, size_t in_size);
    void deinitBuffer(SBufferElement_t *in_pBuffer);
    int addBuffer(SBufferElement_t *in_pBuffer, size_t in_size);
    int addBufferLast(SBufferElement_t *in_pBuffer, size_t in_size);
    void delBuffer(SBufferElement_t *in_pBuffer);
    void delBufferLast(SBufferElement_t *in_pBuffer);
};


#endif // CBUFFERMANAGER_H
