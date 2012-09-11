/**
 *Description:      日志处理.
 *                  根据日志级别仅输出低于初始日志级别低的日志
 *
 *Notice:           当日志级别低于LOG_LEVEL_DEBUG时，不记录源文件名称和行号
 *                  这里的函数都是多线程安全的
 **/

#ifndef CLIB_LOG_H_
#define CLIB_LOG_H_

#include "types.h"

typedef struct _log log_t;

//日志级别
enum log_level {
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_TEST,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE
};

/**
 * 全局默认日志对象，默认输出到标准输出，日志级别为LOG_LEVEL_TRACE
 * 进程可以修改该日志对象,示例如下
 * if(g_default_log != NULL){
 *      log_free(g_default_log);
 * }
 * g_default_log = log_new_file(LOG_LEVEL_WARNING, "test");
 **/
extern log_t* g_default_log;

/*
 *@brief        创建标准输出日志对象
 *@param in     level    日志级别
 *@return       NULL表示失败
 */
log_t* log_new_stdout(enum log_level level);

/*
 *@brief        创建文件日志对象
 *@param in     level       日志级别
 *@param in     filename    日志文件名称
 *@return       NULL表示失败
 */
log_t* log_new_file(enum log_level level, const char *filename);

void log_set_level(log_t* log, enum log_level level);

/*
 *@brief        释放日志对象
 *@param in     log    日志对象
 *@return
 */
void log_free(log_t* log);

/*
 *@brief        记录fmt参数指定的日志
 *@param in     log         日志对象
 *@param in     source_file 源代码文件名称
 *@param in     line        源代码所在行号
 *@param in     level       日志级别
 *@param in     fmt         日志内容
 *@return
 */
void log_str(log_t* log, const char* source_file, int line,
        enum log_level level, const char* fmt, ...);

/*
 *@brief        记录fmt参数指定的日志，并记录errno对应的错误信息
 *@param in     log         日志对象
 *@param in     source_file 源代码文件名称
 *@param in     line        源代码所在行号
 *@param in     level       日志级别
 *@param in     fmt         日志内容
 *@return
 */
void log_errno(log_t* log, const char* source_file, int line,
        enum log_level level, const char* fmt, ...);

//常用的宏定义
#define LOG(level, fmt, a...) log_str(g_default_log, __FILE__, __LINE__, level, fmt, ##a)

#define LOG_ERRNO(level, fmt, a...) log_errno(g_default_log, __FILE__, __LINE__, level, fmt, ##a)


#endif /* CLIB_LOG_H_ */
