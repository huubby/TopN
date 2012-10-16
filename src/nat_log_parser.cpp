#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "ipmask.h"
#include "nat_log_parser.h"

bool identify_protocol_ip_port(const char *log
        , uint32_t *addr, port_type_t *type, char **end)
{
    const char *tcp_protocol = " 6 ";
    const char *udp_protocol = " 17 ";
    const char *dst_ip_name = "dst=";
    const size_t dst_ip_name_len = strlen(dst_ip_name);
    const char *port_name = "dport=";
    const size_t port_name_len = strlen(port_name);
    const char *port_80 = "80";
    const size_t port80_len = strlen(port_80);
    const char *port_443 = "443";
    const size_t port443_len = strlen(port_443);
    const char *port_8080 = "8080";
    const size_t port8080_len = strlen(port_8080);
    const char *blank = " ";

    *addr = 0;
    *end = (char *)log;

    int len = (int)strlen(log);
    const char *blank_loc;
    const char *protocol_loc;
    if (!(protocol_loc = strstr(log, tcp_protocol))
        && !(protocol_loc = strstr(log, udp_protocol))) {
        return false;
    }

    const char *dst_ip_loc = strstr(protocol_loc, dst_ip_name);
    if (dst_ip_loc == NULL || (dst_ip_loc+dst_ip_name_len-log) > len)
        return false;

    dst_ip_loc += dst_ip_name_len;
    blank_loc = strstr(dst_ip_loc, blank);
    if (blank_loc == NULL || blank_loc-dst_ip_loc > max_ip_len)
        return false;   // No blank following "dst=", or ip len is incorrect
    char dst_ip[max_ip_len];
    strncpy(dst_ip, dst_ip_loc, blank_loc-dst_ip_loc);
    dst_ip[blank_loc-dst_ip_loc] = '\0';
    if (inet_pton(AF_INET, dst_ip, addr) <= 0) {
        return false;
    }
    *addr = ntohl(*addr);

    const char *port_loc = strstr(blank_loc, port_name);
    if (port_loc == NULL || (port_loc+port_name_len-log) > len)
        return false;

    port_loc += port_name_len;
    // No blank following "dport="
    blank_loc = strstr(port_loc, " ");
    if (blank_loc == NULL)
        return false;

    char port[16] = {0}; // No port number longer than 16 characters
    strncpy(port, port_loc, blank_loc-port_loc);
    if (!strncmp(port, port_80, port80_len)) {
        *type = TCP_PORT_80;
    } else if (!strncmp(port, port_443, port443_len)) {
        *type = TCP_PORT_443;
    } else if (!strncmp(port, port_8080, port8080_len)) {
        *type = TCP_PORT_8080;
    } else {
        *type = REGULAR_PORT;
    }

    *end = (char*)blank_loc;

    return true;
}

bool identify_bytes(const char *log, uint64_t *bytes)
{
    const char *bytes_name = "bytes=";
    const size_t bytes_name_len = strlen(bytes_name);
    int len = (int)strlen(log);
    *bytes = 0;

    static char *bytes_num = NULL;
    static int bytes_num_len = 0;

    if (len <= (int)bytes_name_len)
        return false;
    char *bytes_loc = (char *)log;
    while ((bytes_loc = strstr(bytes_loc, bytes_name)) != NULL
            && (bytes_loc+bytes_name_len-log) <= len) {
        bytes_loc += bytes_name_len;
        char *blank_loc = strstr(bytes_loc, " ");
        if (blank_loc == NULL) {
            *bytes = 0;
            break;   // No blank following "bytes="
        }

        int cur_num_len = blank_loc - bytes_loc;
        if (bytes_num == NULL || bytes_num_len < cur_num_len+1) {
            if (bytes_num != NULL)
                free(bytes_num);

            bytes_num_len = cur_num_len + 1;
            bytes_num = (char*)malloc(bytes_num_len);
        }
        strncpy(bytes_num, bytes_loc, cur_num_len);
        bytes_num[cur_num_len+1] = '\0';

        *bytes += strtoull(bytes_num, NULL, 10);
    }

    return *bytes != 0;
}

bool parse_log(const char *log
                , uint32_t *addr, logrecord_t *record, port_type_t *type)
{
    if (log == NULL) return false;

    char *position;
    if (!identify_protocol_ip_port(log, addr, type, &position)) {
        return false;
    }
    if (!identify_bytes(position, &record->bytes)) {
        return false;
    }
    record->count = 1;

    return true;
}
