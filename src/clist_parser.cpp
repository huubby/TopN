#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "hash_table.h"
#include "log.h"
#include "ipmask.h"
#include "caches_maps.h"
#include "clist_parser.h"

const uint64_t INVALID_BYTES =  (uint64_t)-1;
const uint32_t BYTES_PER_KB = 1024;
const uint32_t BYTES_PER_MB = 1024*BYTES_PER_KB;
uint64_t convert_bytes(char *traffic_str)
{
    if (!traffic_str)
        return INVALID_BYTES;
    size_t len = strlen(traffic_str);
    if (len <= 0)
        return INVALID_BYTES;

    uint64_t bytes = 0;
    float bytes_float_num = 0;
    errno = 0;
    bytes = strtoull(traffic_str, NULL, 10);

    return errno==0 ? bytes : INVALID_BYTES;
}

bool
parse_c_list(const char *line, int len, uint32_t *addr, logrecord_t *record)
{
    //TODO
    // 1. parse record from middle result line
    // 2. record saved to map
    const char *tab = "\t";
    const char *tab_loc = strstr(line, tab);
    if (tab_loc == NULL || tab_loc-line > max_ip_len)
        return false;   // No tab, or ip len is incorrect
    char ip[max_ip_len];
    strncpy(ip, line, tab_loc-line);
    ip[tab_loc-line] = '\0';
    if (inet_pton(AF_INET, ip, addr) <= 0) {
        return false;
    }
    *addr = ntohl(*addr);

    while (*(++tab_loc) == '\t');
    if (tab_loc - line > len)
        return false;
    const char *traffic_loc = tab_loc;
    tab_loc = strstr(traffic_loc, tab);
    if (tab_loc == NULL || tab_loc-line > len)
        return false;   // No tab
    char bytes[32];
    strncpy(bytes, traffic_loc, tab_loc-traffic_loc);
    bytes[tab_loc-traffic_loc] = '\0';
    if ((record->bytes = convert_bytes(bytes)) == INVALID_BYTES)
        return false;  // Invalid number
    // The c.list bytes is in MB
    record->bytes *= BYTES_PER_MB;

    while (*(++tab_loc) == '\t');
    if (tab_loc - line > len)
        return false;
    errno = 0;
    record->count = strtoull(tab_loc, NULL, 10);
    if (errno != 0)
        return false;  // Invalid number

    return true;
}

bool build_map_from_c_list(const char *file_name, port_type_t type)
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

    while ( (linelen = getline(&line, &len, file)) != -1
            && process_result) {
        line[linelen-1]='\0';
        uint32_t addr;
        logrecord_t record;
        process_result = parse_c_list(line, len, &addr, &record);
        if (process_result)
            process_result = record2map(addr, &record, type);
        else
            LOG(LOG_LEVEL_WARNING, "A line of %s parsing failed", file_name);

        //LOG(LOG_LEVEL_TRACE, "Read a line: %s, length: %zu", line, linelen);
    }

    if (line)
        free(line);

    fclose(file);

    return true;
}
