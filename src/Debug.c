/*
 * =====================================================================================
 *
 *       Filename:  Debug.cc
 *
 *    Description:  functions of debug 
 *
 *        Version:  1.0
 *        Created:  03/28/12 23:00:30
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include "Debug.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

MYDEBUG *my_debug_ptr = NULL;
#define DO_ARG(resBuf , size , count , format) do{\
        va_list pArgList; \
        va_start(pArgList , format); \
        count = vsnprintf(resBuf , size , format , pArgList);\
        va_end(pArgList);\
}while(0)

/* 将格式化字符串输入到一个缓冲区去中...
 * 参数 resBuf 指定输出缓冲区
 * 参数 nSize 指定输出缓冲区的大小
 * 其他参数是格式化字符串信息
 * 函数返回值为有效地字符串的大小
 * */
int mySprintf(char *resBuf , int nSize , char *format , ...)
{
    int nCount = -1;
    if(nSize < 0 || NULL == resBuf)
        return -1;

    nCount = 0;
    DO_ARG(resBuf , nSize , nCount , format);

    return nCount;
}

static void getTime(char *timeBuf , int nSize)
{
    struct timeval ntime;
    gettimeofday(&ntime , NULL);
    struct tm pTime;
    localtime_r(&(ntime.tv_sec) , &pTime);

    int nLength = mySprintf(timeBuf , nSize , "%d:%d:%d:%d" , 
            pTime.tm_year + 1900 , pTime.tm_mon + 1 , pTime.tm_mday , ntime.tv_usec);

    timeBuf[nLength - 1] = '\0';
}

#define DEFAULT_NAME "unknow_proc_debug"
void beginDebug(char *appName , OUT_TYPE type , LEVEL_ level)
{
    MYDEBUG *debug = NULL;
    char myFileName[MY_FILE_NAME_LENGTH];

    while((debug = (MYDEBUG *)malloc(sizeof(MYDEBUG))) == NULL);      //保证执行成功
    memset(debug , 0 , sizeof(MYDEBUG));

    int length = 0;
    time_t now = time(NULL);
    struct tm tm_;
    localtime_r(&now , &tm_);
    if(NULL == appName)
        length = mySprintf(myFileName , MY_FILE_NAME_LENGTH , "%s_%d_%d_%d" , 
                DEFAULT_NAME , tm_.tm_year + 1900 , tm_.tm_mon + 1 , tm_.tm_mday);
    else 
        length = mySprintf(myFileName , MY_FILE_NAME_LENGTH , "%s_debug_%d_%d_%d" , 
                appName , tm_.tm_year + 1900 , tm_.tm_mon + 1 , tm_.tm_mday);
    myFileName[length] = '\0';

    if(type >= OUT_SCR && type <= OUT_BOTH)
        debug->type = type;
    else 
        debug->type = OUT_SCR;

    if(level >= DEBUG && type <= WARNING)
        debug->level = level;
    else 
        debug->level = WARNING;

    if(debug->type >= OUT_FILE)
        while((debug->fp = fopen(myFileName , "a+")) == NULL);    //保证总是执行成功
    
    my_debug_ptr = debug;

    char timeBuf[MY_MAX_LINE];
    getTime(timeBuf , MY_MAX_LINE);
    if(my_debug_ptr->fp)
        fprintf(my_debug_ptr->fp , 
        "\n\n-------------------[%s] : START Debug...-------------------\n" , timeBuf);
}

void endDebug()
{
    char timeBuf[MY_MAX_LINE];
    getTime(timeBuf , MY_MAX_LINE);
    if(my_debug_ptr->fp)
        fprintf(my_debug_ptr->fp , 
        "\n\n-------------------[%s] : FINISH_Debug...-------------------\n" , timeBuf);

    if(NULL == my_debug_ptr)
        return ;

    if(my_debug_ptr->type >= OUT_FILE)
        fclose(my_debug_ptr->fp);
    free(my_debug_ptr);

    my_debug_ptr = NULL;
}

void set_output_type(OUT_TYPE type)
{
    if(NULL == my_debug_ptr)
        return ;

    //here open new file...
    if(type >= OUT_SCR && type <= OUT_BOTH)
        my_debug_ptr->type = type;
    else 
        my_debug_ptr->type = OUT_SCR;
}

void set_debug_level(LEVEL_ level)
{
    if(NULL == my_debug_ptr)
        return ;

    if(level >= DEBUG && level <= WARNING)
        my_debug_ptr->level = level;
    else 
        my_debug_ptr->level = WARNING;
}

static void do_print_debug_info(char *msgBuf , char *timeBuf)
{
    if(NULL == msgBuf)
        return ;

    if(NULL == timeBuf)
    {
        if(OUT_BOTH == my_debug_ptr->type)
        {
            fprintf(my_debug_ptr->fp , "%s" , msgBuf);
            fprintf(stdout , "%s" , msgBuf);
        }
        else if(OUT_FILE == my_debug_ptr->type)
        {
            fprintf(my_debug_ptr->fp , "%s" , msgBuf);
        }
        else
        {
            fprintf(stdout , "%s" , msgBuf);
        }
    }
    else 
    {
        if(OUT_BOTH == my_debug_ptr->type)
        {
            fprintf(my_debug_ptr->fp , "[%s] : %s" , timeBuf , msgBuf);
            fprintf(stdout , "[%s] : %s" , timeBuf , msgBuf);
        }
        else if(OUT_FILE == my_debug_ptr->type)
        {
            fprintf(my_debug_ptr->fp , "[%s] : %s" , timeBuf , msgBuf);
        }
        else
        {
            fprintf(stdout , "[%s] : %s" , timeBuf , msgBuf);
        }
    }
}

void new_line()
{
    FILE *fp = my_debug_ptr->fp;
    OUT_TYPE type = my_debug_ptr->type;
    if(OUT_BOTH == type || OUT_FILE == type)
        fprintf(fp , "\n");
    if(OUT_BOTH == type || OUT_SCR == type)
        fprintf(stdout , "\n");
}

void printDebugInfoWithoutTime(char *format , ...)
{
    char msgBuf[MY_MAX_LINE];
    int nSize = MY_MAX_LINE;

    if(NULL == format)
    {
        fprintf(stderr , "Hint : error input in printDebugInfo()!\n");
        return ;
    }

    int nCount = 0;
    DO_ARG(msgBuf , nSize , nCount , format);

    msgBuf[nCount] = '\0';
    
    do_print_debug_info(msgBuf , NULL);
}

void printDebugInfo(char *format , ...)
{
    int nCount = -1;
    char msgBuf[MY_MAX_LINE];
    char timeBuf[MY_MAX_LINE];
    int nSize = MY_MAX_LINE;

    if(NULL == format)
    {
        fprintf(stderr , "Hint : error input in printDebugInfo()!\n");
        return ;
    }

    nCount = 0;
    DO_ARG(msgBuf , nSize , nCount , format);

    msgBuf[nCount] = '\0';
    getTime(timeBuf , MY_MAX_LINE);
    
    do_print_debug_info(msgBuf , timeBuf);
}

/* 将原始的字符串和相对于的地址以ASCII形式保存在一段缓存中。
 * 参数 outputBuffer 是保存结果的缓存。
 * 参数 maxSize 是这段缓存的大小。
 * 参数 resBuffer 是原始的字符串。
 * 参数 length 是原始字符串的长度。
 * P.S.这个函数式内部函数，所以不需要安全性检查
 */
static int doOutputBin(char *outputBuffer , int maxSize , char *resBuffer , int length)
{
#define IS_PRINTFUL(ch)  ((ch) >= ' ' && (ch) <= '~')

    int putCount = 0;
    char addrBuffer[ADDR_LENGTH];             //保存每次打印的地址信息
    int  addrCount = 0;              //地址字符串的长度
    char binBuffer[BIN_CODE_LENGTH];             //打印这些字符的ASCII码的信息
    int  binCount = 0;               //二进制码信息长度
    char asciiBuffer[COUT_ONE_LINE + 1];           //保存原始字符的信息
    int  asciiCount = 0;             //原始字符的长度

    addrCount = mySprintf(addrBuffer , 32 , "[ %0X ]" , resBuffer);         //首先得到这段原始字符串的首地址。

    int i = 0;
    for(i = 0 ; i < length ; ++i)               
    {
        char tempChar = *(resBuffer + i);
        binCount += mySprintf(binBuffer + binCount , 128 - binCount , "%02X" , (unsigned char )tempChar);         //将每一个字节的转换成ASCII形式。
        if(0 == (i + 1) % 2)
            binCount += mySprintf(binBuffer + binCount , 128 - binCount , " ");                                   //两个字符之间使用一个空格

        if(!IS_PRINTFUL(tempChar))         //不可打印字符使用这个字符表示
            tempChar = '@';

        asciiCount += mySprintf(asciiBuffer + asciiCount , 128 - asciiCount , "%c" , tempChar);                   //保存原始字符，因为参数不是以'\0'结束的。            
    }
    
    while(binCount < BIN_CODE_LENGTH)                        //在ASCII字符串之后补全空格
        binCount += mySprintf(binBuffer + binCount , 128 - binCount , " ");

    while(addrCount < ADDR_LENGTH)                           //将地址字符串补全为指定的字节数。
        addrCount += mySprintf(addrBuffer + addrCount , 32 - addrCount , "-");

    int allCount = mySprintf(outputBuffer , maxSize , "%s----%s   %s" , addrBuffer , binBuffer , asciiBuffer);

    outputBuffer[allCount + 1] = '\0';
    outputBuffer[allCount] = '\n';

    return i;
}

/* 将传入的一段字符串以ASCII编码的形式输出。
 * 参数 debug 是指定的句柄
 * 参数 structBuffer 是这段内存的首地址。
 * 参数 nSize 是这段内存的需要打印的长度。
 */
void binDebugOutput(void *structBuffer , int nSize)
{
    char outputBuf[MY_MAX_LINE];
    int i = 0;
    int beginAddr = 0;
    int leftCount = nSize;
    char *sBuffer = (char *)structBuffer;

    if(nSize <= 0 || NULL == structBuffer)
    {
        fprintf(stderr , "Hint : input error of binDebugOutput()!\n");
        return ;
    }

    while(leftCount > 0)
    {
        int outputCount = 0;
        if(leftCount > COUT_ONE_LINE)
            outputCount = doOutputBin(outputBuf , MY_MAX_LINE , sBuffer + beginAddr , COUT_ONE_LINE);
        else
            outputCount = doOutputBin(outputBuf , MY_MAX_LINE , sBuffer + beginAddr , leftCount);

        printDebugInfoWithoutTime("%s" , outputBuf);

        beginAddr += outputCount;
        leftCount -= outputCount;
    }
}


#ifdef TEST_DEBUG_MAIN

#include <string.h>
#include <errno.h>

int main(int argc , char *argv[])
{
    char buf[128] = "this is just a line for test the out put thing in bin and ascii code...";

    START_DEBUG(argv[0] , 0 , 3);

    DEBUG_BIN_CODE(buf , strlen(buf));

    char someting[100];
    DEBUG_BIN_CODE_TIME(someting , sizeof(someting));

    LOG_INFO("Hello %d world %d" , 16 , 116);

    LOG_WARNING("Hello %d world %d" , 16 , 116);

    LOG_ERROR("Hello %d world %d" , 16 , 116);

    LOG_ERROR_TIME("Hello %d world %d" , 16 , 116);

    SET_DEBUG_TYPE(0);

    LOG_INFO("test after set only output to screen...");

    errno = EAGAIN;
    LOG_SYSCALL_TIME("this errno is EAGAIN");

    SET_DEBUG_LEVEL(4);

    LOG_WARNING_TIME("After set debug level to more than warning...");
    LOG_ERROR_TIME("Now this error can be printed...");

    FINISH_DEBUG();

    return 0;
}
    
#endif
