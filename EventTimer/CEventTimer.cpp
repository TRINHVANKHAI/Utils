/*
 * Copyright (C) 2024 Hino Engineering all right reserved.
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

#include <CEventTimer.h>
#include <iostream>

using namespace std;

CEventTimer::CEventTimer(EEventTimerMode_t in_mode,
                               int in_prio,
                               int in_pol,
                               long long int in_period)
    : m_timePeriod(in_period), m_runMode(in_mode), m_timeoutCb(timeoutCb)
{
    int ret;
    int msgLen = 128;
    char *msg = new char[msgLen];

    if(m_timePeriod<0)
    {
        snprintf(msg, msgLen, "%s invalid time period=%lld\n", __func__, m_timePeriod);
        throw runtime_error(msg);
    }


    ret = pthread_attr_init(&m_threadAttr);
    if (ret!=0) {
        snprintf(msg, msgLen, "%s pthread_attr_init failed ret=%d\n", __func__, ret);
        throw runtime_error(msg);
    }

    ret = pthread_attr_setschedpolicy(&m_threadAttr, in_pol);
    if (ret!=0) {
        snprintf(msg, msgLen, "%s pthread_attr_setschedpolicy failed ret=%d\n", __func__, ret);
        throw runtime_error(msg);
    }

    m_threadParam.sched_priority = in_prio;
    ret = pthread_attr_setschedparam(&m_threadAttr, &m_threadParam);
    if (ret!=0) {
        snprintf(msg, msgLen, "%s pthread_attr_setschedparam failed ret=%d\n", __func__, ret);
        throw runtime_error(msg);
    }

    ret = pthread_attr_setinheritsched(&m_threadAttr, PTHREAD_EXPLICIT_SCHED);
    if (ret!=0) {
        snprintf(msg, msgLen, "%s pthread_attr_setinheritsched failed ret=%d\n", __func__, ret);
        throw runtime_error(msg);
    }


    ret |= pthread_mutexattr_init(&m_mAttr);
    ret |= pthread_mutexattr_setpshared(&m_mAttr, PTHREAD_PROCESS_PRIVATE);
    ret |= pthread_mutexattr_settype(&m_mAttr, PTHREAD_MUTEX_ERRORCHECK);
    ret |= pthread_mutexattr_setprotocol(&m_mAttr, PTHREAD_PRIO_NONE);
    ret |= pthread_mutex_init(&m_threadLock, &m_mAttr);
    if (ret!=0) {
        snprintf(msg, msgLen, "%s pthread_mutex_init failed ret=%d\n", __func__, ret);
        throw runtime_error(msg);
    }


    ret |= pthread_condattr_init(&m_condAttr);
    ret |= pthread_condattr_setpshared(&m_condAttr, PTHREAD_PROCESS_PRIVATE);
    ret |= pthread_condattr_setclock(&m_condAttr, CLOCK_MONOTONIC);
    ret |= pthread_cond_init(&m_threadSig, &m_condAttr);
    if (ret!=0) {
        snprintf(msg, msgLen, "%s pthread_cond_init failed ret=%d\n", __func__, ret);
        throw runtime_error(msg);
    }

    ret = procBegin();
    if(ret != 0)
    {
        snprintf(msg, msgLen, "%s procBegin failed ret=%d\n", __func__, ret);
        throw runtime_error(msg);
    }

    delete []msg;
}

CEventTimer::~CEventTimer()
{
    procEnd();
    pthread_mutexattr_destroy(&m_mAttr);
    pthread_condattr_destroy(&m_condAttr);
    pthread_cond_destroy(&m_threadSig);
    pthread_mutex_destroy(&m_threadLock);
}

int CEventTimer::procBegin()
{
    int ret;

    m_runState = EEventTimerStateRunning;
    ret = pthread_create(&m_thread, &m_threadAttr, procRunHandler, this);
    if (ret!=0) {
        return ret;
    }

    while(m_runState != EEventTimerStateWait)
    {
    }

    return 0;
}

void CEventTimer::procEnd()
{
    pthread_mutex_lock(&m_threadLock);
    m_runState = EEventTimerStateTerminated;
    pthread_cond_signal(&m_threadSig);
    pthread_mutex_unlock(&m_threadLock);
    pthread_join(m_thread, nullptr);
}

int CEventTimer::startTimer()
{
    int ret;
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    m_timerBase  = (time.tv_sec*1000000000L) + time.tv_nsec;

    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateRunning;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    return 0;
}
int CEventTimer::startTimer(long long int *out_timerBase)
{
    int ret;
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    m_timerBase  = (time.tv_sec*1000000000L) + time.tv_nsec;
    *out_timerBase = m_timerBase;

    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateRunning;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    return 0;
}

int CEventTimer::startTimer(long long int in_period)
{
    int ret;
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);

    if(in_period<1)
    {
        return -1;
    }
    m_timerBase  = (time.tv_sec*1000000000L) + time.tv_nsec;
    m_timePeriod = in_period;
    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateRunning;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    return 0;
}

int CEventTimer::startTimer(long long int in_period, long long int *out_timerBase)
{
    int ret;
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);

    if(in_period<1)
    {
        return -1;
    }
    m_timerBase  = (time.tv_sec*1000000000L) + time.tv_nsec;
    m_timePeriod = in_period;
    *out_timerBase = m_timerBase;

    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateRunning;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    return 0;
}

int CEventTimer::startTimer(long long int in_timerBase, long long int in_period)
{
    int ret;
    struct timespec time;
    long long int curTime;
    long long int factor;
    if(in_period<1)
    {
        return -1;
    }
    clock_gettime(CLOCK_MONOTONIC, &time);
    curTime = (time.tv_sec * 1000000000L) + time.tv_nsec;
    if(curTime > in_timerBase) {
        factor  = (curTime - in_timerBase)/in_period;
        m_timerBase  = in_timerBase + (factor*in_period);
    }
    else
    {
        m_timerBase  = in_timerBase;
    }
    m_timePeriod = in_period;
    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateRunning;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    return 0;
}

int CEventTimer::stopTimer()
{
    int ret;
    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateStopped;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }
    while(m_runState != EEventTimerStateWait)
    {

    }
    return 0;
}

int CEventTimer::updateTimer(long long int in_period)
{
    int ret;
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    if(in_period<1)
    {
        return -1;
    }

    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateStopped;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    while(m_runState != EEventTimerStateWait)
    {

    }

    m_timerBase  = (time.tv_sec*1000000000L) + time.tv_nsec;
    m_timePeriod = in_period;

    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateRunning;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    return 0;
}

int CEventTimer::updateTimer(long long int in_timerBase, long long int in_period)
{
    int ret;

    if(in_period<1)
    {
        return -1;
    }

    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateStopped;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    while(m_runState != EEventTimerStateWait)
    {

    }

    m_timerBase  = in_timerBase;
    m_timePeriod = in_period;

    ret = pthread_mutex_lock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    m_runState = EEventTimerStateRunning;
    ret = pthread_cond_signal(&m_threadSig);
    if(ret != 0)
    {
        return ret;
    }

    ret = pthread_mutex_unlock(&m_threadLock);
    if(ret != 0)
    {
        return ret;
    }

    return 0;
}

int CEventTimer::registerTimeoutCb(FEventTimeout_t in_pTimeoutCb, void *in_pArgs)
{
    if(in_pTimeoutCb==nullptr)
    {
        return -1;
    }
    m_timeoutCb = in_pTimeoutCb;
    m_pArgs     = in_pArgs;
    return 0;
}

void CEventTimer::procRun()
{
    long long int nextTick;
    struct timespec time;
    while(1)
    {
        pthread_mutex_lock(&m_threadLock);

        if(m_runState == EEventTimerStateTerminated)
        {
            break;
        }

        m_runState = EEventTimerStateWait;
        pthread_cond_wait(&m_threadSig, &m_threadLock);
        pthread_mutex_unlock(&m_threadLock);

        time.tv_sec  = m_timerBase / 1000000000L;
        time.tv_nsec = m_timerBase % 1000000000L;

        while(m_runState == EEventTimerStateRunning)
        {
            nextTick = ((time.tv_sec * 1000000000L) + time.tv_nsec) + m_timePeriod;
            time.tv_sec  = nextTick / 1000000000L;
            time.tv_nsec = nextTick % 1000000000L;
            pthread_mutex_lock(&m_threadLock);
            pthread_cond_timedwait(&m_threadSig, &m_threadLock, &time);
            pthread_mutex_unlock(&m_threadLock);
            m_timeoutCb(m_pArgs, nextTick);
            if (m_runMode == EEventTimerModeDelay)
            {
                break;
            }
        }
    }
}

