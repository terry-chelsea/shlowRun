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

extern Buffer *create_buffer(int read_size , int write_size);

extern int copy_to_read_buffer(Buffer *buf , unsigned char *stack , unsigned int size);

extern void moveout_read_buffer(Buffer *buf , int len);

extern int decrease_read_buffer(Buffer *buf , int new_size);

extern unsigned int get_current_read_buffer_size(Buffer *buf);

extern unsigned char *get_current_read_buffer(Buffer *buf);

extern unsigned int get_current_read_buffer_length(Buffer *buf);


extern int write_data_to_buffer(Buffer *buf , void *from , unsigned int len , int flag);

extern void get_from_write_buffer(Buffer *buf , int len);

extern void get_writeable_pointer(Buffer *buf , unsigned char **first , unsigned int *first_len , unsigned char **second , unsigned int *second_len);

#endif
