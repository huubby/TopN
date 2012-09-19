#ifndef __TOCH_H__
#define __TOCH_H__

#include "types.h"

bool is_addr_reachable(uint32_t addr, port_type_t type, uint32_t timeout_in_ms);

#endif // __TOCH_H__
