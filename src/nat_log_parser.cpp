#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "ipmask.h"
#include "nat_log_parser.h"
#include "caches_maps.h"

bool identify_ip(const char *log, const char *addr_name
        , uint32_t *addr, char **end)
{
    const size_t ip_name_len = strlen(addr_name);
    const char *blank = " ";

    *addr = 0;
    *end = (char *)log;

    int len = (int)strlen(log);
    const char *blank_loc;
    const char *ip_loc = strstr(log, addr_name);
    if (ip_loc == NULL || (ip_loc + ip_name_len - log) > len)
        return false;

    ip_loc += ip_name_len;
    blank_loc = strstr(ip_loc, blank);
    if (blank_loc == NULL || blank_loc - ip_loc > max_ip_len)
        return false;   // No blank following addr_name, or ip len is incorrect

    char ip[max_ip_len];
    strncpy(ip, ip_loc, blank_loc - ip_loc);
    ip[blank_loc-ip_loc] = '\0';
    if (inet_pton(AF_INET, ip, addr) <= 0) {
        return false;
    }
    *addr = ntohl(*addr);

    *end = (char*)blank_loc;

    return true;
}

bool identify_port(const char *log, const char *port_name
        , port_type_t *type, char **end)
{
    const size_t port_name_len = strlen(port_name);
    const char *port_80 = "80";
    const size_t port80_len = strlen(port_80);
    const char *port_443 = "443";
    const size_t port443_len = strlen(port_443);
    const char *port_8080 = "8080";
    const size_t port8080_len = strlen(port_8080);

    *end = (char *)log;

    int len = (int)strlen(log);
    const char *blank_loc;
    const char *port_loc = strstr(log, port_name);
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
        *type = ALL_PORT;
    }

    *end = (char*)blank_loc;

    return true;
}

bool identify_protocol_ip_port(const char *log
        , uint32_t *addr, port_type_t *type, char **end)
{
    const char *tcp_protocol = " 6 ";
    const char *udp_protocol = " 17 ";
    const char *src_ip_name = "src=";
    const char *src_port_name = "sport=";
    const char *dst_ip_name = "dst=";
    const char *dst_port_name = "dport=";

    *addr = 0;
    *end = (char *)log;

    const char *protocol_loc;
    if (!(protocol_loc = strstr(log, tcp_protocol))
        && !(protocol_loc = strstr(log, udp_protocol))) {
        return false;
    }

    char *position = NULL;
    uint32_t src_ip;
    if (!identify_ip(protocol_loc, src_ip_name, &src_ip, &position))
        return false;
    uint32_t dst_ip;
    if (!identify_ip(position, dst_ip_name, &dst_ip, &position))
        return false;

    port_type_t src_port;
    if (!identify_port(position, src_port_name, &src_port, &position))
        return false;
    port_type_t dst_port;
    if (!identify_port(position, dst_port_name, &dst_port, &position))
        return false;

    bool is_src_in_a_list = ip_in_a_list(src_ip);
    bool is_dst_in_a_list = ip_in_a_list(dst_ip);

    if (is_src_in_a_list == is_dst_in_a_list)
        return false;
    else if (is_src_in_a_list) {
        *addr = dst_ip;
        *type = dst_port;
    } else {
        *addr = src_ip;
        *type = src_port;
    }

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
