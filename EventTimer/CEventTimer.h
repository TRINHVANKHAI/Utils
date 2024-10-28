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


#ifndef CEVENTTIMER_H
#define CEVENTTIMER_H
#include <pthread.h>

typedef enum EEventTimerState
{
    EEventTimerStateTerminated,
    EEventTimerStateStopped,
    EEventTimerStateWait,
    EEventTimerStateRunning
} EEventTimerState_t;

typedef enum EEventTimerMode
{
    EEventTimerModeDelay,
    EEventTimerModePeriodic
} EEventTimerMode_t;

typedef void *(*FEventTimeout_t)(void *, long long int);

class CEventTimer
{
public:
    CEventTimer(EEventTimerMode_t in_mode=EEventTimerModePeriodic,
                   int in_prio=85,
                   int in_pol=SCHED_FIFO,
                   long long int in_period=0L);
    ~CEventTimer();
    int startTimer();
    int startTimer(long long int *out_timerBase);
    int startTimer(long long int in_period);
    int startTimer(long long int in_period, long long int *out_timerBase);
    int startTimer(long long int in_timerBase, long long int in_period);
    int stopTimer();
    int updateTimer(long long int in_period);
    int updateTimer(long long int in_timerBase, long long int in_period);
    int registerTimeoutCb(FEventTimeout_t in_timeoutCb, void *in_pArgs);

private:
    int procBegin();
    void procEnd();
    void procRun();
    static void *timeoutCb(void *in_pArgs, long long int in_time)
    {
        if(in_time == 0)
        {
            return nullptr;
        }
        else
        {
            return in_pArgs;
        }
    }
    static void *procRunHandler(void *in_pThis)
    {
        ((CEventTimer *)in_pThis)->procRun();
        return nullptr;
    }
    long long int m_timerBase;
    long long int m_timePeriod;

    pthread_t m_thread;
    pthread_attr_t m_threadAttr;
    struct sched_param m_threadParam;
    pthread_mutex_t m_threadLock;
    pthread_mutexattr_t m_mAttr;
    pthread_cond_t m_threadSig;
    pthread_condattr_t m_condAttr;
    EEventTimerMode_t  m_runMode;
    EEventTimerState_t m_runState;
    FEventTimeout_t m_timeoutCb;
    void *m_pArgs;
};

#endif // CEVENTTIMER_H
