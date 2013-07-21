/*
 * =====================================================================================
 *
 *       Filename:  Timer.h
 *
 *    Description:  timer component implement with heap and intmap...
 *
 *        Version:  1.0
 *        Created:  07/19/13 10:02:39
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#ifndef _H_TERRY_TIMER_H_
#define _H_TERRY_TIMER_H_

#include <sys/time.h>
#include "Heap.h"

/* 
 * The timer manager unit is implement with timerfd , use Heap to find the latest 
 * timer in O(lg n) time , Use intmap to store all timer and return a handle 
 * representate the timer to caller. this is efficiently too ...
 * However , change timer and delete timer (ahead and delay) may be O(n) operation...
 * because it will have to find the timer in heap (in other way , you can store the index
 *  of the timer in intmap . this is viable but complicated , DO NOT do it...)
 * In normal use , you just want to add timer and wait for timeout ...
 * What is the most improtant , You should implement your callback function in this 
 * restrain : the return value is  the next relative timeout of this timer , return a 
 * (0 , 0) timerval means no more alarm in this timer ...
 * you should always remember this when use the timer manager ...
 * enjoy yourself ...
 *
 */

typedef struct timeval (*Callback)(void *);

typedef struct timer iTimer;

typedef struct manager Timer;

#define RETURN_NO_MORE_TIMER  do{ \
    struct timeval val = {0 , 0}; \
    return val; \
}while(0)

//create function , return Timer handler or NULL...
Timer *create_timer_manager(int type);

//add a timer of relative timer relatived to now ... gap is expire...
//return value is the index of this timer ...
int add_timer(Timer * , struct timeval expire , Callback cb , void *para);

//add repeated timer like timerfd interval something ...
int add_periodic_timer(Timer *tr , struct timeval value ,  struct timeval interval , Callback , void *para);

//add a definitely timer , it will expire at expire , if it ahead of now , do timer
//expire right now ...
int add_definitely_timer(Timer * , time_t expire , Callback cb , void *para);

//change a timer . if not exist , no error ...
//otherewise change the timer expire at expire(parameter) relative to now...
int change_timer(Timer * , int index , struct timeval expire);

//change a timer to a definitely timer , will expire at expire(parameter) ...
int change_definitely_timer(Timer * , int index , time_t expire);

//change a timer ahead of time to privious expire time ...
int ahead_timer(Timer *tr , int index , struct timeval ahead);

//change a timer delay to delay time relative to privious expire time ...
int delay_timer(Timer *tr , int index , struct timeval delay);

//cancel a timer , do not call callback function ...
void cancel_timer(Timer * , int index);

//do call the callback function immediately ...
int expire_now(Timer * , int index);

//do call the callback and delete the timer ...
void expire_now_and_delete(Timer * , int index);

//a timeout happened ... check expired timers and do their callbacks ...
int expire_once(Timer *);

//get next expire time ...
UINT64 next_expire_time(Timer *tr);

//return the fd of timerfd ...
int get_timer_fd(Timer *);

//change a day time to time_t ...
int tm_to_seconds(int year , int month , int day , int hour , int min , int sec , time_t *seconds);

//change a day time to timeval ...
int tm_to_timeval(int year , int month , int day , int hour , int min , int sec , struct timeval *val);

//return the timeval apart from now , gap is sec seconds and micro microseconds...
int apart_from_now(int sec , int micro , struct timeval *val);

//destory the timer menager ...
void destory_timer(Timer *);

#endif
