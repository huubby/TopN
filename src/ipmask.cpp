#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <string.h>
#include "ipmask.h"

using namespace std;

void log(const char* fmt, ...) {
	va_list vap;
	char log_buf[1024];
	va_start(vap, fmt);
	vsnprintf(log_buf, sizeof(log_buf), fmt, vap);
	log_buf[sizeof(log_buf) - 1] = '\0';
	va_end(vap);
	cout << log_buf << endl;

}

struct ip_seg get_ip_seg_from_mask_str(char *str) {
	struct ip_seg tmp_ip_seg = { 0, 0 };
	char *token = NULL;
	char *saveptr = NULL;
	char ipstr[16];
	int mask;
	uint32_t ip;

	token = strtok_r(str, "/", &saveptr);
	strncpy(ipstr, token, sizeof(ipstr) - 1);
    // here, set last char to '\0', in case of xxx.xxx.xxx.xxx?
	int ret = inet_pton(AF_INET, ipstr, &(ip));
	if (ret <= 0) {
		log("Invalid IP format: %s", ipstr);
		return tmp_ip_seg;
	}
	token = strtok_r(NULL, "/", &saveptr);
	if (token == NULL) {
		mask = 32;
	} else {
		mask = atoi(token);
	}
	if ((mask > 32) || (mask < 0)) {
		log("Invalid mask format: %s", token);
		return tmp_ip_seg;
	}
	ip = ntohl(ip);
	if (mask == 32) {
		tmp_ip_seg.start = ip;
		tmp_ip_seg.end = ip;
	} else if (mask == 0) {
		tmp_ip_seg.start = 0x00000000;
		tmp_ip_seg.end = 0xFFFFFFFF;
	} else {
		tmp_ip_seg.start = ip & (0xFFFFFFFF << (32 - mask));
		tmp_ip_seg.end = ip | (0xFFFFFFFF >> mask);
	}

	return tmp_ip_seg;
}

struct ip_seg get_ip_seg_from_ip_mask(uint32_t ip, int mask) {
	struct ip_seg tmp_ip_seg = { 0, 0 };

	if (mask == 0) {
		tmp_ip_seg.start = 0x00000000;
		tmp_ip_seg.end = 0xFFFFFFFF;
		return tmp_ip_seg;
	}

	if (mask == 32) {
		tmp_ip_seg.start = ip;
		tmp_ip_seg.end = ip;
		return tmp_ip_seg;
	}

	if ((mask > 32) || (mask < 0)) {
		log("Invalid mask(%d)", mask);
		return tmp_ip_seg;
	}

	tmp_ip_seg.start = ip & (0xFFFFFFFF << (32 - mask));
	tmp_ip_seg.end = ip | (0xFFFFFFFF >> mask);

	return tmp_ip_seg;
}

void get_ip_mask_from_ip_seg(struct ip_seg ip_seg, uint32_t *ip, int *mask) {
	*ip = ip_seg.start & ip_seg.end;
	uint32_t tmp_ip_mask = ip_seg.start ^ ip_seg.end;
	*mask = 0;
	int begin = 0, end = 32;
	int mid;
	while ((end - begin) > 1) {
		mid = (begin + end) / 2;
		if ((tmp_ip_mask >> mid) == 0x00000000) {
			end = mid;
		} else {
			begin = mid;
		}
	}
	if ((tmp_ip_mask >> begin) == 0x00000000) {
		*mask = 32 - begin;
	} else {
		*mask = 32 - end;
	}
}
