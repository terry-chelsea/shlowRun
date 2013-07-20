/*
 * =====================================================================================
 *
 *       Filename:  expire_some_test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/20/13 21:01:44
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

#include "Timer.h"
#include "Debug.h"

//create some timer and will expire at the same time ...

static int is_odd(int num)
{
    int i = 0;
    int half = sqrt(num) + 1;
    for(i = 2 ; i < half ; ++ i)
    {
        if(num % i == 0)
            return 0;
    }

    return 1;
}

static int get_odd_counter(int num)
{
    if(num < 2)
        return 0;

    int i = 0;
    int counter = 1;
    for(i = 3 ; i <= num ; ++ i)
    {
        if(!(i & 0x1))
            continue ;

        if(is_odd(i))
            ++counter;
    }
    
    return counter;
}

#define NUMS 6
#define INIT_TIMES  3
struct Para 
{
    struct timeval set_time;
    struct timeval expire_time;
    int  counter;
    int  index;
};
struct Para all_infos[NUMS];

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
        int tst = rand() % 1000000;
        get_odd_counter(tst);

        struct timeval now;
        gettimeofday(&now , NULL);
        p->set_time = now;
        struct timeval others = {2 , 0};
        p->expire_time = others;

        return others;
    }
    else 
    {
        struct timeval zero = {0 , 0};
        return zero;
    }
}

int main()
{
    START_DEBUG(NULL , 0 , 0);

    Timer *Tmanager = create_timer_manager(0);
    if(NULL == Tmanager)
    {
        LOG_INFO("Create new timer manager failed ...");
        return -1;
    }

    int i = 0;
    for(i = 0 ; i < NUMS ; ++ i)
    {
        struct timeval now;
        gettimeofday(&now , NULL);
        struct timeval first = {1 , 0};
        all_infos[i].set_time = now;
        all_infos[i].expire_time = first;
        all_infos[i].counter = INIT_TIMES;
        int index = add_timer(Tmanager , first , all_callback , (void *)(all_infos + i));
        if(index < 0)
        {
            LOG_ERROR("Create new timer failed of index %d ..." , i);
            return -1;
        }
        all_infos[i].index = index;
    }

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

