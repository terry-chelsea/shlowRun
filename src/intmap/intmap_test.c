/*
 * =====================================================================================
 *
 *       Filename:  intmap_test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/19/13 10:33:25
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  huyao (H.Y), yaoshunyuhuyao@163.com
 *        Company:  NDSL
 *
 * =====================================================================================
 */


#include "Debug.h"
#include "intmap.h"
#include <stdlib.h>
#include <string.h>

typedef struct item
{
    int index;
    void *value;
}ITEM;


#define MAP_TEST     (102400)
#define CLEAR_TIMES  (MAP_TEST >> 1)
#define GET_TIMES    10
#define MAX_VALUE    1024000

ITEM all_infos[MAP_TEST];
int clear_indexs[CLEAR_TIMES];

int fetch_value(Intmap *imap , int times)
{
    int i = 0;
    for(i = 0 ; i < times ; ++ i)
    {
        int sub_index = rand() % MAP_TEST;
        int index = all_infos[sub_index].index;
        if(index == -1)
        {
            -- i;
            continue ;
        }
        
        void *value = intmap_get_value(imap , index);
        if(value != all_infos[sub_index].value)
        {
            LOG_ERROR("error happened when fetch value of index %d ..." , sub_index);
            return -1;
        }
    }

    return 0;
}

//put MAP_TEST times ...
//then get every value GET_TIMES times...
//then clear CLEAR_TIMES value random...
//then get last every value GET_TIMES times...
//then put CLEAR_TIMES again...
//then get last every value GET_TIMES times...
//then clear all value...
//destory map...

#define START_PTR   ((void *)0X0)

#include <sys/time.h>
#define START_TIME(start)  gettimeofday(&start , NULL)
#define END_TIME(end) gettimeofday(&end , NULL);
//    printf("From start to end , Cost %lf millseconds\n" , msec); 
#define CALCULATE_TIME(start , end) do{ \
    double gap = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec); \
    double msec = gap / 1000; \
    printf("%lf  " , msec); \
}while(0)

int intmap_performance_test()
{
    struct timeval start , end;
    struct timeval all_start , all_end;

    START_TIME(all_start);
    srand(10000283);
    Intmap *imap = create_intmap(0);
    if(NULL == imap)
    {
        LOG_ERROR("Create intmap failed ...");
        return -1;
    }
//    LOG_INFO("Create map success...");

    START_TIME(start);

    int i = 0;
    for(i = 0 ; i < MAP_TEST ; ++ i)
    {
        void *value = (void *)(rand() % MAX_VALUE); 
        int index = intmap_put_value(imap , value);
        if(index < 0)
        {
            LOG_ERROR("error happened when first put...");
            return -1;
        }
        all_infos[i].index = index;
        all_infos[i].value = value;
    }
//    LOG_INFO("Put %d item to map success..." , MAP_TEST);

    END_TIME(end);
    CALCULATE_TIME(start , end);
    START_TIME(start);

    if(fetch_value(imap , GET_TIMES * MAP_TEST) < 0)
        return -1;

//    LOG_INFO("Fetch value %d times success ..." , GET_TIMES * MAP_TEST);

    END_TIME(end);
    CALCULATE_TIME(start , end);
    START_TIME(start);
    
    for(i = 0 ; i < CLEAR_TIMES ; ++ i)
    {
        int sub_index = rand() % MAP_TEST;
        int index = all_infos[sub_index].index;
        if(index == -1)
        {
            -- i;
            continue ;
        }
        if(intmap_clear_value(imap , index) < 0)
        {
            LOG_ERROR("error happened when clear value of index : %d ..." , index);
            return -1;
        }
        clear_indexs[i] = sub_index;
        all_infos[sub_index].index = -1;
        all_infos[sub_index].value = NULL;
    }

//    LOG_INFO("Clear value %d times success ..." , CLEAR_TIMES);

    END_TIME(end);
    CALCULATE_TIME(start , end);
    START_TIME(start);

    if(fetch_value(imap , CLEAR_TIMES * GET_TIMES) < 0)
        return -1;

//    LOG_INFO("Fetch value %d times success ..." , GET_TIMES * CLEAR_TIMES);

    END_TIME(end);
    CALCULATE_TIME(start , end);
    START_TIME(start);

    for(i = 0 ; i < CLEAR_TIMES ; ++ i)
    {
        int sub_index = clear_indexs[i];
        void *value = (void *)(rand() % MAX_VALUE);
        int index = intmap_put_value(imap , value);
        if(index < 0)
        {
            LOG_ERROR("error happened when second put ...");
            return -1;
        }
        all_infos[sub_index].index = index;
        all_infos[sub_index].value = value;
    }

//    LOG_INFO("Reput item %d times success ..." , CLEAR_TIMES);

    END_TIME(end);
    CALCULATE_TIME(start , end);
    START_TIME(start);

    if(fetch_value(imap , MAP_TEST * GET_TIMES) < 0)
        return -1;

//    LOG_INFO("Fetch value %d times success ..." , GET_TIMES * MAP_TEST);

    END_TIME(end);
    CALCULATE_TIME(start , end);
    START_TIME(start);

    for(i = 0 ; i < MAP_TEST ; ++ i)
    {
        int index = all_infos[i].index;
//        LOG_WARNING("start clear item %d i is %d ..." , index , i);
        if(intmap_clear_value(imap , index) < 0)
        {
            LOG_ERROR("error happened when second clear of index %d ..." , index);
            return -1;
        }

//        LOG_WARNING("clear index %d success..." , index);

        all_infos[i].index = -1;
        all_infos[i].value = NULL;
    }
//    LOG_INFO("Clear all items success...");

    END_TIME(end);
    CALCULATE_TIME(start , end);

    destory_intmap(imap);
    imap = NULL;
    
//    LOG_INFO("All test finish...");

    END_TIME(all_end);
    CALCULATE_TIME(all_start , all_end);

    printf("\n");

    return 0;
}

int main()
{
    START_DEBUG(NULL , 0 , 0);

    if(intmap_performance_test() < 0)
    {
        LOG_ERROR("preformance test failed ...");
    }
    else 
        LOG_INFO("Preformance test success ...");
    FINISH_DEBUG();

    return 0;
}

