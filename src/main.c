#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
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
static char *c_list = NULL;
static char *result_file = NULL;
CommandLineOptions_t options[] = {
    { "f", "The name of NAT log file waiting for processing"
        , "log file name", OPTION_REQUIRED, ARG_STR, &log_file },
    { "c", "The name of c.list file"
        , "c.list name", OPTION_REQUIRED, ARG_STR, &c_list },
    { "o", "The output file name"
        , "output file name", OPTION_REQUIRED, ARG_STR, &result_file },
    {NULL, NULL, NULL, 0, 0, NULL}
};
static char description[] = {"Linux NAT log file analyzer\nVersion 1.0\n"};

static mem_cache_t *key_cache = NULL;
static mem_cache_t *value_cache = NULL;

static hash_table_t *tcp80_map = NULL;
static hash_table_t *tcp443_map = NULL;
static hash_table_t *tcp8080_map = NULL;
static hash_table_t *regular_map = NULL;

static hash_table_t *white_list = NULL;
static hash_table_t *black_list = NULL;

bool build_maps_from_ct_list();
bool process(const char *log, uint32_t len);

const uint32_t MAX_LINE_COUNT = 1500000;

int main(int argc, char*argv[])
{
    FILE        *input_file;
    size_t      len;
    ssize_t     linelen = 0;
    uint32_t    line_count = 0;
    char        *line = NULL;
    bool        process_result = true;
    if (init_application(argc, argv, options, description) < 0) {
        exit(-1);
    }
    if (strlen(log_file) == 0
        || strlen(result_file) == 0
        || strlen(c_list) == 0
        ) {
        LOG(LOG_LEVEL_ERROR, "Empty file name");
        exit(-1);
    }

    //TODO build c.list/t.list
    // build_maps_from_ct_list();

    //TODO build white/black list
    // build_wb_list();

    input_file = fopen(log_file, "r");
    if (!input_file) {
        LOG(LOG_LEVEL_ERROR, "Open log file failed");
        exit(-1);
    }
    assert(input_file != NULL);

    linelen = getline(&line, &len, input_file);
    while (linelen != -1
            && line_count < MAX_LINE_COUNT
            && process_result) {
        //LOG(LOG_LEVEL_TRACE, "Read a line: %s, length: %zu", line, linelen);
        process_result = process(line, len);
        line_count++;
    }

    if (line)
        free(line);
    fclose(input_file);

    return 0;
}

//----------------------- Functions -----------------------
typedef struct {
    uint32_t count;
    uint64_t bytes;
} logrecord_t;

typedef enum {
    REGULAR_PORT = 0
    , TCP_PORT_80
    , TCP_PORT_443
    , TCP_PORT_8080
} port_type_t;


void init_caches(uint32_t count)
{
    if (count == 0) count = MAX_LINE_COUNT;

    assert(count > 0);

    // TODO Init 80/443/8080 and all others 4 memcaches and hash_maps...
    //key_cache = mem_cache_create(sizeof(uint32_t), MAX_LINE_COUNT);
    //value_cache = mem_cache_create(sizeof(logrecord_t), MAX_LINE_COUNT);

    tcp80_map = hash_table_new(int_hash, int_compare_func, NULL, NULL, NULL);
    tcp443_map = hash_table_new(int_hash, int_compare_func, NULL, NULL, NULL);
    tcp8080_map = hash_table_new(int_hash, int_compare_func, NULL, NULL, NULL);
    regular_map = hash_table_new(int_hash, int_compare_func, NULL, NULL, NULL);
}

hash_table_t* get_map_by_type(port_type_t type)
{
    switch (type) {
        case REGULAR_PORT:
            return regular_map;
        case TCP_PORT_80:
            return tcp80_map;
        case TCP_PORT_443:
            return tcp443_map;
        case TCP_PORT_8080:
            return tcp8080_map;
        default:
            assert(false);
    }

    return NULL;
}

bool
record2map(const uint32_t *addr, const logrecord_t *record, port_type_t type)
{
    hash_table_t *map = get_map_by_type(type);
    //TODO Set the real number when reading from c.list/c.list.80/c.list.443/c.list.8080
    // Current just set the initilized size to 0, using MAX_LINE_COUNT

    logrecord_t* original_record = (logrecord_t*) hash_table_lookup(map, addr);
    if (original_record == NULL) {
        uint32_t* ip = mem_cache_alloc(key_cache);
        original_record = mem_cache_alloc(value_cache);
        *original_record = *record;
        *ip = *addr;
        hash_table_insert(map, ip, original_record);
    } else {
        original_record->count += record->count;
        original_record->bytes += record->bytes;
    }

    return true;
}

bool
parse_c_list(const char *line, size_t len, port_type_t type)
{
    //TODO
    // 1. parse record from middle result line
    // 2. record saved to map
    return true;
}

bool build_maps_from_ct_list(const char *file_name, port_type_t type)
{
    size_t      len;
    ssize_t     linelen = 0;
    char        *line = NULL;
    bool        process_result = true;

    FILE *file = fopen(file_name, "r");
    if (!file) {
        LOG(LOG_LEVEL_ERROR, "Open %s failed", file_name);
        return false;
    }
    assert(file != NULL);

    linelen = getline(&line, &len, file);
    while (linelen != -1 && process_result) {
        process_result = parse_c_list(line, len, type);

        //LOG(LOG_LEVEL_TRACE, "Read a line: %s, length: %zu", line, linelen);
    }

    if (line)
        free(line);

    fclose(file);

    return true;
}

bool parse_log(const char *log
                , uint32_t *addr, logrecord_t *record, port_type_t *type)
{
    // TODO parse
    //
    return true;
}

bool process(const char *log, uint32_t len)
{
    //time_t start = time(NULL);
    //srand(start);

    uint32_t addr;
    logrecord_t record;
    port_type_t type;

    //TODO Maybe add a counter here to record all ignored lines
    if (!parse_log(log, &addr, &record, &type))
        return true;    // Ignore this line, continue to next

    if (key_cache == NULL) {
        init_caches(0);
    }
    record2map(&addr, &record, type);

    return true;
}
