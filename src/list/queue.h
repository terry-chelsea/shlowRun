/*
 * =====================================================================================
 *
 *       Filename:  queue.h
 *
 *    Description:  interface of a queue implemented with list...
 *
 *        Version:  1.0
 *        Created:  07/10/13 22:58:33
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#ifndef _H_NORMAL_QUEUE_H_
#define _H_NORMAL_QUEUE_H_

#include <pthread.h>
#include <stdio.h>

typedef void * ElemType;
typedef struct node NODE;

//use list to implement a queue , every put is insert to the tail and get from the head ...
//no max elements limits , but when you do get operation and no elements in the queue...
//maybe return failed or waiting until a new value is inserted ...
typedef struct list QUEUE;

//create a queue , parameter is if create it with lock ...
//lock = 0 means do not use lock for this queue , otherwise , with a single lock...
//return NULL means create failed ... return not NULL means a headle of queue...
QUEUE * queue_create(int lock);

//insert a value to the tail of queue...
//rerurn -1 means error because queue is NULL or allocate memory failed ...
//return 0 means success ...
int queue_put(QUEUE * queue , ElemType value);

//get the value from the head of a queue...
//rerurn -1 means failed ...maybe because no elements in queue now...
//return 0 means success...
int queue_get(QUEUE *queue , ElemType *value);

//because getting a value when no elements in queue will block this thread , use this interface 
//means you do not want to waiting for a new value to be putted...
int queue_get_nonblocking(QUEUE *queue , ElemType *value);

//get current size of this queue...
int queue_size(QUEUE *queue);

//just for debug , display infomations about this queue...
void queue_info(QUEUE *queue);

//destory a queue , you should send the second pointer as the parameter...
//no return value ...
void queue_destory(QUEUE **lst);

NODE *queue_get_tail(QUEUE *lst);

#endif

