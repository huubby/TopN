#ifndef __NAT_LOG_PARSER_H__
#define __NAT_LOG_PARSER_H__

#include "types.h"

#pragma pack(4)
typedef struct {
    uint32_t count;
    uint64_t bytes;
} logrecord_t;
#pragma pack()

typedef enum {
    REGULAR_PORT = 0
    , TCP_PORT_80
    , TCP_PORT_443
    , TCP_PORT_8080
} port_type_t;

bool parse_log(const char *log
                , uint32_t *addr, logrecord_t *record, port_type_t *type);

#endif // __NAT_LOG_PARSER_H__
