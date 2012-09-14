#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include "hash_table.h"
#include "log.h"
#include "ipmask.h"
#include "caches_maps.h"
#include "clist_parser.h"

bool
parse_c_list(const char *line, int len, uint32_t *addr, logrecord_t *record)
{
    //TODO
    // 1. parse record from middle result line
    // 2. record saved to map
    const char *tab = "\t";
    char *tab_loc = strstr(line, tab);
    if (tab_loc == NULL || tab_loc-line > max_ip_len)
        return false;   // No tab, or ip len is incorrect
    char ip[max_ip_len];
    strncpy(ip, line, tab_loc-line);
    ip[tab_loc-line] = '\0';
    if (inet_pton(AF_INET, ip, addr) <= 0) {
        return false;
    }

    while (*(++tab_loc) == '\t');
    if (tab_loc - line > len)
        return false;
    char *traffic_loc = tab_loc;
    tab_loc = strstr(traffic_loc, tab);
    if (tab_loc == NULL || tab_loc-line > len)
        return false;   // No tab
    char bytes[32];
    strncpy(bytes, traffic_loc, tab_loc-traffic_loc);
    bytes[tab_loc-traffic_loc] = '\0';
    char *endptr = NULL;
    record->bytes = strtoull(bytes, &endptr, 10);
    if (endptr != NULL)
        return false;  // Invalid number

    while (*(++tab_loc) == '\t');
    if (tab_loc - line > len)
        return false;
    record->count = strtoull(tab_loc, &endptr, 10);
    if (endptr != NULL)
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

    linelen = getline(&line, &len, file);
    while (linelen != -1 && process_result) {
        line[linelen-1]='\0';
        uint32_t addr;
        logrecord_t record;
        process_result = parse_c_list(line, len, &addr, &record);
        if (process_result)
            process_result = record2map(addr, &record, type);

        //LOG(LOG_LEVEL_TRACE, "Read a line: %s, length: %zu", line, linelen);
    }

    if (line)
        free(line);

    fclose(file);

    return true;
}
