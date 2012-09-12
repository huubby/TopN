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

//-------------------- Command Line Arguments ----------------------
static char *log_file = NULL;
static char *w_list = NULL;
static char *b_list = NULL;

static char *c_list = NULL;
static char *c80_list = NULL;
static char *c443_list = NULL;
static char *c8080_list = NULL;
CommandLineOptions_t options[] = {
    { "f", "The name of NAT log file waiting for processing"
        , "log file name", OPTION_REQUIRED, ARG_STR, &log_file },
    { "w", "The name of w.list file"
        , "w.list name", OPTION_REQUIRED, ARG_STR, &w_list },
    { "b", "The name of b.list file"
        , "b.list name", OPTION_REQUIRED, ARG_STR, &b_list },
    { "c", "The name of c.list file"
        , "c.list name", OPTION_OPTIONAL, ARG_STR, &c_list },
    { "c80", "The name of c.list.80 file"
        , "c.list.80 name", OPTION_OPTIONAL, ARG_STR, &c80_list },
    { "c443", "The name of c.list.443 file"
        , "c.list.443 name", OPTION_OPTIONAL, ARG_STR, &c443_list },
    { "c8080", "The name of c.list.8080 file"
        , "c.list.8080 name", OPTION_OPTIONAL, ARG_STR, &c8080_list },
    {NULL, NULL, NULL, 0, 0, NULL}
};
static char description[] = {"Linux NAT log file analyzer\nVersion 1.0\n"};

//--------------------- Global Datas -------------------------------
#pragma pack(4)
typedef struct {
    uint32_t count;
    uint64_t bytes;
} logrecord_t;
#pragma pack()
#define MAX_LOG_LINE 100000
const uint32_t MAX_LINE_COUNT = MAX_LOG_LINE;
static mem_cache_t *key_cache = NULL;
static mem_cache_t *value_cache = NULL;

static hash_table_t *wb_list_map = NULL;

static hash_table_t *tcp80_map = NULL;
static hash_table_t *tcp443_map = NULL;
static hash_table_t *tcp8080_map = NULL;
static hash_table_t *regular_map = NULL;

typedef enum {
    REGULAR_PORT = 0
    , TCP_PORT_80
    , TCP_PORT_443
    , TCP_PORT_8080
} port_type_t;

bool build_wb_list(const char *filename);
bool build_maps_from_c_list(const char *filename, port_type_t type);
bool process(const char *log, uint32_t len);

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
        || strlen(w_list) == 0
        || strlen(b_list) == 0) {
        LOG(LOG_LEVEL_ERROR, "Empty file name");
        exit(-1);
    }
    if (strlen(c_list) == 0
        || strlen(c80_list) == 0
        || strlen(c443_list) == 0
        || strlen(c8080_list) == 0 ) {
        LOG(LOG_LEVEL_WARNING, "Some of c.list missed");
    }

    if (!build_wb_list(w_list)
        || !build_wb_list(b_list)
        || !build_maps_from_c_list(c_list, REGULAR_PORT)
        || !build_maps_from_c_list(c80_list, TCP_PORT_80)
        || !build_maps_from_c_list(c443_list, TCP_PORT_443)
        || !build_maps_from_c_list(c8080_list, TCP_PORT_8080)) {
        LOG(LOG_LEVEL_ERROR, "Build maps failed");
        exit(-1);
    }

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

void init_caches(uint32_t count)
{
    if (count == 0) count = MAX_LINE_COUNT;
    assert(count > 0);

    assert(key_cache == NULL);
    assert(value_cache == NULL);

// TODO Create cache 4 times as MAX_LINE_COUNT, not enough/overflow aware
    key_cache = mem_cache_create(sizeof(uint32_t), 4*MAX_LOG_LINE);
    value_cache = mem_cache_create(sizeof(logrecord_t), 4*MAX_LOG_LINE);

    assert(regular_map == NULL);
    assert(tcp80_map == NULL);
    assert(tcp443_map == NULL);
    assert(tcp8080_map == NULL);
    regular_map = hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
    tcp80_map = hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
    tcp443_map = hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
    tcp8080_map = hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
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
        uint32_t* ip = (uint32_t* )mem_cache_alloc(key_cache);
        original_record = (logrecord_t* )mem_cache_alloc(value_cache);
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

bool build_wb_list(const char *filename)
{
    size_t      len;
    ssize_t     linelen = 0;
    char        *line = NULL;

    FILE *file = fopen(filename, "r");
    if (!file) {
        LOG(LOG_LEVEL_ERROR, "Open %s failed", filename);
        return false;
    }
    assert(file != NULL);

    linelen = getline(&line, &len, file);
    while (linelen != -1) {
        //TODO parse every line, save to wb_list_map...

        //LOG(LOG_LEVEL_TRACE, "Read a line: %s, length: %zu", line, linelen);
    }

    if (line)
        free(line);

    fclose(file);
    return true;
}

bool build_maps_from_c_list(const char *filename, port_type_t type)
{
    return true;
}
