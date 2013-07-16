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

#define LONG_SIZE     (sizeof(unsigned long) * 8)
#define ALIGN_SIZE    16
#define ALIGN_TO(sz)  ((sz) % ALIGN_SIZE ?  \
        ((sz) & ~(ALIGN_SIZE - 1)) + ALIGN_SIZE : \
        sz)

#define BITMAP_SIZE(sz)    ((sz) % LONG_SIZE ? \
        (sz) / LONG_SIZE + 1 : (sz) / LONG_SIZE)
#define SECOND_BITMAP_SIZE(sz)  (BITMAP_SIZE(BITMAP_SIZE(sz)))


Intmap *create_intmap(int init_sz)
{
    if(init_sz < 0)
        init_sz = 0;

    if(init_sz > 0)
        init_sz = ALIGN_TO(init_sz);

    LOG_INFO("create initmap with init size : %d" , init_sz);
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
        imap->last = -1;
        imap->start_index = 0;
        imap->head_size = imap->tail_size = 0;
        imap->bitmap = imap->second_bitmap = NULL;
        return imap;
    }

    void **all = (void **)malloc(sizeof(void *) * init_sz);
    if(NULL == all)
    {
        LOG_ERROR("Allocate memory %d for all array failed..." , sizeof(void *) * init_sz);
        free(imap);
        imap = NULL;
        return NULL;
    }
    int i = 0;
    for(i = 0 ; i < init_sz ; ++ i)
        all[i] = NULL;

    //bitmap and second bitmap is allocate together...
    int bitmap_size = BITMAP_SIZE(init_sz);
    int second_bitmap_size = SECOND_BITMAP_SIZE(init_sz);
    unsigned long *bmp = (unsigned long *)malloc(sizeof(unsigned long) * bitmap_size);
    if(NULL == bmp)
    {
        LOG_ERROR("Allocate memory %d for all bitmap failed..." , sizeof(unsigned long) * bitmap_size);
        free(all);
        free(imap);
        all = NULL ;
        imap = NULL;
        return NULL;
    }
    for(i = 0 ; i < bitmap_size ; ++ i)
        bmp[i] = 0x0;
    
    imap->array = all;
    imap->length = 0;
    imap->size = init_sz;
    imap->last = 0;
    imap->start_index = 0;
    imap->head_size = imap->tail_size = (init_sz >> 1);
    imap->second_bitmap = bmp;
    imap->bitmap = bmp + second_bitmap_size;

    return imap;
}


static int expand_intmap(Intmap *imap)
{
    return 0;
}

static void insert_value_to_bitmap(Intmap *imap , int index , void *value)
{
}

static void clear_value_from_intmap(Intmap *imap , int index)
{
}

static int get_unused_index(Intmap *imap)
{
    return 0;
}

int intmap_put_value(Intmap *imap , void *value)
{
    if(NULL == imap)
    {
        LOG_WARNING("Can not put value to a NULL Intmap...");
        return -1;
    }

    int last_index = imap->last;
    //check last free index firstly...
    if(last_index > 0)
    {
        insert_value_to_bitmap(imap , last_index , value);
        //clear last index...
        imap->last = -1;
        return last_index;
    }

    //then check second bitmap and bitmap...
    last_index = get_unused_index(imap);
    //this means we should increase the length of array...
    int last_size = imap->size;
    if(last_index < 0)
    {
        if(expand_intmap(imap) < 0)
        {
            LOG_ERROR("Expand intmap failed...");
            return -1;
        }
        //find the first index of new allocated and use this index...
        last_index = last_size + 1;
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

    if(imap->size >= index)
    {
        LOG_WARNING("The request index is bigger than max index...");
        return NULL;
    }
    return imap->array[index];
}

void intmap_clear_value(Intmap *imap , int index)
{
    if(NULL == imap)
    {
        LOG_WARNING("Can not clear value in a NULL Intmap...");
        return ;
    }
    if(imap->size >= index)
    {
        LOG_WARNING("The request index is bigger than max index...");
        return ;
    }

    clear_value_from_intmap(imap , index);
}

static void shrink_intmap_inside(Intmap *imap , int head_size , int tail_size)
{
}

static int check_empty_slots(Intmap *imap , int direction)
{
    return 0;
}

void shrink_intmap(Intmap *imap)
{
    if(NULL == imap)
    {
        LOG_WARNING("Can not shrink a NULL Intmap ...");
        return ;
    }

    //check empty slots in array , forward and backword...
    int head_empty_size = check_empty_slots(imap , 0);
    int tail_empty_size = check_empty_slots(imap , 1);

    if(tail_empty_size < ALIGN_SIZE && head_empty_size < ALIGN_SIZE)
    {
        LOG_INFO("There is no empty slots to shrink...");
        return ;
    }
    else if(tail_empty_size >= ALIGN_SIZE && head_empty_size >= ALIGN_SIZE)
    {
        head_empty_size = ALIGN_TO(head_empty_size);
        tail_empty_size = ALIGN_TO(tail_empty_size);
        shrink_intmap_inside(imap , head_empty_size , tail_empty_size);
    }
    else if(tail_empty_size > ALIGN_SIZE)
    {
        tail_empty_size = ALIGN_TO(tail_empty_size);
        shrink_intmap_inside(imap , -1 , tail_empty_size);
    }
    else 
    {
        head_empty_size = ALIGN_TO(head_empty_size);
        shrink_intmap_inside(imap , head_empty_size , -1);
    }
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
    int bitmap_size = (BITMAP_SIZE(imap->size) + SECOND_BITMAP_SIZE(imap->size))* sizeof(unsigned long);

    return self_size + array_size + bitmap_size;
}
