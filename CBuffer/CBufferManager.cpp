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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <CBufferManager.h>

using namespace std;

CBufferManager::CBufferManager(size_t in_size, unsigned int in_count)
{
    unsigned int count;
    int msgLen = 128;
    char *msg = new char[msgLen];

    if((in_size<1) ||
       (in_count<1))
    {
        snprintf(msg, msgLen, "%s: invalid parameter specified size=%zu, count=%d\n", __func__, in_size, in_count);
        throw runtime_error(msg);
    }

    try
    {
        m_pAllocator = new SBufferAllocator;
    } catch (...) {
        snprintf(msg, msgLen, "%s: buffer allocator create error\n", __func__);
        throw runtime_error(msg);
    }


    m_pAllocator->size = in_size;
    m_pAllocator->count = in_count;
    count = allocBufferAllocator(in_size, in_count);
    if (count!=in_count)
    {
        snprintf(msg, msgLen, "%s: failed to allocate buffer, count=%d\n", __func__, count);
        throw runtime_error(msg);
    }
    delete []msg;
}

CBufferManager::~CBufferManager()
{
    deallocBufferAllocator();
    delete m_pAllocator;
}

int CBufferManager::allocBufferAllocator(size_t in_size, unsigned int in_count) {
    SBufferElement_t *pBuffer=nullptr;
    unsigned int i=0;
    if((in_size <=0) || (in_count<=0)) {
        fprintf(stderr, "Error: Invalid buffer size and count\n");
        return -1;
    }

    initBuffer(&pBuffer, in_size);
    for(i=0; i<in_count-1; i++) {
        if(addBufferLast(pBuffer, in_size)<0){
            fprintf(stderr, "Error: Cannot add new buffer %ld\n", in_size);
            break;
        }
    }

    m_pAllocator->pHead = pBuffer;
    m_pAllocator->pDequeue = pBuffer;
    m_pAllocator->pQueue = pBuffer;
    m_pAllocator->size = in_size;
    m_pAllocator->count = in_count;
    return i+1;
}

void CBufferManager::deallocBufferAllocator() {
    SBufferElement_t *pBuffer = m_pAllocator->pHead;

    for(pBuffer=m_pAllocator->pHead; pBuffer->next != m_pAllocator->pHead;) {
        delBufferLast(m_pAllocator->pHead);
    }

    deinitBuffer(m_pAllocator->pHead);
    m_pAllocator->pQueue   = nullptr;
    m_pAllocator->pHead    = nullptr;
    m_pAllocator->pDequeue = nullptr;
}

int CBufferManager::initBuffer(SBufferElement_t **out_pBuffer, size_t in_size) {
    SBufferElement_t *pBuffer = (SBufferElement_t *)malloc(sizeof(SBufferElement_t));
    if(pBuffer==nullptr) {
        return -1;
    }
    pBuffer->next = pBuffer;
    pBuffer->prev = pBuffer;
    pBuffer->size = in_size;
    pBuffer->pBuff = (unsigned char *) malloc (pBuffer->size);
    if (pBuffer->pBuff==nullptr) {
        fprintf (stderr, "Error: Cannot allocate memory %ld \n", pBuffer->size);
        free(pBuffer);
        return -1;
    } else {
        pBuffer->status = EBufferAllocStatusAllocated;
    }

    if (pthread_mutex_init(&pBuffer->lock, nullptr) != 0) {
        fprintf(stderr, "Error: mutex init has failed\n");
        free(pBuffer->pBuff);
        free(pBuffer);
        return -1;
    }

    *out_pBuffer = pBuffer;
    return 0;
}

void CBufferManager::deinitBuffer(SBufferElement_t *in_pBuffer) {
    pthread_mutex_destroy(&in_pBuffer->lock);
    free(in_pBuffer->pBuff);
    free(in_pBuffer);
}

int CBufferManager::addBuffer(SBufferElement_t *in_pBuffer, size_t in_size) {
    if (in_pBuffer == nullptr) {
        fprintf(stderr, "Error: Buffer is empty\n");
        return -1;
    }
    SBufferElement_t *pBuffer = (SBufferElement_t *) malloc(sizeof(SBufferElement_t));
    if(pBuffer == nullptr) {
        fprintf(stderr, "Error: Cannot add new buffer \n");
        return -1;
    }
    pBuffer->size = in_size;
    pBuffer->pBuff = (unsigned char *) malloc (pBuffer->size);
    pBuffer->prev = in_pBuffer;
    pBuffer->next = in_pBuffer->next;

    if (pBuffer->pBuff==nullptr) {
        fprintf (stderr, "Error: Cannot allocate memory %ld \n", pBuffer->size);
        free(pBuffer);
        return -1;
    } else {
        pBuffer->status = EBufferAllocStatusAllocated;
    }
    if (pthread_mutex_init(&pBuffer->lock, nullptr) != 0) {
        fprintf(stderr, "Error: mutex init has failed\n");
        free(pBuffer->pBuff);
        free(pBuffer);
        return -1;
    }

    in_pBuffer->next->prev = pBuffer;
    in_pBuffer->next = pBuffer;
    return 0;
}

int CBufferManager::addBufferLast(SBufferElement_t *in_pBuffer, size_t in_size) {
    if (in_pBuffer == nullptr) {
        fprintf(stderr, "Error: Buffer is empty\n");
        return -1;
    }

    SBufferElement_t *pCurBuffer = in_pBuffer->prev;
    SBufferElement_t *pBuffer    = (SBufferElement_t *) malloc(sizeof(SBufferElement_t));

    if(pBuffer==nullptr) {
        fprintf(stderr, "Error: Cannot add new buffer \n");
        return -1;
    }

    pBuffer->size = in_size;
    pBuffer->pBuff = (unsigned char *) malloc (pBuffer->size);
    pBuffer->prev = pCurBuffer;
    pBuffer->next = in_pBuffer;

    if (pBuffer->pBuff==nullptr) {
        fprintf (stderr, "Error: Cannot add buffer %ld \n", pBuffer->size);
        free(pBuffer);
        return -1;
    } else {
        pBuffer->status = EBufferAllocStatusAllocated;
    }

    if (pthread_mutex_init(&pBuffer->lock, nullptr) != 0) {
        fprintf(stderr, "Error: mutex init has failed\n");
        free(pBuffer->pBuff);
        free(pBuffer);
        return -2;
    }

    in_pBuffer->prev = pBuffer;
    pCurBuffer->next = pBuffer;
    return 0;
}


void CBufferManager::delBuffer(SBufferElement_t *in_pBuffer) {
    if (in_pBuffer == nullptr) {
        fprintf(stderr, "Error: Buffer is empty\n");
        return;
    }
    SBufferElement_t *pNextBuffer = in_pBuffer->next->next;
    SBufferElement_t *pCurrBuffer = in_pBuffer->next;
    in_pBuffer->next = pNextBuffer;
    pNextBuffer->prev = in_pBuffer;
    pthread_mutex_destroy(&pCurrBuffer->lock);
    free(pCurrBuffer->pBuff);
    free(pCurrBuffer);
}


void CBufferManager::delBufferLast(SBufferElement_t *in_pBuffer) {
    if (in_pBuffer == nullptr) {
        fprintf(stderr, "Error: Buffer is empty\n");
        return;
    }
    SBufferElement_t *pPrevBuffer = in_pBuffer->prev->prev;
    SBufferElement_t *pBuffer = in_pBuffer->prev;
    pPrevBuffer->next = in_pBuffer;
    in_pBuffer->prev = pPrevBuffer;
    pthread_mutex_destroy(&pBuffer->lock);
    free(pBuffer->pBuff);
    free(pBuffer);
}

int CBufferManager::setBuffer(void *in_pBuff, size_t in_size) {
    int ret;
    void *pBufferRet;
    SBufferElement_t *pBuffer = m_pAllocator->pQueue;
    if(pBuffer->status == EBufferAllocStatusReady) {
        m_pAllocator->pDequeue = m_pAllocator->pDequeue->next;
    }

    pthread_mutex_lock(&pBuffer->lock);
    pBufferRet = memcpy(pBuffer->pBuff, in_pBuff, in_size);
    ret = pBufferRet == pBuffer->pBuff ? 0 : -1;
    if(ret==0) {
        m_pAllocator->pQueue->status = EBufferAllocStatusReady;
        pthread_mutex_unlock(&m_pAllocator->pQueue->lock);
        m_pAllocator->pQueue = m_pAllocator->pQueue->next;
    } else {
        pthread_mutex_unlock(&m_pAllocator->pQueue->lock);
    }

    return ret;
}

int CBufferManager::getBuffer(void *in_pBuff, size_t *out_size) {
    SBufferElement_t *pBuffer = m_pAllocator->pDequeue;

    if(pBuffer->status == EBufferAllocStatusReady) {
        pthread_mutex_lock(&pBuffer->lock);
        *out_size = pBuffer->size;
        memcpy(in_pBuff, pBuffer->pBuff, pBuffer->size);
        pBuffer->status = EBufferAllocStatusFree;
        pthread_mutex_unlock(&pBuffer->lock);
        m_pAllocator->pDequeue = m_pAllocator->pDequeue->next;
        return 0;
    }
    return -1;
}

