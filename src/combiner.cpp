#include <iostream>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <set>
#include <vector>
#include "ipmask.h"
using namespace std;

#define MERGE_RATE 100u

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
		cout << "usage: combiner input_file1 input_file2 ..." << endl;
		return -1;
	}

	std::set<uint32_t> ip_set;
	for (int i = 1; i < argc; i++) {
		char *filename = argv[i];

		FILE *fp;
		int filenameLen = strlen(filename);

		if (filenameLen >= PATH_MAX) {
			log("%s - name too long(%d)", filename, PATH_MAX);
			continue;
		}

		fp = fopen(filename, "r");
		if (fp == NULL) {
			log("Open file(%s) failed", filename);
			continue;
		}

		char buffer[1024];
		while (fgets(buffer, sizeof(buffer), fp) != NULL) {
			char *p = buffer;
			for (; *p != '\0' && !isspace(*p); p++) {
			}
			*p = '\0';
			//cout << buffer<<endl;
			struct ip_seg ip_seg = get_ip_seg_from_mask_str(buffer);
			for (uint32_t ip = ip_seg.start; ip <= ip_seg.end; ip++) {
				ip_set.insert(ip);
			}
		}

		fclose(fp);
	}
	if(ip_set.size() == 0){
		cout<<"No IP found in input files"<<endl;
		return -1;
	}
	//cout << "-----------------" << endl;
	std::set<uint32_t>::iterator it;
	uint32_t pre_ip = *ip_set.begin();
	uint32_t start_ip;
	start_ip = *ip_set.begin();
	for (it = ip_set.begin(); it != ip_set.end(); ++it) {
		if (*it - pre_ip > 1) {
//			cout << "Continuous IP segment found: start_ip:" << start_ip << "end_ip:" << pre_ip << endl;
			calc(start_ip, pre_ip);
			start_ip = *it;
		}
		pre_ip = *it;
	}

	calc(start_ip, pre_ip);

	return 0;
}
