/*
 * =====================================================================================
 *
 *       Filename:  Debug.h
 *
 *    Description:  use those function debug printf some useful imformations...
 *
 *        Version:  1.0
 *        Created:  03/28/12 22:37:42
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (terry),fengyuatad@126.com 
 *        Company:  
 *
 * =====================================================================================
 */
    
#ifndef _H_MY_DEBUG_H_
#define _H_MY_DEBUG_H_

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* 使用方式 ：使用START_DEBUG宏定义初始化启动调试，参数是特定的文件名和日志类型
 * 使用结束需要调用FINISH_DEBUG释放内存 
 * 如果没有启动调试就调用接口，会打印一次Hint,以后也不会有任何输出
 * 可以使用的接口有下面的宏定义
 * 使用方式和标准的格式化输出一致
 */

//you should compile it with C compiler , because C++ will change C string to string , some Warnings...

typedef enum 
{
    OUT_SCR = 0 ,
    OUT_FILE , 
    OUT_BOTH
}OUT_TYPE;

typedef enum
{
    DEBUG = 0, 
    INFO , 
    WARNING , 
    MAX_LEVEL ,
    SYSCALL ,
    FATAL
}LEVEL_;

extern char *strings[]; 

struct debug_union
{
    LEVEL_   level;
    OUT_TYPE type;
    FILE     *fp;
    char     file_name[256];
};

typedef struct debug_union MYDEBUG;

extern MYDEBUG *my_debug_ptr;

extern void beginDebug(char * , OUT_TYPE type , LEVEL_ level);

extern void endDebug();

extern void new_line();

extern void set_output_type(OUT_TYPE type);

extern void set_debug_level(LEVEL_ level);

extern void printDebugInfo(char * , ...);

extern void printDebugInfoWithoutTime(char *format , ...);

extern void binDebugOutput(void * , int );

//interface of start debug , create some info about debug ...
#define START_DEBUG(app_name , type , level) do{ \
    beginDebug(app_name , type , level); \
    new_line(); \
}while(0)

//delete some info...
#define FINISH_DEBUG() do{ \
    new_line(); \
    endDebug(); \
}while(0)

//only output bin code infomation when level is DEBUG...
//only debug level is DEBUG , can you output the binary infomation...
#define DEBUG_BIN_CODE(struct_ptr , s_size) do{    \
    if(NULL == my_debug_ptr || my_debug_ptr->level != DEBUG) break; \
    new_line(); \
    printDebugInfoWithoutTime("[In file : %s , Line %d] : Output bin code :\n" , __FILE__ , __LINE__); \
    binDebugOutput(struct_ptr , s_size); \
    printDebugInfoWithoutTime("End of output bin code!\n\n"); \
}while(0)

//output binary infomation with current time...
#define DEBUG_BIN_CODE_TIME(struct_ptr , s_size) do{    \
    if(NULL == my_debug_ptr || my_debug_ptr->level != DEBUG) break; \
    new_line(); \
    printDebugInfo("[In file : %s , Line %d] : Output bin code :\n" , __FILE__ , __LINE__); \
    binDebugOutput(struct_ptr , s_size); \
    printDebugInfo("End of output bin code!\n\n"); \
}while(0)

//output a volatile string , if line is not equal to zero , start a newline...
#define DEBUG_OUTPUT(line , format , ...)  do{ \
    printDebugInfoWithoutTime("[In file : %s , Line %d] : " , __FILE__ , __LINE__); \
    printDebugInfoWithoutTime(format , ##__VA_ARGS__); \
    if(line)    new_line(); \
}while(0)

//do output according to level , time or not , newline or not...
#define __LOG_SOMETHING(_level , tm , line , format , ...) do{ \
    if(NULL == my_debug_ptr || my_debug_ptr->level > _level) break; \
    if(tm)    printDebugInfo(strings[_level]); \
    else      printDebugInfoWithoutTime(strings[_level]); \
    DEBUG_OUTPUT(line , format , ##__VA_ARGS__); \
}while(0)

//output infomation string with a newline...
#define LOG_SOMETHING(_level , tm , format , ...) \
    __LOG_SOMETHING(_level , tm , 1 , format , ##__VA_ARGS__)

//log debug infomation with time or not...
#define LOG_DEBUG(format , ...)  \
    LOG_SOMETHING(DEBUG , 0 , format , ##__VA_ARGS__)
#define LOG_DEBUG_TIME(format , ...) \
    LOG_SOMETHING(DEBUG , 1 , format , ##__VA_ARGS__)

//log infomation...
#define LOG_INFO(format , ...) \
    LOG_SOMETHING(INFO , 0 , format , ##__VA_ARGS__)
#define LOG_INFO_TIME(format , ...) \
    LOG_SOMETHING(INFO , 1 , format , ##__VA_ARGS__)

//log warning...
#define LOG_WARNING(format , ...) \
    LOG_SOMETHING(WARNING , 0 , format , ##__VA_ARGS__)
#define LOG_WARNING_TIME(format , ...) \
    LOG_SOMETHING(WARNING , 1 , format , ##__VA_ARGS__)

//log error infomations...
#define LOG_ERROR(format , ...) \
    LOG_SOMETHING(MAX_LEVEL , 0 , format , ##__VA_ARGS__)
#define LOG_ERROR_TIME(format , ...) \
    LOG_SOMETHING(MAX_LEVEL , 1 , format , ##__VA_ARGS__)

//log syscall infomation...
#define LOG_SYSCALL(format , ...) do{ \
    __LOG_SOMETHING(SYSCALL , 0 , 0 , format , ##__VA_ARGS__); \
    printDebugInfoWithoutTime(" Reason : %s" , strerror(errno)); \
    new_line(); \
}while(0)

#define LOG_SYSCALL_TIME(format , ...) do{ \
    __LOG_SOMETHING(SYSCALL , 1 , 0 , format , ##__VA_ARGS__); \
    printDebugInfoWithoutTime(" Reason : %s" , strerror(errno)); \
    new_line(); \
}while(0)

//do fatal log , exit immediately...
#define LOG_FATAL(format , ...) do{ \
    __LOG_SOMETHING(FATAL , 1 , 0 , format , ##__VA_ARGS__); \
    FINISH_DEBUG(); \
    exit(-1) ; \
}while(0)

//set output type and level when running...
#define SET_DEBUG_TYPE(_type)    set_output_type(_type)
#define SET_DEBUG_LEVEL(_level)  set_debug_level(_level)

#endif

