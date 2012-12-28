/**
 *Description:      常用宏定义
 *
 *Notice:
 **/

#ifndef  _MACROS_H_
#define  _MACROS_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define mem_alloc(x) malloc(x)
#define mem_free(m) free(m)

#ifdef __cplusplus
}
#endif



#define MAX_FILE_NAME_LEN 256           //最大文件名称长度
#define MAX_PATH_NAME_LEN 1024          //最大文件路径长度
#define MAX_PATITION_NAME_LEN 64        //最大分区名称长度
#define MAX_LINE_LEN 1024               //最大行长度


#define RETURN_VAL_IF_FAIL(expr, val) {if(!(expr)) return val;}
#define RETURN_IF_FAIL(expr) {if(!(expr)) return;}

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define ABS(a)     (((a) < 0) ? -(a) : (a))

#endif /*  _MACROS_H_ */
