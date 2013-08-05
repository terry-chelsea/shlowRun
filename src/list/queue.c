/*
 * =====================================================================================
 *
 *       Filename:  queue.c
 *
 *    Description:  this is a normal list only offer put and get operations 
 *    with mutil threads...
 *
 *        Version:  1.0
 *        Created:  07/09/13 17:04:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <string.h>
#include "queue.h"

struct node
{
    ElemType value;
    NODE *next;
};

//This is a FIFO queue implemented with list ...
//you can create list with lock or not using lock...
struct list
{
    NODE *head;         //list head...
    NODE *tail;         //list tail...
    int  elems;
    unsigned char need_lock;     //flag show need lock when read&write to list...
    unsigned char waiting;    //if using lock , flag show if some thread waiting for lock unlock...
    //this is used when create list with lock...parameter not 0...
    pthread_mutex_t lmutex;
    pthread_cond_t  lcond;
};

#define SUCCESS  0
#define FAILED  -1

QUEUE *queue_create(int lock)
{
    QUEUE *lst = (QUEUE *)malloc(sizeof(QUEUE));
    if(NULL == lst)
        return NULL;

    memset(lst , 0 , sizeof(*lst));

    lst->head = lst->tail = NULL;
    lst->elems = 0;
    lst->waiting = 0;
    if(!lock)
    {
        lst->need_lock = 0;
        return lst;
    }

    if((pthread_mutex_init(&(lst->lmutex) , NULL) < 0) || 
            (pthread_cond_init(&(lst->lcond) , NULL) < 0))
    {
        perror("init mutex or condtion failed : ");
        return NULL;
    }
    lst->need_lock = 1;

    return lst;
}

static void add_to_list_tail(QUEUE *lst , NODE *node)
{
    if(NULL == lst->head)
    {
        lst->head = node;
        lst->tail = node;
    }
    else 
    {
        lst->tail->next = node;
        lst->tail = node;
    }

    lst->elems ++;
}

static NODE *del_from_list_head(QUEUE *lst)
{
    NODE *node = NULL;
    if(NULL == lst->head)
        return NULL;

    if(lst->head == lst->tail)
    {
        node = lst->head;
        lst->head = NULL;
        lst->tail = NULL;
    }
    else 
    {
        node = lst->head;
        lst->head = node->next;
    }
    lst->elems --;

    return node;
}

int queue_put(QUEUE *queue , ElemType value)
{
    if(queue == NULL)
    {
        fprintf(stderr , "WARNING : Put a value to NULL queue ...\n");
        return FAILED;
    }

    NODE *node = (NODE *)malloc(sizeof(NODE));
    if(NULL == node)
    {
        fprintf(stderr , "Allocate memory size %lu failed when put to queue...\n" , sizeof(NODE));
        return FAILED;
    }

    node->value = value;
    node->next = NULL;

    if(queue->need_lock)
    {
        pthread_mutex_lock(&(queue->lmutex));
        add_to_list_tail(queue , node);
        int flag = 0;
        if(queue->waiting > 0)
        {
            flag = 1;
            queue->waiting --;
        }
        pthread_mutex_unlock(&(queue->lmutex));

        //use flag to unlock first and then do signal ....
        if(flag)
            pthread_cond_signal(&(queue->lcond));
    }
    else 
    {
        add_to_list_tail(queue , node);
    }

    return SUCCESS;
}

static NODE *get_remove_node_lock(QUEUE *lst , int flag)
{
    pthread_mutex_lock(&(lst->lmutex));
    NODE *node = del_from_list_head(lst);
    while(NULL == node)
    {
        if(!flag)
        {
            pthread_mutex_unlock(&(lst->lmutex));
            return NULL;
        }

        lst->waiting ++ ;
        pthread_cond_wait(&(lst->lcond) , &(lst->lmutex));
        node = del_from_list_head(lst);
    }

    pthread_mutex_unlock(&(lst->lmutex));
    
    return node;
}

static int queue_get_inside(QUEUE *queue , ElemType *val , int flag)
{
    NODE *node = NULL;
    if(queue->need_lock)
    {
        //the last parameter means blocking on waiting for condition
        //if there are no elements to get...
        node = get_remove_node_lock(queue , flag);
        if(node == NULL)
        {
            fprintf(stderr , "ERROR:CAN not happen when remove a node return NULL with blocking...\n");
            return FAILED;
        }
    }
    else 
    {
        node = del_from_list_head(queue);
        if(NULL == node)
            return FAILED;
    }
    *val = node->value;
    free(node);

    return SUCCESS;
}

int queue_get(QUEUE *queue , ElemType *val)
{
    if(queue == NULL)
    {
        fprintf(stderr , "WARNING : get value from a NULL queue ...\n");
        return FAILED;
    }
    
    return queue_get_inside(queue , val , 1);
}


int queue_get_nonblocking(QUEUE *queue , ElemType *val)
{
    if(queue == NULL)
    {
        fprintf(stderr , "WARNING : nonblocking get value from a NULL queue ...\n");
        return FAILED;
    }
    
    return queue_get_inside(queue , val , 0);
}

static int list_size_inside(QUEUE *lst)
{
    int counter = 0;
    NODE *tmp = lst->head;
    while(tmp != NULL)
    {
        tmp = tmp->next;

        ++counter;
    }

    return counter;
}

int queue_size(QUEUE *queue)
{
    if(NULL == queue)
    {
        fprintf(stderr , "WARNING : get size from a NULL queue ...\n");
        return -1;
    }

    int size = 0;
    if(queue->need_lock)
    {
        pthread_mutex_lock(&(queue->lmutex));
        size = list_size_inside(queue);
        pthread_mutex_unlock(&(queue->lmutex));
    }
    else 
        size = list_size_inside(queue);

    return size;
}

void queue_info(QUEUE *queue)
{
    if(queue == NULL)
    {
        fprintf(stderr , "WARNING : get queue info with a NULL queue ...\n");
        return ;
    }
    if(queue->need_lock)
        pthread_mutex_lock(&(queue->lmutex));

    printf("Statics : \n");
    printf("sum elements counter : %d\n" , queue->elems);
    printf("With lock : %s and current waiting threads : %d\n" , 
            queue->need_lock ? "Yes" : "No" , queue->waiting);

    if(queue->need_lock)
        pthread_mutex_unlock(&(queue->lmutex));

    printf("Elements in the list : %d\n" , queue_size(queue));
}

void queue_destory(QUEUE **queue)
{
    if(NULL == queue)
        return ;

    if((*queue)->elems != 0)
    {
        printf("----------destory list : -----------\n");
        queue_info(*queue);
    }

    NODE *tmp = (*queue)->head;
    while(tmp != NULL)
    {
        NODE  *next = tmp->next;
        free(tmp);
        tmp = next;
    }

    free(*queue);
    *queue = NULL;
}

NODE *queue_get_tail(QUEUE *lst)
{
    if(NULL == lst)
    {
        fprintf(stderr , "get tail from NULL queue ...\n");
        return NULL;
    }

    NODE *node = NULL;
    if(lst->need_lock)
    {
        pthread_mutex_lock(&(lst->lmutex));
        node =  lst->tail;
        pthread_mutex_unlock(&(lst->lmutex));
    }
    else 
        node = lst->tail;

    return node;
}
