#ifndef __CACHES_MAPS_H__
#define __CACHES_MAPS_H__

#include "types.h"

bool build_wb_list(const char *filename);
void init_caches(uint32_t count);

bool
record2map(uint32_t addr, const logrecord_t *record, port_type_t type);

#endif // __CACHES_MAPS_H__
