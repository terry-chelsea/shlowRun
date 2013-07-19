/*
 * =====================================================================================
 *
 *       Filename:  tmp.c
 *
 *    Description:  implement of common heap , bigger is the max value...
 *
 *        Version:  1.0
 *        Created:  07/18/13 17:42:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Heap.h"

struct item
{
    UINT64  weight;
    void *  value;
};

struct heap
{
    //store heap in a dynamic array ...
    Item *array;
    //array size...
    int  size;
    //current used size , should the first length items in array...
    int  length;
    int  free_counter;
    //max or min in the root , default is min is the root...
    int type;
};

#define LOG_ERROR(format , ...)  printf(format , ##__VA_ARGS__)
#define LOG_INFO(format , ...)  printf(format , ##__VA_ARGS__)

#define MAX_APPEND_SIZE  4096   //do increase by APPEND_STEP when size is bigger than it..
#define APPEND_STEP      256    //otherwise , increase by double...
#define MIN_SHRINK_SIZE  256    //do shrink when unused size is bigger than it...
#define INIT_SIZE        64
#define ALIGN_TO(sz)  ((sz) % INIT_SIZE ?  \
        ((sz) & ~(INIT_SIZE - 1)) + INIT_SIZE : sz)

Heap *create_heap(int init_sz , int type)
{
    Heap *hp = (Heap *)calloc(1 , sizeof(Heap));
    if(NULL == hp)
    {
        LOG_ERROR("Memory allocate %lu failed in creating heap ..." , sizeof(Heap));
        return  NULL;
    }

    if(!(type == _MAX_ROOT || type == _MIN_ROOT))
    {
        type = _MIN_ROOT;
    }

    if(init_sz <= 0)
        init_sz = INIT_SIZE;

    if(init_sz > 0)
        init_sz = ALIGN_TO(init_sz);

    hp->type = type;
    Item *items = (Item *)calloc(init_sz , sizeof(Item));
    if(NULL == items)
    {
        LOG_ERROR("Allocate memory for array size %lu failed ..." , init_sz * sizeof(Item));
        free(hp);
        return NULL;
    }
    hp->size = init_sz;
    hp->length = 0;
    hp->array = items;
    hp->free_counter = 0;

    return hp;
}

static int append_heap(Heap *hp)
{
    Item *start = hp->array;
    int cur_size = hp->size;
    int new_size = ((cur_size >= MAX_APPEND_SIZE) ? (cur_size + APPEND_STEP)
            : (cur_size << 1));

    Item *new_start = realloc(start , new_size * sizeof(Item));
    if(NULL ==  new_start)
    {
        LOG_ERROR("Allocate memory %lu failed when append heap ..." , new_size * sizeof(Item));
        return -1;
    }

    hp->array = new_start;
    hp->size = new_size;

    return 0;
}

static void shrink_heap(Heap *hp)
{
    Item *start = hp->array;
    int empty_size = hp->size - hp->length;
    int new_size = hp->length + (empty_size >> 1);

    Item *new_start = realloc(start , new_size * sizeof(Item));
    if(NULL ==  new_start)
    {
        LOG_ERROR("Allocate memory %lu failed when append heap ..." , new_size * sizeof(Item));
        return ;
    }

    hp->array = new_start;
    hp->size = new_size;
}

#define LEFT_CHILD(i)  (2 * i + 1)
#define RIGHT_CHILD(i) (2 * i + 2)
#define FATHER(i)      ((i - 1) >> 1)

#define SWAP_ITEM(first , second)   do{ \
    Item tmp = {(first)->weight , (first)->value}; \
    (first)->weight = (second)->weight; \
    (first)->value = (second)->value; \
    (second)->weight = tmp.weight; \
    (second)->value = tmp.value; \
}while(0)

#define NEED_SWAP_UP(cur , father , type) \
    ( (_MAX_ROOT == (type)) ? \
      ((cur)->weight > (father)->weight) : \
      ((cur)->weight < (father)->weight) )

#define NEED_SWAP_DOWN(cur , child , type) \
    ( (_MAX_ROOT == (type)) ? \
     ((cur)->weight < (child)->weight) : \
     ((cur)->weight > (child)->weight) )

#define ADJUST_UP   0
#define ADJUST_DOWN 1

static int next_index(Item *head , int max_index , int cur_index , int type)
{
    int index = 0;
    int left_child = LEFT_CHILD(cur_index);
    int right_child = RIGHT_CHILD(cur_index);

    if(right_child >= max_index)
    {
        if(left_child >= max_index)
            index = max_index;
        else 
            index = left_child;
    }
    else 
    {
        if(_MAX_ROOT == type)
        {
            if(head[left_child].weight > head[right_child].weight)
                index = left_child;
            else 
                index = right_child;
        }
        else 
        {
            if(head[left_child].weight < head[right_child].weight)
                index = left_child;
            else 
                index = right_child;
        }
    }

    return index;
}

//if direction is ADJUEST_UP , means adjust heap from back to front...
//otherwise adjust heap from the start to tail...
static void adjust_heap(Heap *hp , Item *cur , int direction)
{
    int type = hp->type;
    Item *head = hp->array;
    int max_index = hp->length;
    int cur_index = cur - head;
    int index = 0;

    //from tail to head...
    if(ADJUST_UP == direction)
    {
        index = FATHER(cur_index);
        while(index >= 0)
        {
            if(NEED_SWAP_UP(head + cur_index , head + index , type))
            {
                SWAP_ITEM(head + cur_index , head + index);
                cur_index = index;
                index = FATHER(cur_index);
            }
            else 
                break;
        }
    }
    else                //from start to tail...
    {
        index = next_index(head , max_index , cur_index , type);
        while(index < max_index)
        {
            if(NEED_SWAP_DOWN(head + cur_index , head + index , type))
            {
                SWAP_ITEM(head + cur_index , head + index);
                cur_index = index;
                index = next_index(head , max_index , cur_index , type);
            }
            else 
                break;
        }
    }
}

static Item *find_value(Heap *hp , UINT64 weight , void *value)
{
    Item *start = hp->array;
    Item *end = start + hp->length;

    for( ; start != end ; ++ start)
    {
        if((start->weight == weight) && (start->value == value))
            break;
    }

    if(start == end)
        return NULL;
    else 
        return start;
}

void adjust_heap_caused_by_modify(Heap *hp , Item *item , UINT64 weight , UINT64 new_weight)
{
    int direction = ADJUST_UP;
    if(_MAX_ROOT == hp->type)
    {
        if(new_weight > weight)
            direction = ADJUST_UP;
        else 
            direction = ADJUST_DOWN;
    }
    else 
    {
        if(new_weight < weight)
            direction = ADJUST_UP;
        else 
            direction = ADJUST_DOWN;
    }
    
    adjust_heap(hp , item , direction);
}

static void free_the_value(Heap *hp , Item *item)
{
    UINT64 old_weight = 0;
    int flag = 0; 
    //for free...
    if(item != hp->array)
    {
        flag = 1;
        old_weight = item->weight;
    }

    item->weight = 0;
    item->value = NULL;
    SWAP_ITEM(item , hp->array + hp->length - 1);
    -- hp->length;
    if(!flag)
        adjust_heap(hp , item , ADJUST_DOWN);
    else 
        adjust_heap_caused_by_modify(hp , item , old_weight , item->weight);

    hp->free_counter ++;
    if((hp->free_counter > INIT_SIZE) && 
            (hp->size - hp->length > MIN_SHRINK_SIZE) && 
            (hp->size >= INIT_SIZE))
    {
        shrink_heap(hp);
        hp->free_counter = 0;
    }
}

int insert_to_heap(Heap *hp , UINT64 weight , void *value)
{
    if(hp == NULL)
    {
        LOG_ERROR("Cannot insert to a NULL heap...");
        return -1;
    }
    //if array is full , append it...
    if(hp->size == hp->length)
    {
        if(append_heap(hp) < 0)
        {
            LOG_ERROR("Append array in heap failed  , current size %d..." , hp->size);
            return -1;
        }
    }

    Item *item = hp->array + hp->length;
    item->weight = weight;
    item->value = value;
    ++ hp->length; 

    adjust_heap(hp , hp->array + hp->length - 1 , ADJUST_UP);
    return 0;
}

void free_from_heap(Heap *hp , UINT64 weight , void *value)
{
    if(NULL == hp)
    {
        LOG_ERROR("Cannot free from a NULL heap ...");
        return ;
    }

    Item *item = find_value(hp , weight , value);
    if(NULL == item)
        return ;

    free_the_value(hp , item);
}

void modify_on_heap(Heap *hp , UINT64 weight , void *value , UINT64 new_weight)
{
    if(NULL == hp)
    {
        LOG_ERROR("Cannot modify on NULL heap ...");
        return ;
    }

    if(new_weight == weight)
        return ;

    Item *item = find_value(hp , weight , value);
    if(NULL == item)
        return ;
    
    adjust_heap_caused_by_modify(hp , item , weight , new_weight);
}

void *get_root_value(Heap *hp)
{
    if(NULL == hp)
    {
        LOG_ERROR("Cannot get root value from a NULL heap ...");
        return NULL;
    }
    if(0 == hp->length)
        return NULL;

    return (hp->array[0]).value;
}

void *get_and_remove_root(Heap *hp)
{
    if(NULL == hp)
    {
        LOG_ERROR("Cannot get root value from a NULL heap ...");
        return NULL;
    }

    if(0 == hp->length)
        return NULL;

    void *value = (hp->array[0]).value;
    free_the_value(hp , hp->array);

    return value;
}

void destory_heap(Heap *hp)
{
    if(NULL == hp)
        return ;

    if(hp->length != 0)
        LOG_INFO("There are %d items when destory heap ...\n" , hp->length);

    if(hp->array != NULL)
    {
        free(hp->array);
        hp->array = NULL;
    }

    free(hp);
    hp = NULL;
}

int get_heap_type(Heap *hp)
{
    if(hp != NULL)
        return hp->type;

    return -1;
}

void display_heap(Heap *hp)
{
    LOG_INFO("Heap %p is a %s heap : \n" , hp , 
            ((_MAX_ROOT == hp->type) ? "MAX" : "MIN"));
    LOG_INFO("Heap current size : %d\n" , hp->size);
    LOG_INFO("Heap current length : %d\n" , hp->length);
    LOG_INFO("All elements weight displays : \n");
    int i = 0;
    for(i = 0 ; i < hp->length ; ++ i)
        printf("%llu  " , (hp->array[i]).weight);

    printf("\n");
}

void do_heap_sort(Heap *hp)
{
    Item *head = hp->array;
    int cur_length = hp->length;
    int i = 0;
    for(i = 0 ; i < cur_length ; ++ i)
    {
        SWAP_ITEM(head , head + hp->length - 1);
        -- hp->length;
        adjust_heap(hp , head , ADJUST_DOWN);
    }

    hp->length = cur_length;
}

#ifdef TEST_HEAP

static int check_state(Heap *hp)
{
    Item *head = hp->array;

    int i = 0;
    for(i = 0 ; i < hp->length ; ++ i)
    {
        UINT64 weight = head[i].weight;
        int left_child = LEFT_CHILD(i);
        int right_child = RIGHT_CHILD(i);

        if(right_child < hp->length)
        {
            if(_MAX_ROOT == hp->type)
            {
                if(head[right_child].weight > weight || 
                        head[left_child].weight > weight)
                    return i;
            }
            else 
            {
                if(head[right_child].weight < weight || 
                        head[left_child].weight < weight)
                    return i;
            }
        }
        else if(left_child < hp->length)
        {
            if(_MAX_ROOT == hp->type)
            {
                if(head[left_child].weight > weight)
                    return i;
            }
            else 
            {
                if(head[left_child].weight < weight)
                    return i;
            }
        }
    }

    if(hp->size > MAX_APPEND_SIZE && hp->size - hp->length > MIN_SHRINK_SIZE)
        printf("Current size : %d and current length : %d\n" , hp->size , hp->length);
    return -1;
}

#define MAX_VALUE  1000
#define TEST_TIME  10240

UINT64  all_index[TEST_TIME];
int main()
{
    Heap *hp = create_heap(0 , _MAX_ROOT);
    if(NULL == hp)
    {
        printf("Create heap failed ...\n");
        return -1;
    }

    int i = 0;
    for(i = 0 ; i < TEST_TIME ; ++ i)
    {
        UINT64 index = rand() % MAX_VALUE;
        if(insert_to_heap(hp , index , NULL) < 0)
        {
            printf("Insert a value failed : weight : %llu and i is %d\n" , 
                    index , i);
            return -1;
        }
//        printf("Insert a weight %llu\n" , index);
//        display_heap(hp);
        int ret = check_state(hp);
        if(ret != -1)
        {
            printf("index %d  is error...\n" , ret);
            display_heap(hp);
        }
        all_index[i] = index;

#ifdef HEAP_SORT
        if(i == 20)
        {
            display_heap(hp);
            do_heap_sort(hp);
            display_heap(hp);
            return 0;
        }
#endif
    }
    printf("finish insert ...\n");

    for(i = 0 ; i < (TEST_TIME >> 1) ; ++ i)
    {
        UINT64 new_index = rand() % MAX_VALUE;
        UINT64 index = all_index[rand() % TEST_TIME];
        modify_on_heap(hp , index , NULL , new_index);

        int ret = check_state(hp);
        if(ret != -1)
        {
            printf("index %d  is error...\n" , ret);
            display_heap(hp);
        }
    }

    printf("finish modify ...\n");

    for(i = 0 ; i < (TEST_TIME >> 1) ; ++ i)
    {
        UINT64 index = all_index[rand() % TEST_TIME];
        free_from_heap(hp , index , NULL);

//        printf("Free a weight %llu\n" , index);
//        display_heap(hp);
        int ret = check_state(hp);
        if(ret != -1)
        {
            printf("index %d  is error...\n" , ret);
            display_heap(hp);
        }
    }
    printf("finish free ...\n");

    for(i = 0 ; hp->length != 0; ++ i)
    {
        void *value = get_and_remove_root(hp);
        if(NULL != value)
        {
            printf("Value is not NULL , error...\n");
            return -1;
        }
//        display_heap(hp);
        int ret = check_state(hp);
        if(ret != -1)
        {
            printf("index %d  is error...\n" , ret);
            display_heap(hp);
        }
    }

    printf("finish get root ...\n");

    return 0;
}

#endif
