#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include "log.h"
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

    assert(regular_map == NULL);
    assert(tcp80_map == NULL);
    assert(tcp443_map == NULL);
    assert(tcp8080_map == NULL);
    regular_map = hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
    tcp80_map = hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
    tcp443_map = hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
    tcp8080_map = hash_table_new(int32_hash, int32_compare_func, NULL, NULL, NULL);
}

bool
record2map(uint32_t addr, const logrecord_t *record, port_type_t type)
{
    hash_table_t *map = get_map_by_type(type);
    if (!map)
        return false;

    logrecord_t* original_record = (logrecord_t*) hash_table_lookup(map, &addr);
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

bool validate_wb(const char *line, int len)
{
    const char *p = line;
    while (p - line < len) {
        if (!isalnum(*p) && (*p) != '.')
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
