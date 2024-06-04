/*
 * POSIX Real Time Example
 * using a single pthread as RT thread
 */

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <event.h>
#include "gpiodev.h"



#define CUSTOM_STACK_SIZE 4096*16
#define FST_TIME_INTERVAL_NS 5000000
#define SND_TIME_INTERVAL_NS 1000000

struct pdata {
    GpioDev *gpio;
    struct timespec pre_time;
    struct timespec cur_time;
    int64_t ns_jitter;
    int64_t ns_jitter_max;
};

static void fst_timer_cb (evutil_socket_t fd, short event, void *args)
{
    struct pdata *pdat = (struct pdata *)args;
    clock_gettime(CLOCK_MONOTONIC, &pdat->cur_time);
    pdat->gpio->toggleValue();
    pdat->ns_jitter = ((int64_t)pdat->cur_time.tv_sec * 1000000000 + (int64_t)pdat->cur_time.tv_nsec)
                    - ((int64_t)pdat->pre_time.tv_sec * 1000000000 + (int64_t)pdat->pre_time.tv_nsec);
    pdat->ns_jitter = pdat->ns_jitter - FST_TIME_INTERVAL_NS;
    if(pdat->ns_jitter_max < pdat->ns_jitter) {
        pdat->ns_jitter_max = pdat->ns_jitter;
        printf("FST_MAX: %06ld us\n", pdat->ns_jitter_max/1000);
    }
    pdat->pre_time = pdat->cur_time;
}

static void snd_timer_cb (evutil_socket_t fd, short event, void *args)
{
    struct pdata *pdat = (struct pdata *)args;
    clock_gettime(CLOCK_MONOTONIC, &pdat->cur_time);
    pdat->gpio->toggleValue();
    pdat->ns_jitter = ((int64_t)pdat->cur_time.tv_sec * 1000000000 + (int64_t)pdat->cur_time.tv_nsec)
                      - ((int64_t)pdat->pre_time.tv_sec * 1000000000 + (int64_t)pdat->pre_time.tv_nsec);
    pdat->ns_jitter = pdat->ns_jitter - SND_TIME_INTERVAL_NS;
    if(pdat->ns_jitter_max < pdat->ns_jitter) {
        pdat->ns_jitter_max = pdat->ns_jitter;
        printf("SND_MAX: %06ld us\n", pdat->ns_jitter_max/1000);
    }
    pdat->pre_time = pdat->cur_time;
}


void *tm_thread_fst(void *args)
{
    /* Do RT specific stuff here */
    int ret;
    struct pdata pdat;
    memset(&pdat, 0, sizeof(pdat));
    pdat.gpio = new GpioDev("gpiochip2", 22);
    struct timespec tm_sleep;
    tm_sleep.tv_sec = 0;
    tm_sleep.tv_nsec = FST_TIME_INTERVAL_NS;
    clock_gettime(CLOCK_MONOTONIC, &pdat.pre_time);
    clock_gettime(CLOCK_MONOTONIC, &pdat.cur_time);
    while(1) {
        clock_nanosleep( CLOCK_MONOTONIC, 0, &tm_sleep, NULL);
        fst_timer_cb (0, 0, &pdat);
    }

    delete pdat.gpio;

    printf("Event thread exit\n");
    return NULL;
}

void *tm_thread_snd(void *args)
{
    /* Do RT specific stuff here */
    int ret;
    struct pdata pdat;
    memset(&pdat, 0, sizeof(pdat));
    pdat.gpio = new GpioDev("gpiochip2", 23);
    struct timespec tm_sleep;
    tm_sleep.tv_sec = 0;
    tm_sleep.tv_nsec = SND_TIME_INTERVAL_NS;
    clock_gettime(CLOCK_MONOTONIC, &pdat.pre_time);
    clock_gettime(CLOCK_MONOTONIC, &pdat.cur_time);
    while(1) {
        clock_nanosleep( CLOCK_MONOTONIC, 0, &tm_sleep, NULL);
        snd_timer_cb (0, 0, &pdat);
    }

    delete pdat.gpio;

    printf("Event thread exit\n");
    return NULL;
}

int main(int argc, char* argv[])
{
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t tm_fst, tm_snd;
    int ret;
#if 1
    static int latency_target_fd = -1;
    static int32_t latency_target_value = 0;

    /* Latency trick
     * if the file /dev/cpu_dma_latency exists,
     * open it and write a zero into it. This will tell
     * the power management system not to transition to
     * a high cstate (in fact, the system acts like idle=poll)
     * When the fd to /dev/cpu_dma_latency is closed, the behavior
     * goes back to the system default.
     *
     * Documentation/power/pm_qos_interface.txt
     */

    struct stat st;
    if (stat("/dev/cpu_dma_latency", &st) == 0) {
        latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
        if (latency_target_fd == -1)
            return -1;
        ret = write(latency_target_fd, &latency_target_value, 4);
        if (ret == 0) {
            printf("# error setting cpu_dma_latency to %d!: %s\n", latency_target_value, strerror(errno));
            close(latency_target_fd);
            return -1;
        }
        printf("# /dev/cpu_dma_latency set to %dus\n", latency_target_value);
    }


    /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }

    /* Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        goto out;
    }

    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN+CUSTOM_STACK_SIZE);
    if (ret) {
        printf("pthread setstacksize failed\n");
        goto out;
    }

    /* Set scheduler policy and priority of pthread */
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        printf("pthread setschedpolicy failed\n");
        goto out;
    }
    param.sched_priority = 80;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        printf("pthread setschedparam failed\n");
        goto out;
    }
    /* Use scheduling parameters of attr */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        printf("pthread setinheritsched failed\n");
        goto out;
    }

    /* Create a pthread with specified attributes */
    ret = pthread_create(&tm_fst, &attr, tm_thread_fst, NULL);
    if (ret) {
        printf("create pthread failed\n");
        goto out;
    }

    /* Create a pthread with specified attributes */
    ret = pthread_create(&tm_snd, &attr, tm_thread_snd, NULL);
    if (ret) {
        printf("create pthread failed\n");
        goto out;
    }
#else
    /* Create a pthread with specified attributes */
    ret = pthread_create(&thread, NULL, thread_func, NULL);
    if (ret) {
        printf("create pthread failed\n");
        goto out;
    }
#endif

    ret = pthread_join(tm_fst, NULL);
    if (ret)
        printf("join pthread failed: %m\n");

    ret = pthread_join(tm_snd, NULL);
    if (ret)
        printf("join pthread failed: %m\n");

out:
    close(latency_target_fd);
    printf("Main thread exit\n");
    return ret;
}
