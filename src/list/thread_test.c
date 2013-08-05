/*
 * =====================================================================================
 *
 *       Filename:  thread_test.c
 *
 *    Description:  test normal list with signal thread and mitilthread...
 *
 *        Version:  1.0
 *        Created:  07/10/13 23:02:34
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  huyao (H.Y), yaoshunyuhuyao@163.com
 *        Company:  NDSL
 *
 * =====================================================================================
 */

#include "queue.h"
#include <sys/time.h>
#include <stdlib.h>

#define CALCULATE_TIME(start , end) do{ \
    double gap = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec); \
    double msec = gap / 1000; \
    printf("From start to end , Cost %lf millseconds\n" , msec); \
}while(0)

#define DEBUG_
#define THREAD_NUM 6

#define THREAD_TEST_NUM 500000
#define TEST_NUM (THREAD_NUM * THREAD_TEST_NUM)
#define COUNTER  (TEST_NUM / 10)

#ifndef  THREADS
int main()
{
    struct timeval start , end;
    gettimeofday(&start , NULL);

    QUEUE *lst = queue_create(0);
    if(NULL == lst)
        return -1;

    queue_info(lst);

    int i = 0;
    for(i = 0 ; i < TEST_NUM ; ++ i)
    {
        unsigned long ran = rand() % 1000000;
#ifdef DEBUG
        if(i > 0 && !(i % COUNTER))
            queue_info(lst);
#endif

        if(queue_put(lst , (void *)ran) < 0)
            return -1;

    }

    for(i = 0 ; i < TEST_NUM ; ++ i)
    {
#ifdef DEBUG
        if(i > 0 && !(i % COUNTER))
            queue_info(lst);
#endif

        void* ret = NULL;
        if(queue_get(lst , &ret) < 0)
            return -1;
    }
    queue_destory(&lst);

    gettimeofday(&end , NULL);
    
    CALCULATE_TIME(start , end);

    return 0;
}

#else

#include <signal.h>
static pthread_t main_thread_tid = 0;
static QUEUE *main_list = NULL;
#define NOTICE_SIGNAL SIGUSR1

void *start_task(void *args)
{
    QUEUE *lst = (QUEUE *)args;

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set , NOTICE_SIGNAL);
    pthread_sigmask(SIG_BLOCK , &set , NULL);

    int i = 0;
    for(i = 0 ; i < THREAD_TEST_NUM ; ++ i)
    {
        unsigned long num = rand() % 1000000;
        if(queue_put(lst , (void *)num) < 0)
            printf("error happened when put an element !!!\n");
    }

    pthread_kill(main_thread_tid , NOTICE_SIGNAL) ;
    
    for(i = 0 ; i < THREAD_TEST_NUM ; ++ i)
    {
        void * num = 0;
        if(queue_get(lst , &num) < 0)
        {
            printf("error happened when get an element !!!\n");
            queue_info(lst);
        }
    }

    return NULL;
}

void notice_func(int signo)
{
#ifdef DEBUG
    printf("***********Notice by Signal : **************\n");
    queue_info(main_list);
#endif
}

int main()
{
    pthread_t tids[THREAD_NUM];
    struct timeval start , end;
    gettimeofday(&start , NULL);
    
    main_thread_tid = pthread_self();
    signal(SIGUSR1 , notice_func);

    main_list = queue_create(1);
    if(NULL == main_list)
        return -1;

    int i = 0;
    for(i = 0 ; i < THREAD_NUM ; ++ i)
    {
        if(pthread_create(tids + i , NULL , start_task , (void *)main_list) < 0)
        {
            perror("Create thread failed : ");
            return -1;
        }
    }

    for(i = 0 ; i < THREAD_NUM ; ++ i)
    {
        pthread_join(tids[i] , NULL);
#ifdef DEBUG
        printf("-----------thread %d exit : -----------\n" , i);
        queue_info(main_list);
#endif
    }

    queue_destory(&main_list);

    gettimeofday(&end , NULL);
    
    CALCULATE_TIME(start , end);
    return 0;
}

#endif
