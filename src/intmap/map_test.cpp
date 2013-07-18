/*
 * =====================================================================================
 *
 *       Filename:  map_test.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/18/13 10:06:29
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include <map>
#include <cstdlib>
#include "Debug.h"

using namespace std;

typedef struct item
{
    int index;
    void *value;
}ITEM;

typedef map<int , void *> MapType;

typedef struct 
{
    MapType mp;
    int  cur_index;
}MAP;

MAP *create_map(int init_sz)
{
    MAP *mp = new MAP;
    if(NULL == mp)
    {
        LOG_ERROR("Create new map failed ...");
        return NULL;
    }
    mp->cur_index = 0;

    return mp;
}

int map_put_value(MAP *mp , void *value)
{
    if(NULL == mp)
    {
        LOG_ERROR("map is NULL ...");
        return -1;
    }
    int index = mp->cur_index;
    pair<MapType::iterator,bool> ret = mp->mp.insert(make_pair(index , value));
    while(!(ret.second))
    {
        ++ index;
        ret = mp->mp.insert(make_pair(index , value));
    }

    mp->cur_index = index + 1;
    return index;
}

void * map_get_value(MAP *mp , int index)
{
    if(NULL == mp)
    {
        LOG_ERROR("map is NULL ...");
        return NULL;
    }

    MapType::iterator ret = mp->mp.find(index);
    if(ret == mp->mp.end())
    {
        LOG_ERROR("Can not find this index %d in map ..." , index);
        return NULL;
    }

    return ret->second;
}

int map_clear_value(MAP *mp , int index)
{
    if(NULL == mp)
    {
        LOG_ERROR("map is NULL ...");
        return -1;
    }

    MapType::iterator ret = mp->mp.find(index);
    if(ret == mp->mp.end())
        return -1;
    
    mp->mp.erase(ret);
    return 0;
}

int map_size(MAP *mp)
{
    if(NULL == mp)
    {
        LOG_ERROR("map is NULL ...");
        return -1;
    }

    return mp->mp.size();
}

void destory_map(MAP *mp)
{
    if(NULL == mp)
        return ;
    
    int size = mp->mp.size();
    if(size != 0)
        LOG_INFO("There are %d elements in map ..." , size);

    mp->mp.clear();
    free(mp);
    mp = NULL;
}


int test_map()
{
#define TEST_TIMES    4096
    ITEM all_info[TEST_TIMES * 2];
    START_DEBUG(NULL , OUT_SCR , DEBUG);

    MAP *mp = create_map(0);
    if(NULL == mp)
    {
        LOG_ERROR("Create a map failed ...");
        return -1;
    }

    int i = 0;
    for(i = 0 ; i < TEST_TIMES ; ++ i)
    {
        int index = map_put_value(mp , (void *)(i + 1));
        if(index < 0)
        {
            LOG_ERROR("put a value %d to map failed ..." , i + 1);
            return -1;
        }
        if(index == 498)
        {
            int k = 0;
        }

        all_info[i].index = index;
        all_info[i].value = (void *)(i + 1);
    }

    for(i = 0 ; i < (TEST_TIMES >> 1) ; ++ i)
    {
        int rad = rand() % TEST_TIMES;
        void *value = map_get_value(mp , all_info[rad].index);
        while(NULL == value)
        {
            rad = rand() % TEST_TIMES;
            value = map_get_value(mp , all_info[rad].index);
        }

        if(value != all_info[rad].value)
        {
            LOG_WARNING("put value %d not equal to get value %d ..." , 
                    all_info[rad].value , value);
            return -1;
        }

        if(map_clear_value(mp , all_info[rad].index) < 0)
        {
            LOG_ERROR("Clear index %d failed ..." , all_info[rad].index);
            return -1;
        }
        all_info[rad].value = NULL;
    }

    for(i = TEST_TIMES ; i < (TEST_TIMES << 1) ; ++ i)
    {
        int index = map_put_value(mp , (void *)(i + 1));
        if(index < 0)
        {
            LOG_ERROR("put a value %d to map failed ..." , i + 1);
            return -1;
        }

        all_info[i].index = index;
        all_info[i].value = (void *)(i + 1);
    }
    for(i = 0 ; i < (TEST_TIMES << 1) ; ++ i)
    {
        int index = all_info[i].index;
        void *value = (all_info[i].value);

        if(value != NULL)
        {
            if(map_get_value(mp , index) == NULL)
            {
                LOG_INFO("This index %d should be not NULL ..." , index);
                return -1;
            }

        }
        /*  
        if(NULL == value && map_get_value(mp , index) != NULL)
        {
            LOG_INFO("This index %d should be NULL ..." , index);
            return -1;
        }
        if(value != NULL && map_get_value(mp , index) == NULL)
        {
            LOG_INFO("This index %d should be not NULL ..." , index);
            return -1;
        }
        */
    }

    LOG_INFO("all TEST finish , current size : %d ..." , map_size(mp));
    destory_map(mp);

    FINISH_DEBUG();
    return 0;
}

#define MAP_TEST     (102400)
#define CLEAR_TIMES  (MAP_TEST >> 1)
#define GET_TIMES    10
#define MAX_VALUE    1024000

ITEM all_infos[MAP_TEST];
int clear_indexs[CLEAR_TIMES];

int fetch_value(MAP *mp , int times)
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
        
        void *value = map_get_value(mp , index);
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
#define CALCULATE_TIME(start , end) do{ \
    double gap = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec); \
    double msec = gap / 1000; \
    printf("%lf  " , msec); \
}while(0)

//printf("From start to end , Cost %lf millseconds\n" , msec); \



int map_performance_test()
{
    struct timeval start , end;
    struct timeval all_start , all_end;

    START_TIME(all_start);
    srand(10000283);
    MAP *mp = create_map(0);
    if(NULL == mp)
    {
        LOG_ERROR("Create map failed ...");
        return -1;
    }
//    LOG_INFO("Create map success...");

    START_TIME(start);
    int i = 0;
    for(i = 0 ; i < MAP_TEST ; ++ i)
    {
        void *value = (void *)(rand() % MAX_VALUE); 
        int index = map_put_value(mp , value);
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

    if(fetch_value(mp , GET_TIMES * MAP_TEST) < 0)
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
        if(map_clear_value(mp , index) < 0)
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

    if(fetch_value(mp , CLEAR_TIMES * GET_TIMES) < 0)
        return -1;

//    LOG_INFO("Fetch value %d times success ..." , GET_TIMES * CLEAR_TIMES);

    END_TIME(end);
    CALCULATE_TIME(start , end);
    START_TIME(start);

    for(i = 0 ; i < CLEAR_TIMES ; ++ i)
    {
        int sub_index = clear_indexs[i];
        void *value = (void *)(rand() % MAX_VALUE);
        int index = map_put_value(mp , value);
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

    if(fetch_value(mp , MAP_TEST * GET_TIMES) < 0)
        return -1;

//    LOG_INFO("Fetch value %d times success ..." , GET_TIMES * MAP_TEST);

    END_TIME(end);
    CALCULATE_TIME(start , end);
    START_TIME(start);

    for(i = 0 ; i < MAP_TEST ; ++ i)
    {
        int index = all_infos[i].index;
//        LOG_WARNING("start clear item %d i is %d ..." , index , i);
        if(map_clear_value(mp , index) < 0)
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

    destory_map(mp);

//    LOG_INFO("All test finish...");

    END_TIME(all_end);
    CALCULATE_TIME(all_start , all_end);
    
    printf("\n");
    return 0;
}

int main()
{
    START_DEBUG(NULL , OUT_SCR , DEBUG);

    if(map_performance_test() < 0)
    {
        LOG_ERROR("preformance test failed ...");
    }
    else 
        LOG_INFO("Preformance test success ...");

    FINISH_DEBUG();
}
