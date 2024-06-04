/*
 * POSIX Real Time Example
 * using a single pthread as RT thread
 */

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <event.h>
#include "gpiodev.h"

static struct timeval g_tv;
static char timer_dmp[8];

/*
 * fd: Timer fd
 * event:
 */

GpioDev *gpio_rt;

static void timer_cb (evutil_socket_t fd, short event, void *args)
{

    read(fd, timer_dmp, 8);
    gpio_rt->toggleValue();
    //gettimeofday(&g_tv, NULL);
    //printf("[%ld.%06ld] timer cb\n", g_tv.tv_sec%100, g_tv.tv_usec);
}

void *thread_func(void *args)
{
    /* Do RT specific stuff here */

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


    close(tfd);

    delete gpio_rt;

    printf("Event thread exit\n");
    return NULL;
}

int main(int argc, char* argv[])
{
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
    int ret;
#if 0
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
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
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
    printf("Main thread exit\n");
    return ret;
}
