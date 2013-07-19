/*
 * =====================================================================================
 *
 *       Filename:  intmap.h
 *
 *    Description:  a implement of atomic increase array...
 *
 *        Version:  1.0
 *        Created:  07/17/13 01:02:44
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  huyao (H.Y), yaoshunyuhuyao@163.com
 *        Company:  NDSL
 *
 * =====================================================================================
 */

#ifndef _H_SMALL_VECTOR_H_
#define _H_SMALL_VECTOR_H_

/*
 * a implement of vector in C++ , this can only store pointer or integer(!)
 * IT can increase length when need and you must decrease it by youself
 * explicitly . the only error it alloc memory failed...
 * in some way , this is a map : key is the index(interge) and value is
 * the item in the array of this index...
 *
 * you can store a item to vector and it will return a index ...
 * after it , you can get you info use this index...
 */


typedef struct map Intmap;

Intmap *create_intmap(int init_sz);

int   intmap_put_value(Intmap *imap , void *value);

void *intmap_get_value(Intmap *imap , int index);

int intmap_clear_value(Intmap *imap , int index);

int  intmap_length(Intmap *imap);

int  intmap_size(Intmap *imap);

int  intmap_used_memory(Intmap *imap);

void destory_intmap(Intmap *imap);

#endif
