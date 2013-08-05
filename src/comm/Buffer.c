/*
 * =====================================================================================
 *
 *       Filename:  Buffer.c
 *
 *    Description:  connection buffer implement...
 *
 *        Version:  1.0
 *        Created:  07/22/13 02:20:16
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include "Buffer.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "Debug.h"

//在我的思考中，因为读和写的操作场景完全不一样，所以读缓冲区和写缓冲区必须分开定义。
//对于读缓冲区，之所以使用循环缓冲区，一方面是我的偏好，另外可以节省空间，如果使用一个连续的缓冲区而不是循环缓冲区
//那么每一次上层从读缓冲区中读取到数据之后都需要mmove操作，而对于循环缓冲区来说，只有在扩展内存或者收缩内存的时候
//才需要执行mmove操作，显然后者的频率远小于前者。
//另外，对于读缓冲区设置一个栈上的内存空间，read操作是通过readv函数完成的，它会将数据优先读到栈的空间中，是有如下考虑的：
//如果栈的空间能够完全存放当前read的全部数据，并且能够作为一个请求被处理，那么就可以省去将这段内存拷贝到读缓冲区的buffer的操作了
//而这种场景应该会很频繁的出现，所以可以减少大量的内存拷贝，甚至不必要的内存分配。
//
//BUG:考虑到用户使用的时候需要的是连续的一段内存保持了一个请求，但是在我当前设计的这种读缓冲区却是不能够满足这样的需求的。
//因为有可能出现这段内存(一个请求)出现在三段内存中(开始从readindex开始，接着是从0开始到writeindex，最后是栈上的内容)
//这样就会给用户带来不方便的情况，经过思考之后我觉得还是应该为用户保持一段连续的内存区以保持用户处理的一个请求。
//我的策略是用户在收到一个请求之后(可以在库中调用回调函数之前)，调用一次force_continuous_memory接口，这时候再进行必要的复制操作
//复试可以是这样的，如果当前已经在一个连续的空间中，直接返回首地址，如果不能够则需要一些复制或者移动操作...
//这种内存处理方式也算是一种lazy方式吧...
//
//BUG again : 之前一直没有考虑关于Buffer的连续性，其实提供给用户的缓冲区的内容必须是连续的，而非连续的几段内存需要再次将它们拷贝到
//一段连续的内存中，这将会是得不偿失的，并且这样几段内存的设计还会带来复杂性，所以不如直接使用一段连续的内存。
//最后的方案是这样的：使用单焕冲的方式，也不再使用循环缓冲区，每次添加只在尾部添加，当尾部的空间不足的时候再尝试移动...

typedef struct CBuffer
{
    unsigned char *buffer;          //array pointer of the buffer , which is a character array...
    unsigned int read_index;        //循环缓冲区的有效数据的起始index，read means read data from this index...
    unsigned int write_index;       //first unused index in this buffer , write means write data to this buffer started from this index...
    unsigned int empty_size;        //unused size , include head empty size and tail empty size , equals to (buffer_size + write_index - read_index) % buffer_size ...
    unsigned int buffer_size;       //current buffer size...
}CBuffer;

struct Buffer
{
    struct CBuffer rbuffer;
    struct CBuffer wbuffer;
};

#define INIT_MAX_SIZE 65536      //max size of init...64k...
#define DEFAULT_MAX_SIZE  65536
#define DEFAULT_INIT_SIZE 1024   //default init size of read&write size ...
#define ALIGN_TO_KB(size)  ((((size) >> 10) + (!!(size % 1024))) << 10)
#define FABS(value)        ((value) > 0 ? (value) : 0 - (value))
#define START_ID           1

static void init_cycle_buffer(CBuffer *buffer , int init_size)
{
    buffer->read_index = buffer->write_index = 0;
    buffer->empty_size = 0;
    buffer->buffer_size = init_size;
    buffer->buffer = NULL;      //allocate memory when first used...
}

static int real_init_cycle_buffer(CBuffer * buffer)
{
    int size = buffer->buffer_size;
    if(size == 0)
        size = DEFAULT_INIT_SIZE;

    unsigned char *buf = (unsigned char *)calloc(size , 1);
    if(NULL == buf)
    {
        LOG_ERROR("Allocate memory size %d when init cycle buffer error ..." , size);
        return -1;
    }

    buffer->buffer = buf;
    buffer->buffer_size = size;
    buffer->empty_size = size;
    
    return 0;
}

#define MAX_NEXT   65536
#define NEXT_STEP  4096
#define NEXT_BUFFER_SIZE(size)    (size >= MAX_NEXT ? size + NEXT_STEP : size << 1)

static int expand_cycle_buffer(CBuffer *buffer , unsigned int len)
{
    int flag = 0;
    unsigned int all = buffer->buffer_size;
    unsigned int new_size = (buffer->write_index + all - buffer->read_index) % all + len;
    new_size = ALIGN_TO_KB(new_size);
    unsigned int next_size = NEXT_BUFFER_SIZE(all);
    if(new_size < next_size)
        new_size = next_size;

    unsigned char *new_buf = (unsigned char *)realloc(buffer->buffer , new_size);
    if(NULL == new_buf)
    {
        LOG_ERROR("Reallocate memory size %d failed when expand buffer ..." , new_size);
        return -1;
    }

    buffer->buffer = new_buf;
    buffer->buffer_size = new_size;

    buffer->empty_size = (buffer->write_index - buffer->read_index + new_size) % new_size;
    return 0;
}

//copy memory to cycle srack...
static int copy_to_cycle_buffer(CBuffer *buffer , unsigned char *buf , unsigned int len)
{
    if(NULL == buffer->buffer && real_init_cycle_buffer(buffer) < 0)
    {
        LOG_WARNING("Do real init buffer failed when copy to buffer ...");
        return -1;
    }
    if(buffer->empty_size < len && expand_cycle_buffer(buffer , len) < 0)
    {
        LOG_WARNING("Do append buffer failed when copy to buffer ...");
        return -1;
    }

    unsigned char *head = buffer->buffer;
    unsigned int  last_size = buffer->buffer_size - buffer->write_index;
    if(last_size < len)
    {
        unsigned int read_index = buffer->read_index;
        memmove(head , head + read_index , read_index - buffer->write_index);
        buffer->read_index = 0;
        buffer->write_index -= read_index;
    }
    
    if(buffer->empty_size < len)
        LOG_FATAL("not enough memory after expand ...");

    memcpy(head + buffer->write_index , buf , len);
    buffer->write_index += len;
    buffer->empty_size -= len;

    return 0;
}

//create a buffer , but do not allocate any real buffer memory , include the list...
Buffer *create_buffer(int read_size , int write_size)
{
    int init_read_size = 0;
    int init_write_size = 0;
    if(read_size < 0 || read_size > INIT_MAX_SIZE)
        init_read_size = DEFAULT_INIT_SIZE;
    if(write_size < 0 || write_size > INIT_MAX_SIZE)
        init_write_size = DEFAULT_INIT_SIZE;

    init_read_size = ALIGN_TO_KB(init_read_size);
    init_write_size = ALIGN_TO_KB(init_write_size);
        
    Buffer *buf = (Buffer *)malloc(sizeof(Buffer));
    if(NULL == buf)
    {
        LOG_ERROR("Allocate memory for buffer object of size %lu failed ..." , sizeof(Buffer));
        return NULL;
    }
    
    init_cycle_buffer(&(buf->rbuffer) , init_read_size);
    init_cycle_buffer(&(buf->wbuffer) , init_write_size);

    return buf;
}

//copy data to read buffer , return 0 means everything is OK , return -1 means error happened...
int copy_to_read_buffer(Buffer *buf , unsigned char *stack , unsigned int size)
{
    if(NULL == stack || 0 == size)
        return ;

    CBuffer *in_buffer = &(buf->rbuffer);
    
    return copy_to_cycle_buffer(in_buffer , stack , size);
}

void moveout_read_buffer(Buffer *buf , int len)
{
    CHECK_BUFFER_NOT_NULL_VOID(buf);

    CBuffer *buffer = &(buf->rbuffer);
    buffer->read_index += len;
    buffer->empty_size += len;
    if(buffer->read_index == buffer->write_index)
    {
        buffer->read_index = 0;
        buffer->write_index = 0;
    }
}

int decrease_read_buffer(Buffer *buf , int new_size)
{
    CBuffer *buffer = &(buf->rbuffer);
    if(NULL == buffer || buffer->buffer_size < new_size)
        return 0;

    unsigned char *head = buffer->buffer;
    unsigned int cur_size = buffer->write_index - buffer->read_index;
    if(cur_size > new_size || new_size < buffer->write_index)
    {
        memmove(head , head + buffer->read_index , cur_size);
        buffer->read_index = 0;
        buffer->write_index -= cur_size;
        if(new_size < cur_size)
            new_size = ALIGN_TO_KB(cur_size);
    }
    
    new_size = ALIGN_TO_KB(new_size);
    unsigned char *new_buf = (unsigned char *)realloc(head , new_size);
    if(NULL == new_buf)
    {
        LOG_ERROR("Reallocate memory size %d failed when decrease buffer ..." , new_size);
        return -1;
    }

    buffer->buffer = new_buf;
    buffer->buffer_size = new_size;
    buffer->empty_size = new_size - (buffer->write_index - buffer->read_index);

    return 0;
}

unsigned int get_current_read_buffer_size(Buffer *buf)
{
    return buf->rbuffer.buffer_size;
}

unsigned char *get_current_read_buffer(Buffer *buf)
{
    CBuffer *buffer = &(buf->rbuffer);
    return (buffer->buffer + buffer->read_index);
}

unsigned int get_current_read_buffer_length(Buffer *buf)
{
    CBuffer *buffer = &(buf->rbuffer);
    return (buffer->write_index - buffer->read_index);
}

static int write_data_to_cycle_buffer(CBuffer *buffer , void *from , unsigned int len)
{
    unsigned int readindex = buffer->read_index;
    unsigned int writeindex = buffer->write_index;
    unsigned int cur_size = buffer->buffer_size;

    if(NULL == buffer->buffer && real_init_cycle_buffer(buffer) < 0)
    {
        LOG_WARNING("Do real init buffer failed when copy to write buffer ...");
        return -1;
    }

    if(buffer->empty_size < len && expand_cycle_buffer(buffer , len) < 0)
    {
        LOG_ERROR("expand buffer failed when write to buffer ...");
        return -1;
    }
    
    unsigned int new_size = buffer->buffer_size;
    if(writeindex <= readindex)
    {
        unsigned int expand_size = new_size - cur_size;
        if(expand_size >= writeindex)
        {
            memcpy(buffer->buffer + cur_size , buffer->buffer , writeindex);
            buffer->write_index = (writeindex + cur_size) % new_size;
        }
        else 
        {
            memcpy(buffer->buffer + cur_size , buffer->buffer , expand_size);
            memmove(buffer->buffer , buffer->buffer + expand_size , writeindex - expand_size);

            buffer->write_index -= expand_size;
        }
    }
    
    int ret = 0; 
    writeindex = buffer->write_index;
    unsigned int last_size = new_size - writeindex;
    if(last_size >= len)
    {
        memcpy(buffer->buffer + writeindex , from , len);
        buffer->write_index = (len + writeindex) % new_size;
    }
    else 
    {
        unsigned int gap = len - last_size;
        memcpy(buffer->buffer + writeindex , from , last_size);
        memcpy(buffer->buffer , from + last_size , gap);
        buffer->write_index = gap;
        ret = 1;
    }

    return ret;
}

//write some data to write buffer ...
//flag = 0 means no callback after those data write to kernel ...
//flag = 1 means it will do callback after those data write to kernel ...
int write_data_to_buffer(Buffer *buf , void *from , unsigned int len , int flag)
{
    CBuffer *buffer = &(buf->wbuffer);
   
    int ret = write_data_to_cycle_buffer(buffer , from , len);
    if(ret < 0)
    {
        LOG_ERROR("write data of len %d to cycle buffer failed ..." , len);
        return -1;
    }
    return ret;
}

void get_from_write_buffer(Buffer *buf , int len)
{
    CHECK_BUFFER_NOT_NULL_VOID(buf);

    CBuffer *wbuf = &(buf->wbuffer);

    unsigned int rindex = wbuf->read_index;
    unsigned int windex = wbuf->write_index;
    unsigned int tail_size = wbuf->buffer_size - rindex;
    if(windex <= rindex && tail_size < len)
    {
        wbuf->write_index -= (len - tail_size);
        wbuf->read_index = 0;
    }
    else 
    {
        wbuf->read_index += len;
    }

    wbuf->empty_size -= len;
}

void get_writeable_pointer(Buffer *buf , unsigned char **first , unsigned int *first_len , unsigned char **second , unsigned int *second_len)
{
    CHECK_BUFFER_NOT_NULL_VOID(buf);

    CBuffer *wbuf = &(buf->wbuffer);
    unsigned int rindex = wbuf->read_index;
    unsigned int windex = wbuf->write_index;
    *first = wbuf->buffer + rindex;
    if(windex <= rindex)
    {
        *first_len = wbuf->buffer_size - rindex;
        *second = wbuf->buffer;
        *second_len = windex;
    }
    else 
    {
        *first_len = windex - rindex;
        *second = NULL;
        *second_len = 0;
    }
}


