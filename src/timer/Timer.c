/*
 * =====================================================================================
 *
 *       Filename:  Timer.c
 *
 *    Description:  implement of Timer manager 
 *
 *        Version:  1.0
 *        Created:  07/19/13 11:00:52
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  huyao (H.Y), yaoshunyuhuyao@163.com
 *        Company:  NDSL
 *
 * =====================================================================================
 */

#include "Timer.h"
#include "../log/Debug.h"
#include "../intmap/intmap.h"
#include "Heap.h"
#include <stdlib.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <inttypes.h>

struct timer
{
    UINT64   expire_time;
    Callback callback;
    void *   data;
    int      index;
    UINT64   interval;
};

struct Intmap;
struct Heap;
struct manager
{
    int     fd;
    int     type;
    Intmap *all_elements;
    Heap   *hp;
    unsigned int expire_counter;
};

#define INIT_SIZE 64

Timer *create_timer_manager(int type)
{
    int flags = TFD_CLOEXEC;
    if(type != 0)
        flags |= TFD_NONBLOCK;

    int fd = timerfd_create(CLOCK_REALTIME , flags) ;
    if(fd < 0)
    {
        LOG_SYSCALL("Create timerfd failed in creating timer ...");
        return NULL;
    }

    Timer *tr = (Timer *)calloc(1 , sizeof(Timer));
    if(NULL == tr)
    {
        LOG_ERROR("Allocate memory %u failed in create timer ..." , sizeof(Timer));
        goto FREE_TIMERFD;
    }
   
    Heap *hp = create_heap(INIT_SIZE , _MIN_ROOT);
    if(NULL == hp)
    {
        LOG_ERROR("Create heap failed in creating timer ...");
        goto FREE_TR;
    }

    Intmap *imap = create_intmap(INIT_SIZE);
    if(NULL == imap)
    {
        LOG_ERROR("Create intmap failed in creating timer ...");
        goto FREE_HEAP;
    }

    tr->fd = fd;
    tr->type = (type ? 1 : 0);
    tr->hp = hp;
    tr->all_elements = imap;

    LOG_INFO_TIME("Create a timer manager with %s successfully ..." , type ? "nonblocking" : "blocking");
    return tr;

FREE_HEAP :
    destory_heap(hp);
    hp = NULL;

FREE_TR :
    free(tr);
    tr = NULL;

FREE_TIMERFD :
    close(fd);
    fd = -1;
    
    return NULL;
}

#define TIMEVAL_TO_WEIGHT(val)  ((val).tv_sec * 1000000 + (val).tv_usec)
#define SECOND_TO_WEIGHT(t)     ((t) * 1000000)

#define CHECK_TIMER_NOT_NULL_VOID(tr) do{ \
    if(NULL == (tr)){ \
        LOG_ERROR("Cannot do %s to a NULL timer ..." , __func__); \
        return ; \
    } \
}while(0)

#define CHECK_TIMER_NOT_NULL(tr , type) do{ \
    if(NULL == (tr)){ \
        LOG_ERROR("Cannot do %s to a NULL timer ..." , __func__); \
        return ((1 == (type)) ? -1 : 0); \
    } \
}while(0)

#define NORMAL_REASON  "no nore alarm"
#define GET_TIME_ERROR "gettimeofday failed"
#define TIMERFD_ERROR  "reset timerfd failed"
#define USER_CANCEL    "cancel this timer"

static int do_reset_timerfd(Timer *tr)
{
    iTimer *root = get_root_value(tr->hp);
    if(NULL == root)
    {
        LOG_WARNING("Current no timer exist when reset timerfd ...");
        struct itimerspec gap;
        gap.it_value.tv_sec = 0;
        gap.it_value.tv_nsec = 0;
        gap.it_interval.tv_sec = 0;
        gap.it_interval.tv_nsec = 0;

        if(timerfd_settime(tr->fd , 0 , &gap , NULL) < 0)
        {
            LOG_SYSCALL("do settime relative failed when reset timerfd ...");
            return -1;
        }
        return 0;
    }
    struct timeval now;
    if(gettimeofday(&now , NULL) < 0)
    {
        LOG_SYSCALL("Gettimeofday failed when reset timerfd ...");
        return -1;
    }

    UINT64 expire = root->expire_time;
    time_t sec = 0;
    long micro = 0;
    //this happens when reset the the timer with a small value ...
    //this time is smaller the time costing in upper operations...
    if(expire <= TIMEVAL_TO_WEIGHT(now))
    {
        micro = now.tv_usec;
        sec = now.tv_sec ;
    }
    else
    {
        sec = (time_t)(expire / 1000000);
        micro = (long)(expire % 1000000);
    }

    struct itimerspec gap;
    gap.it_value.tv_sec = sec;
    gap.it_value.tv_nsec = micro * 1000;
    gap.it_interval.tv_sec = 0;
    gap.it_interval.tv_nsec = 0;

    if(timerfd_settime(tr->fd , TFD_TIMER_ABSTIME , &gap , NULL) < 0)
    {
        LOG_SYSCALL("do settime absolute failed when reset timerfd ...");
        return -1;
    }

    return 0;
}

static void free_timer(Timer *tr , iTimer *itmr , char *reason)
{
    LOG_INFO_TIME("Free timer index %d because %s ..." , itmr->index , reason);
    free_from_heap(tr->hp , itmr->expire_time , itmr);
    intmap_clear_value(tr->all_elements , itmr->index);
    free(itmr);
}

static int change_itimer_inside(Timer *tr , iTimer *itmr , UINT64 next_expire)
{
    iTimer *itr = get_root_value(tr->hp);
    UINT64 old_root_expire = itr->expire_time;
    UINT64 old_expire = itmr->expire_time;
    itmr->expire_time = next_expire;
    modify_on_heap(tr->hp , old_expire , itmr , next_expire , itmr);

    iTimer *root_timer = (iTimer *)get_root_value(tr->hp);
    if(old_root_expire != root_timer->expire_time)
    {
        if(do_reset_timerfd(tr) < 0)
        {
            LOG_ERROR("reset timerfd failed when change timer inside ...");
            free_timer(tr , itmr , TIMERFD_ERROR);
            return -1;
        }
    }

    return 0;
}

static int add_timer_inside(Timer *tr , UINT64 expire , Callback cb , void *para , UINT64 interval)
{
    iTimer *itmr = (iTimer *)calloc(1 , sizeof(iTimer));
    if(NULL == itmr)
    {
        LOG_ERROR("Allocate memory %d failed when add timer ..." , sizeof(iTimer));
        return -1;
    }
    
    struct timeval now;
    if(gettimeofday(&now , NULL) < 0)
    {
        LOG_SYSCALL("gettimeofday failed when add timer inside...");
        return -1;
    }
    itmr->expire_time = expire + TIMEVAL_TO_WEIGHT(now);
    itmr->callback = cb;
    itmr->data = para;
    itmr->interval = interval;

    int index = intmap_put_value(tr->all_elements , itmr);
    if(index < 0)
    {
        LOG_ERROR("Put a timer to intmap failed when add timer ...");
        free(itmr);
        return -1;
    }
    
    iTimer *itr = get_root_value(tr->hp);
    if(insert_to_heap(tr->hp , itmr->expire_time , itmr) < 0)
    {
        LOG_ERROR("Insert a timer weight %llu to heap failed when add timer ..." , 
                expire);
        intmap_clear_value(tr->all_elements , index);
        free(itmr);
        return -1;
    }
    if(itr != get_root_value(tr->hp))
    {
        if(do_reset_timerfd(tr) < 0)
        {
            LOG_ERROR("Reset timerfd failed when add timer ...");
            free_timer(tr , itmr , TIMERFD_ERROR);
            return -1;
        }
    }
    
//    LOG_INFO_TIME("Create a timer index %d ..." , index);
    itmr->index = index;
    return index; 
}

static int do_itimer_expire(Timer *tr , iTimer *itmr)
{
    struct timeval val = (itmr->callback)(itmr->data);
    UINT64 next_interval = itmr->interval;
    UINT64 next_expire = ((next_interval != 0) ? next_interval : TIMEVAL_TO_WEIGHT(val));

    if(0 == next_expire)
    {
        free_timer(tr , itmr , NORMAL_REASON);
        return 0;
    }
    else 
    {
        struct timeval now;
        if(gettimeofday(&now , NULL) < 0)
        {
            LOG_SYSCALL("Gettimeofday failed when do expire timer ...");
            free_timer(tr , itmr , GET_TIME_ERROR);
            return -1;
        }

        next_expire += TIMEVAL_TO_WEIGHT(now);
        iTimer *itr = get_root_value(tr->hp);
        UINT64 old_root_expire = itr->expire_time;
        UINT64 old_expire = itmr->expire_time;
        itmr->expire_time = next_expire;
        modify_on_heap(tr->hp , old_expire , itmr , next_expire , itmr);

        iTimer *root_timer = (iTimer *)get_root_value(tr->hp);
        if(old_root_expire != root_timer->expire_time)
        {
            if(do_reset_timerfd(tr) < 0)
            {
                LOG_ERROR("Reset timerfd failed when do itmer expire , Clear timer %d..." , 
                        itmr->index);
                free_timer(tr , itmr , TIMERFD_ERROR);
                return -1;
            }
        }
    }

    return 0;
}

static void delete_timer_inside(Timer *tr , iTimer *itmr)
{
    free_timer(tr , itmr , USER_CANCEL);
    if(do_reset_timerfd(tr) < 0)
        LOG_FATAL("reset timerfd failed when delete timer caused by user ...");
}

int add_timer(Timer * tr , struct timeval expire , Callback cb , void *para)
{
    CHECK_TIMER_NOT_NULL(tr , 1);
    if(NULL == cb)
    {
        LOG_ERROR("Cannot pass a NULL callback when add timer...");
        return -1;
    }

    return add_timer_inside(tr , TIMEVAL_TO_WEIGHT(expire) , cb , para , 0);
}

int add_definitely_timer(Timer *tr , time_t expire , Callback cb , void *para)
{
    CHECK_TIMER_NOT_NULL(tr , 1);

    time_t now = time(NULL);
    if((now == (time_t)-1))
    {
        LOG_SYSCALL("Get current time failed when create difinitely timer ...");
        return -1;
    }
    if(NULL == cb)
    {
        LOG_ERROR("Cannot pass a NULL callback when add difinitely timer...");
        return -1;
    }
    if(expire <= now)
    {
        LOG_WARNING_TIME("Create a timer expired , callback now ...");
        struct timeval next = cb(para);
        if(TIMEVAL_TO_WEIGHT(next) == 0)
            return -1;
        else 
            return add_timer_inside(tr , TIMEVAL_TO_WEIGHT(next) , cb , para , 0);
    }
    
    return add_timer_inside(tr , SECOND_TO_WEIGHT(expire - now) , cb , para , 0);
}

int add_periodic_timer(Timer *tr , struct timeval value ,  struct timeval interval , Callback cb , void *para)
{
    CHECK_TIMER_NOT_NULL(tr , 1);
    
    if(NULL == cb)
    {
        LOG_ERROR("Cannot pass a NULL callback when add timer...");
        return -1;
    }

    return add_timer_inside(tr , TIMEVAL_TO_WEIGHT(value) , cb , para , TIMEVAL_TO_WEIGHT(interval));
}

int change_timer(Timer *tr , int index , struct timeval expire)
{
    CHECK_TIMER_NOT_NULL(tr , 1);
    void *value = intmap_get_value(tr->all_elements , index);
    if(NULL == value)
    {
        LOG_WARNING("Cannot find index %d in intmap when change timer ..." , index);
        return -1;
    }

    struct timeval now;
    if(gettimeofday(&now , NULL) < 0)
    {
        LOG_SYSCALL("Gettimeofday failed when change timer ...");
        return -1;
    }
    
    iTimer *itmr = (iTimer *)value;
    UINT64 next_expire = TIMEVAL_TO_WEIGHT(expire) + TIMEVAL_TO_WEIGHT(now);

    return change_itimer_inside(tr , itmr , next_expire);
}

int change_definitely_timer(Timer *tr , int index , time_t expire)
{
    CHECK_TIMER_NOT_NULL(tr , 1);
    
    void *value = intmap_get_value(tr->all_elements , index);
    if(NULL == value)
    {
        LOG_WARNING("Cannot find index %d in intmap when change timer ..." , index);
        return -1;
    }
    time_t now = time(NULL);
    if((now == (time_t)-1))
    {
        LOG_SYSCALL("Get current time failed when change difinitely timer ...");
        return -1;
    }

    iTimer *itmr = (iTimer *)value;
    if(expire <= now)
    {
        LOG_WARNING_TIME("Change a timer expired , callback now ...");
        return do_itimer_expire(tr , itmr);
    }
     
    UINT64 next_expire = SECOND_TO_WEIGHT(expire);

    return change_itimer_inside(tr , itmr , next_expire);
}

int ahead_timer(Timer *tr , int index , struct timeval ahead)
{
    CHECK_TIMER_NOT_NULL(tr , 1);
    
    void *value = intmap_get_value(tr->all_elements , index);
    if(NULL == value)
    {
        LOG_WARNING("Cannot find index %d in intmap when ahead timer ..." , index);
        return -1;
    }
    struct timeval now;
    if(gettimeofday(&now , NULL) < 0)
    {
        LOG_SYSCALL("Gettimeofday failed when ahead timer ...");
        return -1;
    }
    iTimer *itmr = (iTimer *)value;
    UINT64 next_expire = itmr->expire_time - TIMEVAL_TO_WEIGHT(ahead);
    if(next_expire < TIMEVAL_TO_WEIGHT(now))
    {
        LOG_WARNING_TIME("Ahead a timer expired , callback now ...");
        return do_itimer_expire(tr , itmr);
    }

    return change_itimer_inside(tr , itmr , next_expire);
}

int delay_timer(Timer *tr , int index , struct timeval delay)
{
    CHECK_TIMER_NOT_NULL(tr , 1);
    
    void *value = intmap_get_value(tr->all_elements , index);
    if(NULL == value)
    {
        LOG_WARNING("Cannot find index %d in intmap when delay timer ..." , index);
        return -1;
    }
    
    iTimer *itmr = (iTimer *)value;
    UINT64 next_expire = itmr->expire_time + TIMEVAL_TO_WEIGHT(delay);

    return change_itimer_inside(tr , itmr , next_expire);
}

void cancel_timer(Timer *tr , int index)
{
    CHECK_TIMER_NOT_NULL_VOID(tr);

    void *value = intmap_get_value(tr->all_elements , index);
    if(NULL == value)
    {
        LOG_WARNING("Cannot find index %d in intmap when cancel timer ..." , index);
        return ;
    }
    
    iTimer *itmr = (iTimer *)value;
    delete_timer_inside(tr , itmr);
}

int expire_now(Timer *tr , int index)
{
    CHECK_TIMER_NOT_NULL(tr , 1);

    void *value = intmap_get_value(tr->all_elements , index);
    if(NULL == value)
    {
        LOG_WARNING("Cannot find index %d in intmap when expire timer ..." , index);
        return -1;
    }
    
    iTimer *itmr = (iTimer *)value;
    
    return do_itimer_expire(tr , itmr);
}

void expire_now_and_delete(Timer *tr , int index)
{
    CHECK_TIMER_NOT_NULL_VOID(tr);

    void *value = intmap_get_value(tr->all_elements , index);
    if(NULL == value)
    {
        LOG_WARNING("Cannot find index %d in intmap when delete and expire timer ..." , index);
        return ;
    }
    
    iTimer *itmr = (iTimer *)value;

    (itmr->callback)(itmr->data);
    delete_timer_inside(tr , itmr);
}

int expire_once(Timer *tr)
{
    CHECK_TIMER_NOT_NULL(tr , 1);

    Heap *heap = tr->hp;
    uint64_t ret = 0;
    if(read(tr->fd , &ret , sizeof(ret)) != sizeof(ret))
    {
        if(tr->type && errno != EAGAIN)
        {
            LOG_SYSCALL_TIME("Do timerfd read failed ...");
            return -1;
        }
    }
    if(ret != 1)
        LOG_WARNING("BUG : Every read should return 1 ... do not miss any timeout...");

    struct timeval now;
    int this_time = 0;
    int flags = 0;
    //cycle do this because the callback function will task some time ...
    while(1)
    {
        if(gettimeofday(&now , NULL) < 0)
        {
            LOG_SYSCALL("Do gettimeofday failed in expire_once ...");
            return -1;
        }
        iTimer *itmr = (iTimer *)get_root_value(heap);
        if(NULL == itmr)
        {
            flags = 1;
            LOG_WARNING_TIME("There are not any timer in heap ...");
            break;
        }
        if((itmr->expire_time > TIMEVAL_TO_WEIGHT(now)))
        {
            if(!this_time)
                LOG_ERROR("Timer expire when not reach timeout : timeout : %llu and timer expire at %llu ..." ,
                    itmr->expire_time , TIMEVAL_TO_WEIGHT(now));
            break;
        }
        
        struct timeval next_expire = (itmr->callback)(itmr->data);
        UINT64 next_interval = itmr->interval;
        if(next_interval != 0)
        {
            next_expire.tv_sec = next_interval / 1000000;
            next_expire.tv_usec = next_interval % 1000000;
        }

        UINT64 next = TIMEVAL_TO_WEIGHT(next_expire);
        //do not alarm again...
        if(0 == next)
        {
            free_timer(tr , itmr , NORMAL_REASON);
        }
        else 
        {
            if(gettimeofday(&now , NULL) < 0)
            {
                LOG_SYSCALL("Do gettimeofday failed in expire_once inside ...");
                return -1;
            }
            next += TIMEVAL_TO_WEIGHT(now);
            itmr->expire_time = next;
            modify_heap_root(heap , next , itmr);
        }
        ++ tr->expire_counter;
        ++ this_time;
    }
    LOG_INFO_TIME("Out expire once while counter %d ..." , this_time);

    //after all timeout timer , do reset timerfd...
    if(!flags && (do_reset_timerfd(tr) < 0))
        LOG_FATAL("reset timerfd failed when check all expire timers ...");

    return this_time;
}

int get_timer_fd(Timer *tr)
{
    CHECK_TIMER_NOT_NULL(tr , 1);

    return tr->fd;
}

UINT64 next_expire_time(Timer *tr)
{
    CHECK_TIMER_NOT_NULL(tr , 3);

    struct itimerspec gap;
    if(timerfd_gettime(tr->fd , &gap) < 0)
    {
        LOG_SYSCALL("do gettime of timerfd failed ...");
        return 0;
    }

    struct timeval val = {gap.it_value.tv_sec , (int)(gap.it_value.tv_nsec / 1000)};
    return TIMEVAL_TO_WEIGHT(val);
}

void destory_timer(Timer *tr)
{
    CHECK_TIMER_NOT_NULL_VOID(tr);

    int len = intmap_length(tr->all_elements);
    if(len != 0)
        LOG_WARNING("There are %d timer last in the timer manager when destory it ..." , len);

    destory_intmap(tr->all_elements);
    tr->all_elements = NULL;
    destory_heap(tr->hp);
    tr->hp = NULL;

    close(tr->fd);
}

int tm_to_seconds(int year , int month , int day , int hour , int min , int sec , time_t *seconds)
{
    if(NULL == seconds)
        return -1;

    struct tm target;
    memset(&target , 0 ,sizeof(target));
    target.tm_year = year - 1900;
    target.tm_mon = month - 1;
    target.tm_mday = day;
    target.tm_hour = hour;
    target.tm_min = min;
    target.tm_sec = sec;

    time_t tm = mktime(&target);
    if((time_t)-1 == tm)
    {
        LOG_ERROR("Convert %d-%d-%d %d:%d:%d to seconds(time_t) failed ..." ,
                year , month , day , hour , min , sec);
        
        return -1;
    }
    
    *seconds = tm;
    return 0;
}

int tm_to_timeval(int year , int month , int day , int hour , int min , int sec , struct timeval *val)
{
    if(NULL == val)
        return -1;

    struct tm target;
    memset(&target , 0 ,sizeof(target));
    target.tm_year = year - 1900;
    target.tm_mon = month - 1;
    target.tm_mday = day;
    target.tm_hour = hour;
    target.tm_min = min;
    target.tm_sec = sec;

    time_t tm = mktime(&target);
    if((time_t)-1 == tm)
    {
        LOG_ERROR("Convert %d-%d-%d %d:%d:%d to seconds(time_t) failed ..." ,
                year , month , day , hour , min , sec);
        
        return -1;
    }

    val->tv_sec = tm;
    val->tv_usec = 0;
    return 0;
}

int apart_from_now(int sec , int micro , struct timeval *value)
{
    if(NULL == value)
        return -1;

    struct timeval now;
    if(gettimeofday(&now , NULL) < 0)
    {
        LOG_SYSCALL("Gettimeofday failed when calculate apart time ...");
        return -1;
    }

    if(micro >= 1000000)
    {
        sec += micro / 1000000;
        micro = micro % 1000000;
    }
    value->tv_sec = now.tv_sec + sec;
    value->tv_usec = now.tv_usec + micro;
    if(value->tv_usec >= 1000000)
    {
        value->tv_sec += (value->tv_usec / 1000000);
        value->tv_usec += (value->tv_usec % 1000000);
    }

    return 0;
}

