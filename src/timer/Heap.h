/*
 * =====================================================================================
 *
 *       Filename:  Heap.h
 *
 *    Description:  a heap for timer insert / delete and select the smallest item
 *
 *        Version:  1.0
 *        Created:  04/19/13 16:13:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Heaven (), zhanwenhan@163.com
 *        Company:  NDSL
 *
 * =====================================================================================
 */

#ifndef _H_HEAP_INSIDE_H_
#define _H_HEAP_INSIDE_H_

typedef unsigned long long int UINT64;

typedef struct item
{
    UINT64  weight;
    void *  value;
}Item;

#define _MAX_ROOT  0
#define _MIN_ROOT  1

typedef struct heap
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
}Heap;

Heap *create_heap(int init_sz , int type);

int get_heap_type();

int insert_to_heap(Heap *hp , UINT64 weight , void *value);

void free_from_heap(Heap *hp , UINT64 weight , void *value);

void modify_on_heap(Heap *hp , UINT64 weight , void *value , UINT64 new_weight);

void *get_root_value(Heap *hp);

void *get_and_remove_root(Heap *hp);

void destory_heap(Heap *hp);

#endif
