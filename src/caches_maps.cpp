#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <arpa/inet.h>
#include "log.h"
#include "topn.h"
#include "hash_table.h"
#include "list.h"
#include "memcache.h"
#include "ipmask.h"
#include "caches_maps.h"

static mem_cache_t *key_cache = NULL;
static mem_cache_t *value_cache = NULL;
static mem_cache_t *ipseg_cache = NULL;

static hash_table_t *wb_list_map = NULL;

static hash_table_t *tcp80_map = NULL;
static hash_table_t *tcp443_map = NULL;
static hash_table_t *tcp8080_map = NULL;
static hash_table_t *regular_map = NULL;

static list_t *a_list = NULL;

static topn_t *topn_ip = NULL;

const uint32_t MAX_LINE_COUNT = MAX_LOG_LINE;
const uint32_t MAX_A_LIST_LINES = 200;

hash_table_t* get_map_by_type(port_type_t type)
{
    switch (type) {
        case ALL_PORT:
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
    ipseg_cache = mem_cache_create(sizeof(ip_seg), MAX_A_LIST_LINES);

    assert(wb_list_map == NULL);
    wb_list_map =
        hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);

    assert(a_list == NULL);
    a_list = list_new(MAX_A_LIST_LINES, NULL);

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
        //if (!validate_wb(line, linelen))
        //    continue;

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

int ipseg_compare(const void *a, const void *b, void *user_data)
{
    assert(a != NULL);
    assert(b != NULL);
    uint32_t ip = *(uint32_t*)a;
    ip_seg *ipseg = (ip_seg*)b;

    if (ipseg->start <= ip && ip <= ipseg->end)
        return 0;
    else if (ipseg->start > ip)
        return 1;
    else if (ipseg->end < ip)
        return -1;

    return -1;
}

bool ip_in_a_list(uint32_t ip)
{
    return NULL != list_find(a_list, &ip, ipseg_compare, NULL);
}

bool build_a_list(const char *filename)
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
        // a.list has the same format with w.list and b.list
        //if (!validate_wb(line, linelen))
        //    continue;

        ip_seg ipseg = get_ip_seg_from_mask_str(line);
        if (ipseg.end == 0)
            continue;

        ip_seg* seg = (ip_seg*)mem_cache_alloc(ipseg_cache);
        *seg = ipseg;
        list_append(a_list, seg);
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

typedef struct
{
    uint64_t    data_volume_high;
    uint64_t    data_volume_low;
    uint64_t    dumped_volume;
    void        *data;
} user_data_t;

const uint32_t BYTES_PER_MB = 1024*1024;

bool sort_ip(void *key, void *value, void *user_data)
{
    user_data_t *udata = (user_data_t *)user_data;

    topn_t *topn = (topn_t *)udata->data;
    uint64_t &volume_high = udata->data_volume_high;
    uint64_t &volume_low = udata->data_volume_low;

    logrecord_t *record = (logrecord_t*)value;
    if (record->bytes >= BYTES_PER_MB) { // Greater than 1MB
        volume_high += record->bytes / BYTES_PER_MB;
        volume_low += record->bytes % BYTES_PER_MB;
    } else {
        volume_low += record->bytes;
    }

    if (volume_low >= BYTES_PER_MB) {
        volume_high += volume_low / BYTES_PER_MB;
        volume_low %= BYTES_PER_MB;
    }

    topn_insert(topn, value, key); // reverse insertion
    return true;
}

bool sort_list(port_type_t type, user_data_t *user_data)
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
    user_data->data = topn_ip;
    hash_table_foreach(table, sort_ip, user_data);
    //topn_foreach(topn_ip, print, NULL);
    return true;
}

bool dump_record(void *key, void *value, void *user_data)
{
    user_data_t *udata = (user_data_t *)user_data;
    FILE *f = (FILE*)udata->data;
    float data_volume_in_MB = (float)udata->data_volume_high;
    uint64_t &dumped_volume = udata->dumped_volume;

    uint32_t addr = htonl(*(uint32_t*)value);
    char ip[16] = {0};

    if (!inet_ntop(AF_INET, &addr, ip, sizeof(ip))) {
        return false;
    }

    logrecord_t *record = (logrecord_t*)key;
    if (record->bytes >= BYTES_PER_MB                 // Greater than 1MB
        && dumped_volume < data_volume_in_MB*0.8 ) { // Rate greater than 20%
        dumped_volume += record->bytes/BYTES_PER_MB;

        char content[128];
        sprintf(content
                , "%s\t%llu\t%u\n"
                , ip, record->bytes/BYTES_PER_MB, record->count);
        size_t len = strlen(content);
        return len != fwrite(content, sizeof(char), len, f);
    }

    return true;
}

bool dump_list(port_type_t type, const char *filename)
{
    user_data_t user_data;
    memset(&user_data, 0, sizeof(user_data_t));
    if (!sort_list(type, &user_data)) {
        LOG(LOG_LEVEL_ERROR, "Sort list %s failed", filename);
        return false;
    }

    FILE *file = fopen(filename, "w");
    if (!file) {
        LOG(LOG_LEVEL_ERROR, "Open %s for writing failed", filename);
        return false;
    }
    assert(file != NULL);

    user_data.data = file;
    topn_foreach(topn_ip, dump_record, &user_data);
    fclose(file);
    return true;
}
