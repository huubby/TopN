#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <set>
#include <vector>
#include "ipmask.h"
#include "seg_tree.h"
#include "list.h"

using namespace std;

#define MERGE_RATE 100u

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

void output(uint32_t start, uint32_t end)
{
    int net_order_ip;
    if (start == end) {
        net_order_ip = htonl(start);
        cout << inet_ntoa(*(struct in_addr*) &net_order_ip) << "/32" << endl;
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
    cout << inet_ntoa(*(struct in_addr*) &net_order_ip) << "/" << mask << endl;
//	cout << inet_ntoa(*(struct in_addr*) &net_order_ip) << "/" << mask << "\t"
//			<< ntohl(ip_seg.start) << "\t" << ntohl(ip_seg.end) << endl;
}

void calc_local(uint start, uint end)
{
    if (start == end) {
        output(start, end);
        return;
    }
    uint32_t start_ip = start;
    uint32_t end_ip = end;

    struct ip_seg pre_ip_seg;
    pre_ip_seg.start = start_ip;
    pre_ip_seg.end = start_ip;
    for (int mask = 31; mask >= 0; mask--) {
        struct ip_seg ip_seg = get_ip_seg_from_ip_mask(start_ip, mask);
        if (ip_seg.start == start_ip && ip_seg.end == end_ip) {
            output(start_ip, end_ip);
            break;
        }
//        cout<<mask<<endl;
        //取重叠部分
        uint64_t overlapping =
                (end_ip - start_ip) > (ip_seg.end - start_ip) ?
                                                                (ip_seg.end - start_ip) :
                                                                (end_ip - start_ip);
        if (overlapping * 100 / (ip_seg.end - ip_seg.start) < MERGE_RATE) {
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

uint g_start = 0;
uint g_end = 0;
bool calc(uint start, uint end, void* user_data)
{
//    cout << inet_ntoa(*(struct in_addr*) &start);
//    cout<< "/" << inet_ntoa(*(struct in_addr*) &end)<< "\t"
//            << start << "\t" << end << endl;
    if (g_end == 0) {
        g_start = start;
        g_end = end;
    } else {
        if (start == g_end + 1) {
            g_end = end;
        } else {
            calc_local(g_start, g_end);
            g_start = start;
            g_end = end;
        }
    }
    return false;
}

int main(int argc, char **argv)
{

    if (argc < 2) {
        cout << "usage: combiner input_file1 input_file2 ..." << endl;
        return -1;
    }

    seg_tree_t* seg_tree = seg_tree_new(NULL);

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
            //空行
            if (buffer[0] == '\0') {
                continue;
            }
            struct ip_seg ip_seg = get_ip_seg_from_mask_str(buffer);
            seg_tree_insert(seg_tree, ip_seg.start, ip_seg.end);
        }

        fclose(fp);
    }

    if (seg_tree_nnodes(seg_tree) == 0) {
        cout << "No IP found in input files" << endl;
        return -1;
    }

    seg_tree_foreach(seg_tree, calc, NULL);
    if (g_end != 0) {
        calc_local(g_start, g_end);
    }

    return 0;
}
