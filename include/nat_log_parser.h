#ifndef __NAT_LOG_PARSER_H__
#define __NAT_LOG_PARSER_H__

#include "types.h"

bool parse_log(const char *log
                , uint32_t *addr, logrecord_t *record, port_type_t *type);

#endif // __NAT_LOG_PARSER_H__
