#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "types.h"
#include "hash_table.h"
#include "topn.h"
#include "memcache.h"
#include "memory.h"
#include "log.h"
#include "cmdparse.h"

static char *log_file = NULL;
static char *result_file = NULL;
CommandLineOptions_t options[] = {
    { "f", "The name of NAT log file waiting for processing"
        , "log file name", OPTION_REQUIRED, ARG_STR, &log_file },
    { "o", "The output file name"
        , "output file name", OPTION_REQUIRED, ARG_STR, &result_file },
    {NULL, NULL, NULL, 0, 0, NULL}
};
static char description[] = {"Linux NAT log file analyzer\nVersion 1.0\n"};

//struct LogRecord {
//    uint32_t dst_ip;
//    uint16_t dst_port;
//    uint32_t session_count;
//    uint64_t netflow;
//};

bool sort_ip(void *key, void *value, void *user_data);
bool print(void *key, void *value, void *user_data);
//bool getNextLogRecord(FILE *file, LogRecord *record);

#define TOPN 1000
int main(int argc, char*argv[])
{
    if (init_application(argc, argv, options, description) < 0) {
        exit(-1);
    }
    if (strlen(log_file) == 0 || strlen(result_file) == 0) {
        printf("Empty file name\n");
        exit(-1);
    }

    /*
    time_t start = time(NULL);
    srand(start);

    //构造memcache
    mem_cache_t * uint32_cache = mem_cache_create(sizeof(uint32_t), 10000);
    mem_cache_t * uint64_cache = mem_cache_create(sizeof(uint64_t), 10000);

    //生成随机的IP
    uint32_t IP_COUNT = 10000;
    uint32_t random_ip[IP_COUNT];
    for (int i = 0; i < IP_COUNT; i++) {
        random_ip[i] = rand() % UINT32_MAX;
    }

    //根据源IP统计
    hash_table_t *ip_count = hash_table_new(int_hash, int_compare_func, NULL,
            NULL, NULL);
    uint64_t SESSION_COUNT = 25 * 10000 * 10000U;
    for (uint64_t i = 0; i < SESSION_COUNT; i++) {
        //构造session
        struct session session;
        bzero(&session, sizeof(struct session));
        session.src_ip = random_ip[rand() % IP_COUNT];
        //计数
        uint64_t* count = (uint64_t*) hash_table_lookup(ip_count,
                &session.src_ip);
        if (count == NULL) {
            uint32_t* ip = mem_cache_alloc(uint32_cache);
            count = mem_cache_alloc(uint64_cache);
            *count = 1;
            *ip = session.src_ip;
            hash_table_insert(ip_count, ip, count);
        } else {
            (*count)++;
        }
    }

    //求topn源IP
    topn_t *topn_ip = topn_new(TOPN, int64_compare_func, NULL, NULL, NULL);
    hash_table_foreach(ip_count, sort_ip, topn_ip);
    topn_foreach(topn_ip, print, NULL);

    topn_free(topn_ip);
    mem_cache_destroy(uint32_cache);

    LOG(LOG_LEVEL_TRACE, "程序运行耗时：\t%u", time(NULL) - start);
    */
}

bool sort_ip(void *key, void *value, void *user_data)
{
    topn_t *topn_ip = (topn_t *) user_data;
    topn_insert(topn_ip, value, key);
    return false;
}

bool print(void *key, void *value, void *user_data)
{
    char ip_str[16];
    uint32_t ip = *((uint32_t*) value);
    uint64_t count = *((uint64_t*) key);
    inet_ntop(AF_INET, (char*) &ip, ip_str, sizeof(ip_str));
    LOG(LOG_LEVEL_TRACE, "%s\t%llu", ip_str, count);
    return false;
}
