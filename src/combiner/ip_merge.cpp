//============================================================================
// Name        : ip_merge.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <set>
#include <vector>

#define MERGE_RATE 100u

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

struct ip_seg {
	uint32_t start;		//本地字节流
	uint32_t end;		//本地字节流
};

struct ip_seg get_ip_seg_from_mask_str(char *str) {
	struct ip_seg tmp_ip_seg = { 0, 0 };
	char *token = NULL;
	char *saveptr = NULL;
	char ipstr[16];
	int mask;
	uint32_t ip;

	token = strtok_r(str, "/", &saveptr);
	strncpy(ipstr, token, sizeof(ipstr) - 1);
	int ret = inet_pton(AF_INET, ipstr, &(ip));
	if (ret <= 0) {
		log("ip地址格式错误:%s", ipstr);
		return tmp_ip_seg;
	}
	token = strtok_r(NULL, "/", &saveptr);
	if (token == NULL) {
		mask = 32;
	} else {
		mask = atoi(token);
	}
	if ((mask > 32) || (mask < 0)) {
		log("掩码格式错误:%s", token);
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
		log("mask长度错误(%d)", mask);
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

void output(uint32_t start, uint32_t end) {
	int net_order_ip;
	if (start == end) {
		net_order_ip = htonl(start);
		cout << inet_ntoa(*(struct in_addr*) &net_order_ip) << "/32" <<endl;
//		cout << inet_ntoa(*(struct in_addr*) &net_order_ip) << "/32" << "\t"
//				<< start << "\t" << start << endl;
		return;
	}
	struct ip_seg ip_seg;
	ip_seg.start = start;
	ip_seg.end = end;
	uint32_t ip;
	int mask;
	get_ip_mask_from_ip_seg(ip_seg, &ip, &mask);
	net_order_ip = htonl(ip);
	cout << inet_ntoa(*(struct in_addr*) &net_order_ip) << "/" << mask  <<endl;
//	cout << inet_ntoa(*(struct in_addr*) &net_order_ip) << "/" << mask << "\t"
//			<< ip_seg.start << "\t" << ip_seg.end << endl;
}

void calc(uint32_t start, uint32_t end) {
	if (start == end) {
		output(start, end);
		return;
	}
	uint32_t start_ip = start;
	uint32_t end_ip = end;

	struct ip_seg pre_ip_seg;
	pre_ip_seg.start = start_ip;
	pre_ip_seg.end = start_ip;
	for (int mask = 31; mask > 0; mask--) {
		struct ip_seg ip_seg = get_ip_seg_from_ip_mask(start_ip, mask);
		if (ip_seg.start == start_ip && ip_seg.end == end_ip) {
			output(start_ip, end_ip);
			break;
		}
		//取重叠部分
		uint32_t overlapping =
				(end_ip - start_ip) > (ip_seg.end - start_ip) ?
						(ip_seg.end - start_ip + 1) : (end_ip - start_ip + 1);
		if (overlapping * 100 / (ip_seg.end - ip_seg.start + 1) < MERGE_RATE) {
			output(pre_ip_seg.start, pre_ip_seg.end);
			start_ip = pre_ip_seg.end + 1;
			pre_ip_seg.start = start_ip;
			pre_ip_seg.start = start_ip;
			mask = 32;
			if (start_ip == end_ip) {
				output(start_ip, end_ip);
				break;
			}
		}
		pre_ip_seg = ip_seg;
	}
}

int main(int argc, char **argv) {

	if (argc < 2) {
		cout << "usage: merge input_file1 input_file2 ..." << endl;
		return -1;
	}

	std::set<uint32_t> ip_set;
	for (int i = 1; i < argc; i++) {
		char *filename = argv[i];

		FILE *fp;
		int filenameLen = strlen(filename);

		if (filenameLen >= PATH_MAX) {
			log("%s -文件名称超过最大长度限制(%d)", filename, PATH_MAX);
			continue;
		}

		fp = fopen(filename, "r");
		if (fp == NULL) {
			log("打开文件失败(%s)", filename);
			continue;
		}

		char buffer[1024];
		while (fgets(buffer, sizeof(buffer), fp) != NULL) {
			char *p = buffer;
			for (; *p != '\0' && !isspace(*p); p++) {
			}
			*p = '\0';
			cout << buffer<<endl;
			struct ip_seg ip_seg = get_ip_seg_from_mask_str(buffer);
			for (uint32_t ip = ip_seg.start; ip <= ip_seg.end; ip++) {
				ip_set.insert(ip);
			}
		}

		fclose(fp);
	}
	if(ip_set.size() == 0){
		cout<<"未找到要处理的IP段"<<endl;
		return -1;
	}
	cout << "-----------------" << endl;
	std::vector<uint32_t> ip_vector;
	std::set<uint32_t>::iterator it;
	uint32_t pre_ip = *ip_set.begin();
	uint32_t start_ip;
	start_ip = *ip_set.begin();
	for (it = ip_set.begin(); it != ip_set.end(); ++it) {
		if (*it - pre_ip > 1) {
//			cout << "找到连续IP： start_ip:" << start_ip << "end_ip:" << pre_ip << endl;
			calc(start_ip, pre_ip);
			start_ip = *it;
		}
		pre_ip = *it;
	}

	calc(start_ip, pre_ip);

	return 0;
}
