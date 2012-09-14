#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <linux/limits.h>
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
const uint32_t MAX_PATH_LEN = (PATH_MAX-4); //counting the backup suffix, "orig"
bool process_logfile();
bool output();

int main(int argc, char*argv[])
{
    if (init_application(argc, argv, options, description) < 0) {
        exit(-1);
    }
    if (strlen(log_file) == 0
        || strlen(w_list) == 0
        || strlen(b_list) == 0) {
        LOG(LOG_LEVEL_ERROR, "Empty file name");
        exit(-1);
    }
    if (!c_list || strlen(c_list) == 0
        || !c80_list || strlen(c80_list) == 0
        || !c443_list || strlen(c443_list) == 0
        || !c8080_list || strlen(c8080_list) == 0 ) {
        LOG(LOG_LEVEL_WARNING, "Some of c.list missed");
        c_list = "c.list";
        c80_list = "c80.list";
        c443_list = "c443.list";
        c8080_list = "c8080.list";
    }

    if (strlen(c_list) > MAX_PATH_LEN
        || strlen(c80_list) > MAX_PATH_LEN
        || strlen(c443_list) > MAX_PATH_LEN
        || strlen(c8080_list) > MAX_PATH_LEN) {
        LOG(LOG_LEVEL_WARNING, "Some of c.list name length too long");
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

    if (!process_logfile()) {
        LOG(LOG_LEVEL_ERROR, "Process log file failed");
        exit(-1);
    }

    if (!output()) {
        LOG(LOG_LEVEL_ERROR, "Output failed");
        exit(-1);
    }

    return 0;
}

//----------------------- Functions -----------------------

bool process_logfile()
{
    //time_t start = time(NULL);
    //srand(start);

    FILE        *input_file;
    size_t      len;
    ssize_t     linelen = 0;
    uint32_t    line_count = 0;
    char        *line = NULL;
    bool        process_result = true;
    input_file = fopen(log_file, "r");
    if (!input_file) {
        LOG(LOG_LEVEL_ERROR, "Open log file failed");
        return false;
    }
    assert(input_file != NULL);

    linelen = getline(&line, &len, input_file);
    while ((linelen = getline(&line, &len, input_file)) != -1
            && ++line_count < MAX_LINE_COUNT
            && process_result) {
        //LOG(LOG_LEVEL_TRACE, "Read a line: %s, length: %zu", line, linelen);
        line[linelen-1] = '\0';

        uint32_t addr;
        logrecord_t record;
        port_type_t type;

        //TODO Maybe add a counter here to record all ignored lines
        if (!parse_log(line, &addr, &record, &type) || ip_in_wb_list(addr))
            continue;    // Ignore this line, continue to next

        bool valiable = false;
        if (type == TCP_PORT_80
            || type == TCP_PORT_443
            || type == TCP_PORT_8080) {
            // TODO Test if the addr could be reached
            // ...
        }

        if (valiable)
            record2map(addr, &record, type);
    }

    if (line)
        free(line);
    fclose(input_file);

    return true;
}

bool output()
{
    const char *suffix = ".orig";

    char *name = new char[PATH_MAX];
    if (name == NULL) {
        LOG(LOG_LEVEL_ERROR, "Memory allocation failed");
        return false;
    }
    sprintf(name, "%s%s", c_list, suffix);
    rename(c_list, name);
    sprintf(name, "%s%s", c80_list, suffix);
    rename(c80_list, name);
    sprintf(name, "%s%s", c443_list, suffix);
    rename(c443_list, name);
    sprintf(name, "%s%s", c8080_list, suffix);
    rename(c8080_list, name);

    bool success = false;
    if ((success = dump_list(TCP_PORT_80, c80_list)
                    && dump_list(TCP_PORT_443, c443_list)
                    && dump_list(TCP_PORT_8080, c8080_list)
                    && dump_list(REGULAR_PORT, c_list))
        ) {
        sprintf(name, "%s%s", c_list, suffix);
        remove(name);
        sprintf(name, "%s%s", c80_list, suffix);
        remove(name);
        sprintf(name, "%s%s", c443_list, suffix);
        remove(name);
        sprintf(name, "%s%s", c8080_list, suffix);
        remove(name);
    } else {
        sprintf(name, "%s%s", c_list, suffix);
        rename(name, c_list);
        sprintf(name, "%s%s", c80_list, suffix);
        rename(name, c80_list);
        sprintf(name, "%s%s", c443_list, suffix);
        rename(name, c443_list);
        sprintf(name, "%s%s", c8080_list, suffix);
        rename(name, c8080_list);
    }

    delete[] name;
    return success;
}
