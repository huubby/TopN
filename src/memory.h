/*
 * memory.h
 *
 *  Created on: 2012-6-7
 *      Author: Administrator
 */

#ifndef CLIB_MEMORY_H_
#define CLIB_MEMORY_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define mem_alloc(x) malloc(x)
#define mem_free(m) free(m)

#ifdef __cplusplus
}
#endif

#endif /* CLIB_MEMORY_H_ */
