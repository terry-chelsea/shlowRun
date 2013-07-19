/*
 * =====================================================================================
 *
 *       Filename:  heap_sort.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/19/13 10:44:00
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  huyao (H.Y), yaoshunyuhuyao@163.com
 *        Company:  NDSL
 *
 * =====================================================================================
 */

#include "Heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main()
{
    srand(time(NULL));
    Heap *hp = create_heap(0 , _MAX_ROOT);
    if(NULL == hp)
    {
        printf("Create heap failed ...\n");
        return -1;
    }

    for(int i = 0 ; i < 16 ; ++ i)
    {
        UINT64 index = rand() % 1000;
        if(insert_to_heap(hp , index , NULL) < 0)
        {
            printf("insert %llu failed ...\n" , index);
            return -1;
        }
    }

    display_heap(hp);
    do_heap_sort(hp);
    display_heap(hp);

    destory_heap(hp);
    return 0;
}
