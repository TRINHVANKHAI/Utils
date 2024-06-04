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

static struct timeval g_tv;
static char timer_dmp[8];

/*
 * fd: Timer fd
 * event:
 */

GpioDev *gpio_rt;

static struct timeval pre_time;
static struct timeval cur_time;
int64_t us_jitter=-1;
int64_t us_jitter_max=0;
static void timer_cb (evutil_socket_t fd, short event, void *args)
{
    gettimeofday(&cur_time, NULL);
    //read(fd, timer_dmp, 8);
    gpio_rt->toggleValue();

    us_jitter = ((int64_t)cur_time.tv_sec * 1000000 + (int64_t)cur_time.tv_usec) - ((int64_t)pre_time.tv_sec * 1000000 + (int64_t)pre_time.tv_usec);
    us_jitter = us_jitter - 5000;
    if(us_jitter_max < us_jitter) {
        us_jitter_max = us_jitter;
        printf("MAX: %06ld us\n", us_jitter_max);
    }
    pre_time = cur_time;
}

void *thread_func(void *args)
{
    /* Do RT specific stuff here */
    int ret;
    gpio_rt = new GpioDev("gpiochip2", 22);

    struct itimerspec tm_spec;
    tm_spec.it_interval.tv_sec = 0;
    tm_spec.it_interval.tv_nsec = 5000000;
    tm_spec.it_value.tv_sec = 0;
    tm_spec.it_value.tv_nsec = 5000000;

    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if(tfd<0) {
        fprintf(stderr, "Error create timer\n");
        return NULL;
    }
    if (timerfd_settime (tfd, 0, &tm_spec, NULL) < 0 ) {
        fprintf(stderr, "Error set time\n");
        close(tfd);
        return NULL;
    }

#if 0
    struct event_config *evcfg = event_config_new();
    if(evcfg == NULL) {
        printf("Create evbase config failed\n");
        return NULL;
    }
    event_config_set_flag(evcfg, EVENT_BASE_FLAG_PRECISE_TIMER);
    event_config_avoid_method(evcfg, "select");
    event_config_avoid_method(evcfg, "poll");
    struct event_base *evbase = event_base_new_with_config(evcfg);
    if(evbase == NULL) {
        printf("Create evbase failed \n");
        return NULL;
    }

    struct timeval tv = {1, 0};
    struct event *ev  = event_new (evbase, tfd, EV_PERSIST | EV_READ, timer_cb, NULL);

    event_add(ev, &tv);
    event_base_dispatch(evbase);
    event_base_free(evbase);
    event_config_free(evcfg);
#else
    struct epoll_event eev;
    int efd = epoll_create(1);
    if (efd == -1) {
        printf("epoll_create() failed: errno=%d\n", errno);
        close(tfd);
        return NULL;
    }
    eev.events = EPOLLIN;
    eev.data.fd = tfd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, tfd, &eev) == -1) {
        printf("epoll_ctl(ADD) failed: errno=%d\n", errno);
        close(efd);
        close(tfd);
        return NULL;
    }
    gettimeofday(&cur_time, NULL);
    gettimeofday(&pre_time, NULL);
    struct timespec tm_sleep;
    tm_sleep.tv_sec = 0;
    tm_sleep.tv_nsec = 5000000;
    while(1) {
#if 0
        ret = epoll_wait(efd, &eev, 1, 1000);
        if (ret < 0) {
            printf("epoll_wait() failed: errno=%d\n", errno);
            close(efd);
            close(tfd);
            return NULL;
        } else if (ret == 0) {
            printf("epoll_wait() timeout, this should not happend\n");
        }

        if(eev.data.fd == tfd) {
            if(eev.events != EPOLLIN) {
                printf("timerfd triggred with strange event=%d\n", eev.events);
            }
            timer_cb (tfd, eev.events, args);
        } else {
            printf("monitored unregisterd fd\n");
        }
#else
        clock_nanosleep( CLOCK_MONOTONIC, 0, &tm_sleep, NULL);
        timer_cb (tfd, eev.events, args);

#endif
    }
    close(efd);
#endif

    close(tfd);

    delete gpio_rt;

    printf("Event thread exit\n");
    return NULL;
}
#define CUSTOM_STACK_SIZE 4096*16
int main(int argc, char* argv[])
{
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
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

    struct stat s;
    if (stat("/dev/cpu_dma_latency", &s) == 0) {
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
    param.sched_priority = 88;
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
    ret = pthread_create(&thread, &attr, thread_func, NULL);
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
    /* Join the thread and wait until it is done */
    ret = pthread_join(thread, NULL);
    if (ret)
        printf("join pthread failed: %m\n");

out:
    close(latency_target_fd);
    printf("Main thread exit\n");
    return ret;
}
