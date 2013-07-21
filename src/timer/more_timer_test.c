/*
 * =====================================================================================
 *
 *       Filename:  more_timer_test.c
 *
 *    Description:  test timer manager with more timers ...
 *
 *        Version:  1.0
 *        Created:  07/20/13 20:03:30
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

//create some timers and cycle timeout , check expire time

#include <stdio.h>
#include "Debug.h"
#include "Timer.h"
#include <signal.h>
#include <time.h>

#define NUMS  100
#define INIT_TIMES  2

struct Para 
{
    struct timeval set_time;
    struct timeval expire_time;
    int  counter;
    int  index;
};

static int cur_index = 0;
struct Para all_infos[NUMS];
Timer *Tmanager = NULL;
struct Para special_one;
struct timeval expiod_value = {4 , 0};
struct timeval expiod_interval = {1 , 0};

struct timeval all_callback(void *para)
{
    struct Para *p = (struct Para *)para;
    time_t sec = p->set_time.tv_sec + p->expire_time.tv_sec;
    long micro = p->set_time.tv_usec + p->expire_time.tv_usec;
    if(micro >= 1000000)
    {
        sec ++ ;
        micro -= 1000000;
    }
    
    struct tm tm_value;
    gmtime_r(&sec , &tm_value);
    LOG_INFO_TIME("Timer %d should print at %d:%d:%d:%ld ..." , p->index ,
            tm_value.tm_hour , tm_value.tm_min , tm_value.tm_sec , micro);

    p->counter --;
    if(p->counter > 0)
    {
        struct timeval now;
        gettimeofday(&now , NULL);
        p->set_time = now;
        struct timeval others = {100 , 0};
        p->expire_time = others;

        return others;
    }
    else 
        RETURN_NO_MORE_TIMER;
}

struct timeval special_callback(void *para)
{
    struct Para *p = (struct Para *)para;
    time_t sec = p->set_time.tv_sec + p->expire_time.tv_sec;
    long micro = p->set_time.tv_usec + p->expire_time.tv_usec;
    if(micro >= 1000000)
    {
        sec ++ ;
        micro -= 1000000;
    }
    
    struct tm tm_value;
    gmtime_r(&sec , &tm_value);
    LOG_INFO_TIME("Timer %d should print at %d:%d:%d:%ld ..." , p->index ,
            tm_value.tm_hour , tm_value.tm_min , tm_value.tm_sec , micro);
    
    struct timeval now;
    gettimeofday(&now , NULL);
    p->set_time = now;
    p->expire_time = expiod_interval;

    struct timeval tm = {100 , 0};
    return tm;
}

void sig_alrm(int signo)
{
    struct timeval now;
    gettimeofday(&now , NULL);
    struct timeval first = {1 , 0};
    all_infos[cur_index].set_time = now;
    all_infos[cur_index].expire_time = first;
    all_infos[cur_index].counter = INIT_TIMES;
    int index = add_timer(Tmanager , first , all_callback , (void *)(all_infos + cur_index));
    if(index < 0)
    {
        LOG_ERROR("Create new timer failed of index %d ..." , cur_index);
        return ;
    }
    all_infos[cur_index].index = index;
    ++cur_index;

    if(cur_index < NUMS)
        alarm(1);
}

int main()
{
    START_DEBUG(NULL , 0 , 0);

    Tmanager = create_timer_manager(0);
    if(NULL == Tmanager)
    {
        LOG_INFO("Create new timer manager failed ...");
        return -1;
    }

    if(signal(SIGALRM , sig_alrm) < 0)
    {
        LOG_SYSCALL("set signal alarm callback failed ...");
        return -1;
    }

    struct timeval now;
    gettimeofday(&now , NULL);

    struct timeval expiod_value = {4 , 0};
    struct timeval expiod_interval = {1 , 0};
    int index = add_periodic_timer(Tmanager , expiod_value , expiod_interval , special_callback , &special_one);
    if(index < 0)
    {
        LOG_ERROR("Create periodic timer failed ...");
        return -1;
    }
    special_one.set_time = now;
    special_one.expire_time = expiod_value;
    special_one.counter = 0;
    special_one.index = index;
    
    struct timeval first = {1 , 0};
    all_infos[cur_index].set_time = now;
    all_infos[cur_index].expire_time = first;
    all_infos[cur_index].counter = INIT_TIMES;
    index = add_timer(Tmanager , first , all_callback , (void *)(all_infos + cur_index));
    if(index < 0)
    {
        LOG_ERROR("Create new timer failed of index %d ..." , cur_index);
        return -1;
    }
    all_infos[cur_index].index = index;
    ++cur_index;

    alarm(1);
    while(1)
    {
        if(expire_once(Tmanager) < 0)
        {
            LOG_ERROR("Timer manager expire once failed ...");
            return -1;
        }
    }

    destory_timer(Tmanager);
    FINISH_DEBUG();
    return 0;
}

