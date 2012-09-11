#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "time.h"
#include <unistd.h>
#include "memory.h"
#include <pthread.h>

void time_format(time_t time, char *buf, uint len)
{
    struct tm tm_now;
    localtime_r(&time, &tm_now);

    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d", tm_now.tm_year + 1900,
            tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min,
            tm_now.tm_sec);
}


#define LOCAL_LOG(level, fmt, a...) local_log(__FILE__, __LINE__,  level, fmt, ##a)

log_t* g_default_log = NULL;
static pthread_mutex_t l_default_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t l_local_log_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_LOG_LENGTH 1024

typedef void (*log_func)(void* log_param, const char* loginfo);
typedef void (*log_destory_func)(void* log_param);

struct _log {
    void* log_param;
    log_func log;
    int level;
    log_destory_func log_destory;
    pthread_mutex_t mutex;
};

struct rolling_file_param {
    FILE* fd;                                     //文件句柄
    uint16_t current_file_number;                  //当前到第几个文件
    uint16_t max_file_number;                      //最大文件计数
    uint max_size;                                //最大文件规格
    char file_name[MAX_PATH_NAME_LEN];            //文件名称
};

static void local_log(const char* source_file, int line, enum log_level level,
        const char* fmt, ...);

static void log_stdout(void *param, const char* loginfo);

static void* log_file_new(const char *filename);
static void log_file(void *param, const char* loginfo);
static void log_file_destory(void *param);

void log_set_level(log_t* log, enum log_level level)
{
    if (log == NULL) {
        return;
    }

    struct _log *_log = (struct _log *) log;
    pthread_mutex_lock(&_log->mutex);
    _log->level = level;
    pthread_mutex_unlock(&_log->mutex);
}

log_t* log_new_stdout(enum log_level level)
{
    struct _log *_log = (struct _log*) mem_alloc(sizeof(struct _log));

    _log->log_param = NULL;
    _log->log = log_stdout;
    _log->level = level;
    _log->log_destory = NULL;
    pthread_mutex_init(&_log->mutex, NULL);

    return _log;
}

log_t* log_new_file(enum log_level level, const char *filename)
{
    void *log_param;
    struct _log *_log;

    log_param = log_file_new(filename);
    if (log_param == NULL) {
        return NULL;
    }
    _log = (struct _log*) mem_alloc(sizeof(struct _log));

    _log->log_param = log_param;
    _log->log = log_file;
    _log->level = level;
    _log->log_destory = log_file_destory;
    pthread_mutex_init(&_log->mutex, NULL);

    return _log;
}

void log_free(log_t* log)
{
    struct _log *_log = (struct _log *) log;
    if (_log->log_param != NULL) {
        _log->log_destory(_log->log_param);
    }
    pthread_mutex_destroy(&_log->mutex);
    mem_free(_log);
}

static void create_default_log()
{
    if (g_default_log == NULL) {
        g_default_log = log_new_stdout(LOG_LEVEL_TRACE);
    }
}

static bool prepare_log(struct _log *_log, const char* source_file, int line,
        enum log_level level, char* buf, uint buflen)
{
    int tmp_buf_len;

    if (_log == NULL) {
        return false;
    }

    time_format(time(NULL), buf, buflen);
    tmp_buf_len = strlen(buf);
    switch (level) {
    case LOG_LEVEL_TRACE:
        strncat(buf, " [TRACE] ", buflen - tmp_buf_len);
        break;
    case LOG_LEVEL_DEBUG:
        strncat(buf, " [DEBUG] ", buflen - tmp_buf_len);
        break;
    case LOG_LEVEL_TEST:
        strncat(buf, " [TEST] ", buflen - tmp_buf_len);
        break;
    case LOG_LEVEL_WARNING:
        strncat(buf, " [WARNING] ", buflen - tmp_buf_len);
        break;
    case LOG_LEVEL_ERROR:
        strncat(buf, " [ERROR] ", buflen - tmp_buf_len);
        break;
    default:
        break;
    }
    buf[buflen - 1] = '\0';
    tmp_buf_len = strlen(buf);
    if (_log->level > LOG_LEVEL_TEST) {
        snprintf(buf + tmp_buf_len, buflen - tmp_buf_len, "%s(%d):",
                source_file, line);
        buf[buflen - 1] = '\0';
    }

    return true;
}

static void local_log(const char* source_file, int line, enum log_level level,
        const char* fmt, ...)
{
    va_list vap;
    char log_buf[MAX_LOG_LENGTH];
    int buflen;
    struct _log log;

    bzero(&log, sizeof(struct _log));
    log.level = level;

    if (!prepare_log(&log, source_file, line, level, log_buf,
            sizeof(log_buf))) {
        return;
    }

    buflen = strlen(log_buf);
    va_start(vap, fmt);
    vsnprintf(log_buf + buflen, sizeof(log_buf) - buflen, fmt, vap);
    log_buf[sizeof(log_buf) - 1] = '\0';
    va_end(vap);

    pthread_mutex_lock(&l_local_log_mutex);
    log_stdout(NULL, log_buf);
    pthread_mutex_unlock(&l_local_log_mutex);
}

void log_str(log_t* log, const char* source_file, int line,
        enum log_level level, const char* fmt, ...)
{

    struct _log *_log = (struct _log *) log;

    va_list vap;
    char log_buf[MAX_LOG_LENGTH];
    int buflen;

    if (_log == NULL) {
        pthread_mutex_lock(&l_default_log_mutex);
        create_default_log();
        _log = g_default_log;
        pthread_mutex_unlock(&l_default_log_mutex);
    }

    if (level > _log->level) {
        return;
    }

    pthread_mutex_lock(&_log->mutex);
    if (!prepare_log(_log, source_file, line, level, log_buf,
            sizeof(log_buf))) {
        pthread_mutex_unlock(&_log->mutex);
        return;
    }

    buflen = strlen(log_buf);
    va_start(vap, fmt);
    vsnprintf(log_buf + buflen, sizeof(log_buf) - buflen, fmt, vap);
    log_buf[sizeof(log_buf) - 1] = '\0';
    va_end(vap);
    _log->log(_log->log_param, log_buf);
    pthread_mutex_unlock(&_log->mutex);

}

void log_errno(log_t* log, const char* source_file, int line,
        enum log_level level, const char* fmt, ...)
{
    struct _log *_log = (struct _log *) log;
    va_list vap;
    char log_buf[MAX_LOG_LENGTH];
    int buflen;
    char errno_str[MAX_LOG_LENGTH];

    strerror_r(errno, errno_str, sizeof(errno_str));
    errno_str[sizeof(errno_str) - 1] = '\0';

    if (_log == NULL) {
        pthread_mutex_lock(&l_default_log_mutex);
        create_default_log();
        _log = g_default_log;
        pthread_mutex_unlock(&l_default_log_mutex);
    }

    if (level > _log->level) {
        return;
    }

    pthread_mutex_lock(&_log->mutex);
    if (!prepare_log(_log, source_file, line, level, log_buf,
            sizeof(log_buf))) {
        pthread_mutex_unlock(&_log->mutex);
        return;
    }

    buflen = strlen(log_buf);
    va_start(vap, fmt);
    vsnprintf(log_buf + buflen, sizeof(log_buf) - buflen, fmt, vap);
    log_buf[sizeof(log_buf) - 1] = '\0';
    va_end(vap);

    buflen = strlen(log_buf);
    snprintf(log_buf + buflen, sizeof(log_buf) - buflen, " (%s)", errno_str);
    log_buf[sizeof(log_buf) - 1] = '\0';

    _log->log(_log->log_param, log_buf);
    pthread_mutex_unlock(&_log->mutex);
}

/*
 *  stdout log
 */
static void log_stdout(void *param, const char* loginfo)
{
    printf("%s\n", loginfo);
}

/*
 *  file log
 */
static void* log_file_new(const char *filename)
{
    FILE *file = fopen(filename, "a");

    if (file == NULL) {
        LOCAL_LOG(LOG_LEVEL_ERROR, "打开日志文件失败%s:", filename);
    }

    return file;
}

static void log_file_destory(void *param)
{
    fclose((FILE*) param);
}

static void log_file(void *param, const char* loginfo)
{
    fprintf((FILE*) param, "%s\n", loginfo);
}

