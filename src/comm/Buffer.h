/*
 * =====================================================================================
 *
 *       Filename:  Buffer.h
 *
 *    Description:  connection buffer interface ... 
 *
 *        Version:  1.0
 *        Created:  07/22/13 02:19:47
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#ifndef _H_INTERNEL_CONNECTION_BUFFER_H_
#define _H_INTERNEL_CONNECTION_BUFFER_H_

typedef struct Buffer Buffer;

typedef struct 
{
    unsigned char *buffer;          //array pointer of the buffer , which is a character array...
    unsigned int max_size;          //循环缓冲区的存储上限，对于读缓冲区，超出改上限将取消读操作，对于写操作，超出上限，返回调用者-1...
    unsigned int read_index;        //循环缓冲区的有效数据的起始index，read means read data from this index...
    unsigned int write_index;       //first unused index in this buffer , write means write data to this buffer started from this index...
    unsigned int empty_size;        //unused size , include head empty size and tail empty size , equals to (buffer_size + write_index - read_index) % buffer_size ...
    unsigned int buffer_size;       //current buffer size...
}CBuffer;

struct RBuffer
{
     CBuffer       read_buffer;    //读缓冲区就是一个单循环缓冲区...
};

struct Winfo
{
    unsigned int  index;
    unsigned int  id;
};

struct WBuffer
{
    QUEUE      *callback_list;    //this is a queue marks the index which should do callback after write to kernel...
    NODE       *second_head;
    unsigned int  current_id;
    CBuffer    write_buffer;      //this is the real buffer of data to be send...
};

struct Buffer
{
    struct RBuffer rbuffer;
    struct WBuffer wbuffer;
};

#endif
