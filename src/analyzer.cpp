#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "log.h"
#include "cmdparse.h"
#include "nat_log_parser.h"
#include "clist_parser.h"
#include "caches_maps.h"

//-------------------- Command Line Arguments ----------------------
static char *log_file = NULL;
static char *w_list = NULL;
static char *b_list = NULL;

static char *c_list = NULL;
static char *c80_list = NULL;
static char *c443_list = NULL;
static char *c8080_list = NULL;
CommandLineOptions_t options[] = {
    { "f", "The name of NAT log file waiting for processing"
        , "log file name", OPTION_REQUIRED, ARG_STR, &log_file },
    { "w", "The name of w.list file"
        , "w.list name", OPTION_REQUIRED, ARG_STR, &w_list },
    { "b", "The name of b.list file"
        , "b.list name", OPTION_REQUIRED, ARG_STR, &b_list },
    { "c", "The name of c.list file"
        , "c.list name", OPTION_OPTIONAL, ARG_STR, &c_list },
    { "c80", "The name of c.list.80 file"
        , "c.list.80 name", OPTION_OPTIONAL, ARG_STR, &c80_list },
    { "c443", "The name of c.list.443 file"
        , "c.list.443 name", OPTION_OPTIONAL, ARG_STR, &c443_list },
    { "c8080", "The name of c.list.8080 file"
        , "c.list.8080 name", OPTION_OPTIONAL, ARG_STR, &c8080_list },
    {NULL, NULL, NULL, 0, 0, NULL}
};
static char description[] = {"Linux NAT log file analyzer\nVersion 1.0\n"};

//--------------------- Global Datas -------------------------------
const uint32_t MAX_LINE_COUNT = MAX_LOG_LINE;
bool process(const char *log, uint32_t len);

int main(int argc, char*argv[])
{
    FILE        *input_file;
    size_t      len;
    ssize_t     linelen = 0;
    uint32_t    line_count = 0;
    char        *line = NULL;
    bool        process_result = true;
    if (init_application(argc, argv, options, description) < 0) {
        exit(-1);
    }
    if (strlen(log_file) == 0
        || strlen(w_list) == 0
        || strlen(b_list) == 0) {
        LOG(LOG_LEVEL_ERROR, "Empty file name");
        exit(-1);
    }
    if (strlen(c_list) == 0
        || strlen(c80_list) == 0
        || strlen(c443_list) == 0
        || strlen(c8080_list) == 0 ) {
        LOG(LOG_LEVEL_WARNING, "Some of c.list missed");
    }

    init_caches(0);

    if (!build_wb_list(w_list)
        || !build_wb_list(b_list)
        || !build_map_from_c_list(c_list, REGULAR_PORT)
        || !build_map_from_c_list(c80_list, TCP_PORT_80)
        || !build_map_from_c_list(c443_list, TCP_PORT_443)
        || !build_map_from_c_list(c8080_list, TCP_PORT_8080)) {
        LOG(LOG_LEVEL_ERROR, "Build maps failed");
        exit(-1);
    }

    input_file = fopen(log_file, "r");
    if (!input_file) {
        LOG(LOG_LEVEL_ERROR, "Open log file failed");
        exit(-1);
    }
    assert(input_file != NULL);

    linelen = getline(&line, &len, input_file);
    while (linelen != -1
            && line_count < MAX_LINE_COUNT
            && process_result) {
        //LOG(LOG_LEVEL_TRACE, "Read a line: %s, length: %zu", line, linelen);
        line[linelen-1] = '\0';
        process_result = process(line, len);
        line_count++;
        linelen = getline(&line, &len, input_file);
    }

    if (line)
        free(line);
    fclose(input_file);

    return 0;
}

//----------------------- Functions -----------------------

bool process(const char *log, uint32_t len)
{
    //time_t start = time(NULL);
    //srand(start);

    uint32_t addr;
    logrecord_t record;
    port_type_t type;

    //TODO Maybe add a counter here to record all ignored lines
    if (!parse_log(log, &addr, &record, &type))
        return true;    // Ignore this line, continue to next

    // TODO Test if the addr could be reached
    // ...

    record2map(addr, &record, type);

    return true;
}

