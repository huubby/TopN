#ifndef __CACHES_MAPS_H__
#define __CACHES_MAPS_H__

#include "types.h"

bool ip_in_wb_list(uint32_t ip);
bool build_wb_list(const char *filename);
void init_caches(uint32_t count);

bool
record2map(uint32_t addr, const logrecord_t *record, port_type_t type);

bool sort_list(port_type_t type);
bool dump_list(port_type_t type, const char *filename);

#endif // __CACHES_MAPS_H__
