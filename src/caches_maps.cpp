#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <arpa/inet.h>
#include "log.h"
#include "topn.h"
#include "hash_table.h"
#include "memcache.h"
#include "ipmask.h"
#include "caches_maps.h"

static mem_cache_t *key_cache = NULL;
static mem_cache_t *value_cache = NULL;

static hash_table_t *wb_list_map = NULL;

static hash_table_t *tcp80_map = NULL;
static hash_table_t *tcp443_map = NULL;
static hash_table_t *tcp8080_map = NULL;
static hash_table_t *regular_map = NULL;

static topn_t *topn_ip = NULL;

const uint32_t MAX_LINE_COUNT = MAX_LOG_LINE;

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

void init_caches(uint32_t count)
{
    if (count == 0) count = MAX_LINE_COUNT;
    assert(count > 0);

    assert(key_cache == NULL);
    assert(value_cache == NULL);

// TODO Create cache 4 times as MAX_LINE_COUNT, not enough/overflow aware
    key_cache = mem_cache_create(sizeof(uint32_t), 4*MAX_LOG_LINE);
    value_cache = mem_cache_create(sizeof(logrecord_t), 4*MAX_LOG_LINE);

    assert(wb_list_map == NULL);
    wb_list_map =
        hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);

    assert(regular_map == NULL);
    assert(tcp80_map == NULL);
    assert(tcp443_map == NULL);
    assert(tcp8080_map == NULL);
    regular_map =
        hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
    tcp80_map =
        hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
    tcp443_map =
        hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
    tcp8080_map =
        hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
}

bool
record2map(uint32_t addr, const logrecord_t *record, port_type_t type)
{
    hash_table_t *map = get_map_by_type(type);
    if (!map)
        return false;

    logrecord_t* original_record =
            (logrecord_t*) hash_table_lookup(map, &addr);
    if (original_record == NULL) {
        uint32_t* ip = (uint32_t* )mem_cache_alloc(key_cache);
        original_record = (logrecord_t* )mem_cache_alloc(value_cache);
        *original_record = *record;
        *ip = addr;
        hash_table_insert(map, ip, original_record);
    } else {
        original_record->count += record->count;
        original_record->bytes += record->bytes;
    }

    return true;
}

bool ip_in_wb_list(uint32_t ip)
{
    return hash_table_lookup_extended(wb_list_map, &ip, NULL, NULL);
}

bool validate_wb(const char *line, int len)
{
    const char *p = line;
    while (p - line < len) {
        if (!isalnum(*p) && (*p) != '.' && (*p) != '\0')
            return false;

        p++;
    }
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

    while ((linelen = getline(&line, &len, file)) != -1) {
        line[linelen-1] = '\0';
        if (!validate_wb(line, linelen))
            continue;

        ip_seg ipseg = get_ip_seg_from_mask_str(line);
        if (ipseg.end == 0)
            continue;

        for (uint32_t ip = ipseg.start; ip <= ipseg.end; ip++) {
            uint32_t* key = (uint32_t* )mem_cache_alloc(key_cache);
            *key = ip;
            hash_table_insert(wb_list_map, key, NULL);
        }
        //LOG(LOG_LEVEL_TRACE, "Read a line: %s, length: %zu", line, linelen);
    }

    if (line)
        free(line);

    fclose(file);
    return true;
}

int ip_compare(const void *key1, const void *key2, void *user_data)
{
    logrecord_t *record1 = (logrecord_t *)key1;
    logrecord_t *record2 = (logrecord_t *)key2;
    if (record1->bytes > record2->bytes)
        return 1;
    else if (record1->bytes < record2->bytes)
        return -1;
    else {
        return uint32_compare_func(&record1->count, &record2->count, NULL);
    }
}

bool sort_ip(void *key, void *value, void *user_data)
{
    topn_t *topn = (topn_t *)user_data;
    topn_insert(topn, value, key); // reverse insertion
    return true;
}

bool sort_list(port_type_t type)
{
    hash_table_t *table = get_map_by_type(type);
    if (table == NULL) {
        return false;
    }
    uint table_size = hash_table_size(table);

    if (topn_ip != NULL) {
        topn_free(topn_ip);
    }
    topn_ip = topn_new(table_size, ip_compare, table, NULL, NULL);
    hash_table_foreach(table, sort_ip, topn_ip);
    //topn_foreach(topn_ip, print, NULL);
    return true;
}

inline void
format(const char *ip, uint64_t bytes, uint32_t count, char *content)
{
    assert(content);

    const uint32_t BYTES_PER_MB = 1000000;
    const uint32_t BYTES_PER_KB = 1000;
    bool isMB = false;
    bool isKB = false;
    float bytes_after_convert = 0;
    if (bytes > BYTES_PER_MB*10) { // Greater than 10MB
        bytes_after_convert = (((float)bytes)/BYTES_PER_MB);
        isMB = true;
    } else if (bytes > BYTES_PER_KB*10) { // Greater than 1MB
        bytes_after_convert = (((float)bytes)/BYTES_PER_KB);
        isKB = true;
    }

    if (isMB) {
        sprintf(content
                , "%s\t%.3fM\t%u\n"
                , ip, bytes_after_convert, count);
    } else if (isKB) {
        sprintf(content
                , "%s\t%.3fK\t%u\n"
                , ip, bytes_after_convert, count);
    } else {
        sprintf(content, "%s\t%lu\t%u\n", ip, bytes, count);
    }
}

bool dump_record(void *key, void *value, void *user_data)
{
    uint32_t addr = htonl(*(uint32_t*)value);
    char ip[16] = {0};

    if (!inet_ntop(AF_INET, &addr, ip, sizeof(ip))) {
        return false;
    }
    logrecord_t *record = (logrecord_t*)key;
    char content[128];
    //sprintf(content, "%s\t%lu\t%u\n", ip, record->bytes, record->count);
    format(ip, record->bytes, record->count, content);
    size_t len = strlen(content);
    return len != fwrite(content, sizeof(char), len, (FILE *)user_data);
}

bool dump_list(port_type_t type, const char *filename)
{
    if (!sort_list(type)) {
        LOG(LOG_LEVEL_ERROR, "Sort list %s failed", filename);
        return false;
    }

    FILE *file = fopen(filename, "w");
    if (!file) {
        LOG(LOG_LEVEL_ERROR, "Open %s for writing failed", filename);
        return false;
    }
    assert(file != NULL);

    topn_foreach(topn_ip, dump_record, file);
    fclose(file);
    return true;
}
