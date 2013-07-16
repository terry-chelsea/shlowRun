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

#include<stdarg.h>
#include <stdio.h>

/* 使用方式 ：使用START_DEBUG宏定义初始化启动调试，参数是特定的文件名和日志类型
 * 使用结束需要调用FINISH_DEBUG释放内存 
 * 如果没有启动调试就调用接口，会打印一次Hint,以后也不会有任何输出
 * 可以使用的接口有下面的宏定义
 * 使用方式和标准的格式化输出一致
 */

#define MY_FILE_NAME_LENGTH 256                 //日志文件的最大长度
#define MY_MAX_LINE         512                 //一行缓冲区的最大长度
#define COUT_ONE_LINE       16                  //打印二进制的时候一行的字符数
#define ADDR_LENGTH         16                  //IP地址的最大字节数
#define BIN_CODE_LENGTH     (COUT_ONE_LINE * 2 + COUT_ONE_LINE / 2)


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
    SYSCALL
}LEVEL_;

static char *strings[] = 
{
    "[DEBUG]" , 
    "[INFO]" , 
    "[WARNING]" ,
    "[ERROR]" ,
    "[SYSCALL]"
};

typedef struct 
{
    LEVEL_   level;
    OUT_TYPE type;
    FILE     *fp;
}MYDEBUG;

extern MYDEBUG *my_debug_ptr;

extern void beginDebug(char * , OUT_TYPE type , LEVEL_ level);

extern void endDebug();

extern void new_line();

extern void set_output_type(OUT_TYPE type);

extern void set_debug_level(LEVEL_ level);

extern void printDebugInfo(char * , ...);

extern void printDebugInfoWithoutTime(char *format , ...);

extern void binDebugOutput(void * , int );

#define START_DEBUG(app_name , type , level) do{ \
    beginDebug(app_name , type , level); \
    new_line(); \
}while(0)

#define FINISH_DEBUG() do{ \
    new_line(); \
    endDebug(); \
}while(0)

//only output bin code infomation when level is DEBUG...
#define DEBUG_BIN_CODE(struct_ptr , s_size) do{    \
    if(NULL == my_debug_ptr || my_debug_ptr->level != DEBUG) break; \
    new_line(); \
    printDebugInfoWithoutTime("[In file : %s , Line %d] : Output bin code :\n" , __FILE__ , __LINE__); \
    binDebugOutput(struct_ptr , s_size); \
    printDebugInfoWithoutTime("End of output bin code!\n\n"); \
}while(0)

#define DEBUG_BIN_CODE_TIME(struct_ptr , s_size) do{    \
    if(NULL == my_debug_ptr || my_debug_ptr->level != DEBUG) break; \
    new_line(); \
    printDebugInfo("[In file : %s , Line %d] : Output bin code :\n" , __FILE__ , __LINE__); \
    binDebugOutput(struct_ptr , s_size); \
    printDebugInfo("End of output bin code!\n\n"); \
}while(0)

#define DEBUG_OUTPUT(line , format , ...)  do{ \
    printDebugInfoWithoutTime("[In file : %s , Line %d] : " , __FILE__ , __LINE__); \
    printDebugInfoWithoutTime(format , ##__VA_ARGS__); \
    if(line)    new_line(); \
}while(0)

#define __LOG_SOMETHING(_level , tm , line , format , ...) do{ \
    if(NULL == my_debug_ptr || my_debug_ptr->level > _level) break; \
    if(tm)    printDebugInfo(strings[_level]); \
    else      printDebugInfoWithoutTime(strings[_level]); \
    DEBUG_OUTPUT(line , format , ##__VA_ARGS__); \
}while(0)

#define LOG_SOMETHING(_level , tm , format , ...) \
    __LOG_SOMETHING(_level , tm , 1 , format , ##__VA_ARGS__)

#define LOG_DEBUG(format , ...)  \
    LOG_SOMETHING(DEBUG , 0 , format , ##__VA_ARGS__)
#define LOG_DEBUG_TIME(format , ...) \
    LOG_SOMETHING(DEBUG , 1 , format , ##__VA_ARGS__)

#define LOG_INFO(format , ...) \
    LOG_SOMETHING(INFO , 0 , format , ##__VA_ARGS__)
#define LOG_INFO_TIME(format , ...) \
    LOG_SOMETHING(INFO , 1 , format , ##__VA_ARGS__)

#define LOG_WARNING(format , ...) \
    LOG_SOMETHING(WARNING , 0 , format , ##__VA_ARGS__)
#define LOG_WARNING_TIME(format , ...) \
    LOG_SOMETHING(WARNING , 1 , format , ##__VA_ARGS__)

#define LOG_ERROR(format , ...) \
    LOG_SOMETHING(MAX_LEVEL , 0 , format , ##__VA_ARGS__)
#define LOG_ERROR_TIME(format , ...) \
    LOG_SOMETHING(MAX_LEVEL , 1 , format , ##__VA_ARGS__)

#define LOG_SYSCALL(format , ...) do{ \
    __LOG_SOMETHING(SYSCALL , 0 , 0 , format , ##__VA_ARGS__); \
    printDebugInfoWithoutTime(" Reason : %s" , strerror(errno)) \
    new_line(); \
}while(0)

#define LOG_SYSCALL_TIME(format , ...) do{ \
    __LOG_SOMETHING(SYSCALL , 1 , 0 , format , ##__VA_ARGS__); \
    printDebugInfoWithoutTime(" Reason : %s" , strerror(errno)); \
    new_line(); \
}while(0)

#define SET_DEBUG_TYPE(_type)    set_output_type(_type)
#define SET_DEBUG_LEVEL(_level)  set_debug_level(_level);

#endif

