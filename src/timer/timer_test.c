/*
 * =====================================================================================
 *
 *       Filename:  timer_test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/20/13 01:21:12
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include "Timer.h"
#include "Debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PRINT_NOW(info)   do{ \
    struct timeval now; \
    gettimeofday(&now , NULL); \
    printf("%s : %d.%d\n" , info , (int)(now.tv_sec) , (int)(now.tv_usec)); \
}while(0)

struct para
{
    struct timeval next;
    int time;
};

struct timeval test_cb(void *para)
{
    struct para *Para = (struct para *)para;
    struct timeval val = Para->next;
    printf("in callback , val : %d.%d\n" , (int)(val.tv_sec) , (int)(val.tv_usec));

    int tm = (Para->time) -- ; 
    if(3 == tm)
    {
        PRINT_NOW("First timeout ");
        struct timeval ret = {0 , 10};
        return ret;
    }
    else if(2 == tm)
    {
        PRINT_NOW("Second timeout ");
        struct timeval ret = {2 , 0};
        return ret;
    }
    else if(1 == tm)
    { 
        PRINT_NOW("Third timeout ");
        struct timeval ret = {1 , 0};
        return ret;
    }

    PRINT_NOW("Last timeout ");
    struct timeval ret = {0 , 0};
    return ret;
}

int main()
{
    START_DEBUG(NULL , 0 , 0);
    Timer *timer = create_timer_manager(0);
    if(NULL == timer)
    {
        printf("Create timer manager failed ...\n");
        return -1;
    }

    struct para *val = (struct para *)malloc(sizeof(*val));
    if(NULL == val)
    {
        printf("Allocate memory failed ...\n");
        return -1;
    }
    (val->next).tv_sec = 10;
    (val->next).tv_usec = 1000;
    val->time = 3;

    int index = add_timer(timer , val->next , test_cb , (void *)val);
    if(index < 0)
    {
        printf("Create a timer failed ...\n");
        return -1;
    }
    printf("Create relative timer index %d\n" , index);
    PRINT_NOW("After create timer");

//    expire_now_and_delete(timer , index);
    struct timeval ahead = {3 , 1000};
    ahead_timer(timer , index , ahead);
    struct timeval delay = {6 , 0};
    delay_timer(timer , index , delay);
    change_definitely_timer(timer , index , time(NULL) + 10);
    struct timeval change = {5 , 2000};
    change_timer(timer , index , change);
    while(1)
    {
        expire_once(timer);
    }

    destory_timer(timer);
    FINISH_DEBUG();
    return 0;
}
