/**
 *
 *Description:      常见的字符串处理函数
 *
 *Notice:           多线程安全
**/
#ifndef CLIB_STRING_H_
#define CLIB_STRING_H_

#include "types.h"
//#include <clib/array.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

/*
 *@brief        去掉字符串前面和后面的空格以及回车
 *@param in     str     待处理的字符串
 *@param out    strlen  待处理的字符串长度
 *@return       处理结果字符串
 *
 *@notice       该函数会修改传入的str的值，并且返回str的一个字串
 *              例如：char str[] = " hello  ";
 *              char *result = strtrim(str, strlen(str));
 *              结果：      str 变成 " hello"
 *                      result 指向地址 &(str[1])，值为"hello"
 */
char * strtrim(char *str, int strlen);


extern void* memdup(const void *mem, uint size);

/*
 *@brief        分割字符串
 *@param in     str     待处理的字符串
 *@return       得到的字符串数组
 *
 *@notice       返回的array需要调用者自己释放
 */
//str_array_t* strsplit(const char *str, const char *sep);

/*
 *@brief        将字符串转换成小写
 *@param in     str     待转换的字符串
 *@return
 *
 *@notice
 */
void str_tolower(char *str);


#ifdef __cplusplus
}
#endif

#endif /* CLIB_STRING_H_ */
