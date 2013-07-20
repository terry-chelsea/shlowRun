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

#define _MAX_ROOT  0
#define _MIN_ROOT  1

typedef unsigned long long int UINT64;

typedef struct item Item;

typedef struct heap Heap;

Heap *create_heap(int init_sz , int type);

int get_heap_type();

int insert_to_heap(Heap *hp , UINT64 weight , void *value);

void free_from_heap(Heap *hp , UINT64 weight , void *value);

void modify_on_heap(Heap *hp , UINT64 weight , void *value , UINT64 new_weight , void *new_value);

void modify_heap_root(Heap *hp , UINT64 new_weight , void *value);

void *get_root_value(Heap *hp);

void *get_and_remove_root(Heap *hp);

void free_heap_root(Heap *hp);

void do_heap_sort(Heap *hp);

void display_heap(Heap *hp);

void destory_heap(Heap *hp);

#endif
