/*
 * =====================================================================================
 *
 *       Filename:  intmap.c
 *
 *    Description:  the implement of int to pointer map...
 *
 *        Version:  1.0
 *        Created:  07/17/13 01:27:30
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include "intmap.h"
#include "Debug.h"
#include <stdlib.h>
#include <string.h>

struct map
{
    void **array;   //the array of all items...
    int  length;    //current length(used) of this array...
    int  size;      //current size(used + unused) of this array...
    //next two fileds is allocated together , and their size is 
    //related to size filed...
    unsigned long *bitmap;    //use bitmap to find next index...
    unsigned long *second_bitmap;    //second index of bitmap...
    int second_bitmap_size;
    int bitmap_size;
};

#define LONG_SIZE     (sizeof(unsigned long) * 8)
#define ALIGN_SIZE    64
#define INIT_SIZE     64
#define ALIGN_TO(sz)  ((sz) % ALIGN_SIZE ?  \
        ((sz) & ~(ALIGN_SIZE - 1)) + ALIGN_SIZE : \
        sz)
#define MAX_EXPAND_SIZE   4096
#define EXPAND_STEP       256
#define SHRINK_SIZE       1024

#define BITMAP_SIZE(sz)    ((sz) % LONG_SIZE ? \
        (sz) / LONG_SIZE + 1 : (sz) / LONG_SIZE)
#define SECOND_BITMAP_SIZE(sz)  (BITMAP_SIZE(BITMAP_SIZE(sz)))

static unsigned long MAX_LONG = (0x1UL << LONG_SIZE) - 1;

static int allocate_intmap(Intmap *imap , int sz)
{
    void **all = (void **)malloc(sizeof(void *) * sz);
    if(NULL == all)
    {
        LOG_ERROR("Allocate memory %d for all array failed..." , sizeof(void *) * sz);
        return -1;
    }
    int i = 0;
    for(i = 0 ; i < sz ; ++ i)
        all[i] = NULL;

    //bitmap and second bitmap is allocate together...
    int bitmap_size = BITMAP_SIZE(sz);
    int second_bitmap_size = SECOND_BITMAP_SIZE(sz);
    int all_bitmap_size = bitmap_size + second_bitmap_size;
    unsigned long *bmp = (unsigned long *)malloc(sizeof(unsigned long) * all_bitmap_size);
    if(NULL == bmp)
    {
        LOG_ERROR("Allocate memory %d for all bitmap failed..." , 
                sizeof(unsigned long) * all_bitmap_size);
        free(all);
        all = NULL ;
        return -1;
    }
    for(i = 0 ; i < all_bitmap_size ; ++ i)
        bmp[i] = 0x0UL;
    
    imap->array = all;
    imap->length = 0;
    imap->size = sz;
    imap->second_bitmap = bmp;
    imap->bitmap = bmp + second_bitmap_size;
    imap->second_bitmap_size = second_bitmap_size;
    imap->bitmap_size = bitmap_size;

    return 0;
}

Intmap *create_intmap(int init_sz)
{
    if(init_sz < 0)
        init_sz = 0;

    if(init_sz > 0)
        init_sz = ALIGN_TO(init_sz);

    Intmap *imap = (Intmap *)calloc(1 , sizeof(Intmap));
    if(NULL == imap)
    {
        LOG_ERROR("Allocate memory %d for Intmap failed..." , sizeof(Intmap));
        return NULL;
    }

    if(0 == init_sz)
    {
        imap->array = NULL;
        imap->length = 0;
        imap->size = 0;
        imap->bitmap = imap->second_bitmap = NULL;
        imap->bitmap_size = imap->second_bitmap_size = 0;
        return imap;
    }
    
    if(allocate_intmap(imap , init_sz) < 0)
    {
        LOG_ERROR("Allocate intmap for size %d failed ..." , init_sz);
        return NULL;
    }
    
    return imap;
}

//change intmap size , no matter expand or shrink...
//if memory error , this function can back to previous state...
static int change_intmap_size(Intmap *imap , int new_size , int flags)
{
    //first allocate bitmap , if this failed , we can back to previout state...
    //if array allocate failed , we can back to previous state too...
    int bitmap_size = BITMAP_SIZE(new_size);
    int second_bitmap_size = SECOND_BITMAP_SIZE(new_size);
    int all_size = bitmap_size + second_bitmap_size;
    unsigned long *tmp_bm = (unsigned long *)malloc(sizeof(unsigned long) * all_size);
    if(NULL == tmp_bm)
    {
        LOG_ERROR("Allocate memory %d failed when reallocate bitmap ..." , sizeof(unsigned long) * all_size);
        return -1;
    }
    void **new_array = (void **)realloc(imap->array , sizeof(void *) * new_size);
    if(NULL == new_array)
    {
        LOG_ERROR("reallocate memory %d failed when expand intmap ..." , new_size);
        free(tmp_bm);
        tmp_bm = NULL;
        return -1;
    }
    imap->array = new_array;
    imap->size = new_size;
    //find the less size of previous size and after size of bitmap...
    int cp_size = (flags ? imap->second_bitmap_size : second_bitmap_size);
    memcpy(tmp_bm , imap->second_bitmap , cp_size * sizeof(unsigned long));
    cp_size = (flags ? imap->bitmap_size : bitmap_size);
    memcpy(tmp_bm + second_bitmap_size , imap->bitmap , cp_size * sizeof(unsigned long));
    if(flags)
    {
        int i = 0;
        for(i = imap->second_bitmap_size ; i < second_bitmap_size ; ++ i)
            tmp_bm[i] = 0x0UL;
        for(i = imap->bitmap_size ; i < bitmap_size ; ++ i)
            tmp_bm[second_bitmap_size + i] = 0x0UL;
    }

    imap->bitmap_size = bitmap_size;
    imap->second_bitmap_size = second_bitmap_size;
    free(imap->second_bitmap);
    imap->second_bitmap = tmp_bm;
    imap->bitmap = tmp_bm + second_bitmap_size;

    return 0;
}

//append this array when it is full...
static int expand_intmap(Intmap *imap)
{
    int cur_size = imap->size;

    //this means just after init intmap...
    if(0 == cur_size)
    {
        if(allocate_intmap(imap , INIT_SIZE) < 0)
        {
            LOG_ERROR("Allocate intmap for size %d failed when expand intmap ..." , INIT_SIZE);
            return -1;
        }
        return 0;
    }
    
    int expand_size = 0;
    //if current size is more than max expand size , should increate with EXPAND_STEP
    if(cur_size >= MAX_EXPAND_SIZE)
        expand_size = cur_size + EXPAND_STEP;
    else 
        expand_size = 2 * cur_size;
    
    if(change_intmap_size(imap , expand_size , 1) < 0)
    {
        LOG_ERROR("Change intmap size from %d to %d failed ..." , cur_size , expand_size);
        return -1;
    }

    return 0;
}

#define IS_FULL(bm)   (!((bm) ^ MAX_LONG))
#define IS_EMPTY(bm)  (!((bm) | 0x0))

#define SET_BIT(value , index) \
    (*value) |= (0x1UL << index)

#define CLEAR_BIT(value , index) \
    (*value) &= ~(0x1UL << index)

//#define CHECK_SET(value , index)      (*value) & (0x1UL << index)
unsigned long CHECK_SET(unsigned long *value , int index)
{
    return (*value) & (0x1UL << index);
}


static void insert_value_to_bitmap(Intmap *imap , int index , void *value)
{
    (imap->array)[index] = value;
    int bitmap_index = index / LONG_SIZE;
    int in_index = index % LONG_SIZE;
    
    SET_BIT((imap->bitmap + bitmap_index) , in_index);
    if(IS_FULL((imap->bitmap)[bitmap_index]))
    {
        int second_bitmap_index = bitmap_index / LONG_SIZE;
        in_index = bitmap_index % LONG_SIZE;
        SET_BIT((imap->second_bitmap + second_bitmap_index) , in_index);
    }

    ++ imap->length;
}

static void clear_value_from_intmap(Intmap *imap , int index)
{
    (imap->array)[index] = NULL;
    int bitmap_index = index / LONG_SIZE;
    int in_index = index % LONG_SIZE;
    
    //keep this bitmap item value first , check it after clear...
    unsigned long cur_bm = *(imap->bitmap + bitmap_index);
    CLEAR_BIT((imap->bitmap + bitmap_index) , in_index);
    if(IS_FULL(cur_bm))
    {
        int second_bitmap_index = bitmap_index / LONG_SIZE;
        in_index = bitmap_index % LONG_SIZE;
        CLEAR_BIT((imap->second_bitmap + second_bitmap_index) , in_index);
    }
    -- imap->length;
}

static int find_lowest_index(unsigned long bm)
{
    int index = 0;
    //this means the unsigned long is full of 1...
    if(IS_FULL(bm))
        LOG_FATAL("this single unsigned long can not be full unsigned long...");

     //get the lowerest 0 in this unsigned long...
    while((bm & 0x1UL) != 0)
    {
        bm >>= 1;
        ++ index;
    }

    return index;
}

//get the first unused index in the array use bitmap...
static int get_unused_index(Intmap *imap)
{
    //length equal to size means this array is full....
    if(imap->length == imap->size)
        return -1;

    int index = 0;
    int second_index = 0;
    int in_index = 0;
    
    unsigned long *bm_cur = imap->second_bitmap;
    unsigned long *bm_start = imap->second_bitmap;
    unsigned long *bm_end = imap->second_bitmap + imap->second_bitmap_size;
    //only need check the second bitmap , a bit in second bitmap means 
    //a unsigned long in bitmap , so you can get the unused index only use second bitmap...
    for( ; bm_cur != bm_end ; ++ bm_cur)
    {
        //this means all bit in this unsigned long has set to 1...
        if(IS_FULL(*bm_cur))
        {
            continue ;
        }
        else 
            break;
    }
    //get first unused bit in second bitmap , this means index  of second_index in 
    //bitmap must be not full...
    in_index = find_lowest_index(*bm_cur);
    second_index = in_index + (bm_cur - bm_start) * LONG_SIZE;

    //so you can get the inside first unused bit directly...
    in_index = find_lowest_index((imap->bitmap)[second_index]);
    index = in_index + second_index * LONG_SIZE;

    return index;
}

int intmap_put_value(Intmap *imap , void *value)
{
    if(NULL == imap)
    {
        LOG_WARNING("Can not put value to a NULL Intmap...");
        return -1;
    }

    //then check second bitmap and bitmap...
    int last_index = get_unused_index(imap);
    //this means we should increase the length of array...
    int last_size = imap->size;
    if(last_index < 0)
    {
        int ret = expand_intmap(imap);
        if(ret < 0)
        {
            LOG_ERROR("Expand intmap failed...");
            return -1;
        }
        //find the first index of new allocated and use this index...
        last_index = last_size;
        insert_value_to_bitmap(imap , last_index , value);
        return last_index;
    }
    else 
    {
        insert_value_to_bitmap(imap , last_index , value);
        return last_index;
    }
}

void* intmap_get_value(Intmap *imap , int index)
{
    if(NULL == imap)
    {
        LOG_WARNING("Can not get a value from a NULL Intmap...");
        return NULL;
    }

    if(imap->size <= index)
    {
        LOG_WARNING("Get value : current size %d and request index %d..." , imap->size , index);
        return NULL;
    }
    
    int bitmap_index = index / LONG_SIZE;
    int in_index = index % LONG_SIZE;
    //this is a bug , if use CHECK_SET implemented with #define will cause unknown error...
//    if(!CHECK_SET(imap->bitmap + bitmap_index , in_index))
    if(!((imap->bitmap)[bitmap_index] & (0x1UL << in_index)))
        return NULL;

    return imap->array[index];
}

static int shrink_intmap_inside(Intmap *imap , int shrink_size)
{
    int cur_size = imap->size;
    int real_shrink_size = shrink_size * LONG_SIZE;
    if(cur_size - real_shrink_size < INIT_SIZE)
        real_shrink_size = INIT_SIZE;
    else 
        real_shrink_size = cur_size - real_shrink_size;

    if(change_intmap_size(imap , real_shrink_size , 0) < 0)
    {
        LOG_WARNING("Change intmap size from %d to %d failed ..." , cur_size , real_shrink_size);
        return -1;
    }
    return 0;
}

static int check_empty_slots(Intmap *imap)
{
    int counter = 0;
    unsigned long *bm_head = imap->second_bitmap + imap->second_bitmap_size;
    unsigned long *bm_tail = bm_head + imap->bitmap_size - 1;

    while(bm_tail >= bm_head)
    {
        if(!IS_EMPTY(*bm_tail))
            break;
            
        ++ counter;
        bm_tail --;
    }

    return counter;
}

//you can only shrink memory from tail....
static void shrink_intmap(Intmap *imap)
{
    if(NULL == imap)
    {
        LOG_WARNING("Can not shrink a NULL Intmap ...");
        return ;
    }

    //check empty slots in array , only backword...
    int empty_size = check_empty_slots(imap);

    if(empty_size == 0)
    {
        return ;
    }

    if(shrink_intmap_inside(imap , empty_size) < 0)
        LOG_WARNING("shrink intmap %d size failed ..." , empty_size * LONG_SIZE);
}

int intmap_clear_value(Intmap *imap , int index)
{
    if(NULL == imap)
    {
        LOG_WARNING("Can not clear value in a NULL Intmap...");
        return -1;
    }
    if(imap->size <= index)
    {
        LOG_WARNING("Clear value : current size %d and request index %d..." , imap->size , index);
        return -1;
    }
    
    int bitmap_index = index / LONG_SIZE;
    int in_index = index % LONG_SIZE;
    //this is the same with upper...
//    if(!CHECK_SET(imap->bitmap + bitmap_index , in_index))
    if(!((imap->bitmap)[bitmap_index] & (0x1UL << in_index)))
        return -1;

    clear_value_from_intmap(imap , index);

    if(imap->size > SHRINK_SIZE && ((imap->size >> 2) * 3) > imap->length)
        shrink_intmap(imap);

    return 0;
}


int intmap_length(Intmap *imap)
{
    if(NULL == imap) 
        return -1;
    else 
        return imap->length;
}

int intmap_size(Intmap *imap)
{
    if(NULL == imap)
        return -1;
    else 
        return imap->size;
}

int intmap_used_memory(Intmap *imap)
{
    if(NULL == imap)
        return 0;

    int self_size = sizeof(*imap);
    int array_size = sizeof(void *) * imap->size;
    int bitmap_size = (imap->bitmap_size + imap->second_bitmap_size) * sizeof(unsigned long);

    return self_size + array_size + bitmap_size;
}

void destory_intmap(Intmap *imap)
{
    if(NULL == imap)
        return ;

    if(imap->length != 0)
        LOG_WARNING("There are %d elements left when destory Intmap !!!" , imap->length);
    if(NULL == imap->array)
    {
        free(imap);
        return ;
    }

    free(imap->array);
    imap->array = NULL;
    free(imap->second_bitmap);
    imap->second_bitmap = NULL;
    free(imap);
    imap = NULL;
}











#ifdef TEST_INTMAP

//check current intmap state , return -1 means error...return 0 means no error...
static int check_current_intmap(Intmap *imap)
{
    int real_bitmap_bits = imap->size;
    int bitmap_size = ((real_bitmap_bits % LONG_SIZE) ? (real_bitmap_bits / LONG_SIZE + 1) : (real_bitmap_bits / LONG_SIZE));
    if(bitmap_size != imap->bitmap_size)
    {
        LOG_INFO("Current bitmap size in bitmap : %d and should be %d ..." , imap->bitmap_size , bitmap_size);
        return -1;
    }
    int second_real_bitmap_bits = bitmap_size;
    int second_bitmap_size = ((second_real_bitmap_bits % LONG_SIZE) ? (second_real_bitmap_bits / LONG_SIZE + 1) : 
            (second_real_bitmap_bits / LONG_SIZE));
    if(second_bitmap_size != imap->second_bitmap_size)
    {
        LOG_INFO("Current second bitmap size in bitmap : %d and shoule be %d ..." , imap->second_bitmap_size , second_bitmap_size);
        return -1;
    }

    unsigned long *second_bitmap_ptr = imap->second_bitmap;
    int i = 0;
    int current_index = 0;
    for(i = 0 ; i < second_real_bitmap_bits; ++ i)
    {
        if(!(i % LONG_SIZE))
            second_bitmap_ptr = imap->second_bitmap + (i / LONG_SIZE);

        int index = i % LONG_SIZE;
        int bit = (*second_bitmap_ptr >> index) & 0x1UL;
        //in second bitmap , a bit is 1 means a MAX_LONG in bitmap...
        if(1 == bit && imap->bitmap[i] != MAX_LONG)
        {
            LOG_INFO("NO. %d is 1 in second bitmap while bitmap is %lu ..." , i , imap->bitmap[i]);
            return -1;
        }
        if(0 == bit && imap->bitmap[i] == MAX_LONG)
        {
            LOG_INFO("NO. %d is 0 in second bitmap while bitmap is %lu ..." , i , imap->bitmap[i]);
            return -1;
        }

        unsigned long *cur_bitmap_ptr = imap->bitmap + i;
        int k = 0;
        for(k = 0 ; k < LONG_SIZE && current_index < imap->size ; ++ k)
        {
            int bmbit = (*cur_bitmap_ptr >> k) * 0x1UL;
            if(1 == bmbit && imap->array[current_index] == NULL)
            {
                LOG_INFO("Index %d bitmap is 1 while value is NULL ..." , current_index);
                return -1;
            }

            if(0 == bmbit && imap->array[current_index] != NULL)
            {
                LOG_INFO("Index %d bitmap is 0 while value is not NULL ..." , current_index);
                return -1;
            }
            current_index ++;
        }
    }

    LOG_INFO("All bitmap and value is correct !!!");
    return 0;
}

#include <time.h>
#define TEST_NUM    10000
int main(int argc , char *argv[])
{
    START_DEBUG(argv[0] , 0 , 0);
    int all_index[TEST_NUM];

    int init_size = 0;
    Intmap *imap = create_intmap(init_size);
    if(NULL == imap)
    {
        LOG_ERROR("Create intmap of size %d failed !" , init_size);
        return -1;
    }

    int i = 0;
    for(i = 0 ; i < TEST_NUM ; ++ i)
    {
        int index = intmap_put_value(imap , (void *)(i + 1));
        if(index < 0)
        {
            LOG_ERROR("Put value %d to intmap failed !" , i + 1);
            return -1;
        }

//        LOG_INFO("Put a value %d to intmap and index %d ..." , i + 1 , index);
        all_index[i] = index;
        if(i > 0 && !(i % 1000))
            check_current_intmap(imap);
    }

    for(i = 0 ; i < TEST_NUM ; ++ i)
    {
        int value = (int)intmap_get_value(imap , all_index[i]);
        if(value != i + 1)
            LOG_WARNING("index %d this is not equal %d , ERROR ...." , all_index[i] , value);
    }

    for(i = 0 ; i < TEST_NUM ; ++ i)
    {
        intmap_clear_value(imap , all_index[i]);
    }

    LOG_INFO("Current intmap size : %d ..." , intmap_size(imap));
    LOG_INFO("Current intmap length : %d ..." , intmap_length(imap));
    LOG_INFO("Current memory allocate : %d\n" , intmap_used_memory(imap));

    //next start random test...
    //put TEST_TIMES and then clear TEST_TIMES / 2 times...and then reput TEST_TIMES times...
#define TEST_TIMES 10000
#define MAX_NUMBER 100000
    srand(time(NULL));

    LOG_INFO("Start random test : \n");
    int random_index[2 * TEST_TIMES] = {0};
    int random_values[2 * TEST_TIMES] = {0};
    for(i = 0 ; i < 2 * TEST_TIMES ; ++ i)
    {
        random_index[i] = -1;
        random_values[i] = -1;
    }
    for (i = 0 ; i < TEST_TIMES ; ++ i)
    {
        int value = rand() % MAX_NUMBER;
        random_index[i] = intmap_put_value(imap , (void *)value);
        if(random_index[i] < 0)
        {
            LOG_ERROR("intmap put value error ...");
            return -1;
        }
        random_values[random_index[i]] = value;
    }

    for(i = 0 ; i < (TEST_TIMES >> 1) ; ++ i)
    {
        int index = rand() % TEST_TIMES;
        if(intmap_clear_value(imap , random_index[index]) < 0)
        {
            -- i;
            continue ;
        }
        random_values[random_index[index]] = -1;
    }
    check_current_intmap(imap);
    LOG_INFO("Current intmap size : %d ..." , intmap_size(imap));
    LOG_INFO("Current intmap length : %d ..." , intmap_length(imap));
    LOG_INFO("Current memory allocate : %d\n" , intmap_used_memory(imap));

    for(i = TEST_TIMES ; i < (TEST_TIMES << 1) ; ++ i)
    {
        int value = rand() % MAX_NUMBER;
        random_index[i] = intmap_put_value(imap , (void *)value);
        if(random_index[i] < 0)
        {
            LOG_ERROR("intmap put value failed ...");
            return -1;
        }
//        LOG_INFO("Insert a value : %d and index %d length %d and size %d .." , value , random_index[i] , intmap_length(imap) , intmap_size(imap));
        random_values[random_index[i]] = value;
    }

    check_current_intmap(imap);
    LOG_INFO("Current intmap size : %d ..." , intmap_size(imap));
    LOG_INFO("Current intmap length : %d ..." , intmap_length(imap));
    LOG_INFO("Current memory allocate : %d\n" , intmap_used_memory(imap));

    for(i = 0 ; i < (TEST_TIMES << 1) ; ++ i)
    {
        int rand_index = random_index[i];
        int value = (int)intmap_get_value(imap , random_index[i]);
        if(value == (int)NULL && random_values[rand_index] != -1)
        {
            LOG_ERROR("get value failed : get value %d and value %d of index %d ..." , value , random_values[i] , random_index[i]);
            return -1;
        }
        if(value != (int)NULL && random_values[rand_index] != value)
        {
            LOG_ERROR("get value error : get value %d and should be %d of index %d ..." , value , random_values[i] , random_index[i]);
            return -1;
        }
    }

    
    destory_intmap(imap);
    FINISH_DEBUG();

    return 0;
}

#endif


#ifdef PRO_TEST

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
}

#endif
