#ifndef __C_LIST_PARSER_H__
#define __C_LIST_PARSER_H__

#include "types.h"

const int max_ip_len = 16; // xxx.xxx.xxx.xxx

void log(const char* fmt, ...);

struct ip_seg {
	uint32_t start;		//本地字节流
	uint32_t end;		//本地字节流
};

struct ip_seg get_ip_seg_from_mask_str(char *str);

struct ip_seg get_ip_seg_from_ip_mask(uint32_t ip, int mask);

void get_ip_mask_from_ip_seg(struct ip_seg ip_seg, uint32_t *ip, int *mask);

#endif // __C_LIST_PARSER_H__
