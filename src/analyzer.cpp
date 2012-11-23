#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include "log.h"
#include "cmdparse.h"
#include "nat_log_parser.h"
#include "clist_parser.h"
#include "caches_maps.h"
#include "touch.h"

//-------------------- Command Line Arguments ----------------------
static char *log_file = NULL;
static char *w_list = NULL;
static char *b_list = NULL;
static char *a_list = NULL;

static char *c_list = NULL;
static char *c80_list = NULL;
static char *c443_list = NULL;
static char *c8080_list = NULL;

static char *timeout = NULL;
CommandLineOptions_t options[] = {
    { "f", "The name of NAT log file waiting for processing"
        , "log file name", OPTION_REQUIRED, ARG_STR, &log_file },
    { "a", "The name of a.list file"
        , "a.list name", OPTION_REQUIRED, ARG_STR, &a_list },
    { "w", "The name of w.list file"
        , "w.list name", OPTION_OPTIONAL, ARG_STR, &w_list },
    { "b", "The name of b.list file"
        , "b.list name", OPTION_OPTIONAL, ARG_STR, &b_list },
    { "c", "The name of c.list file"
        , "c.list name", OPTION_OPTIONAL, ARG_STR, &c_list },
    { "h", "The name of c.list.80 file"
        , "c.list.80 name", OPTION_OPTIONAL, ARG_STR, &c80_list },
    { "s", "The name of c.list.443 file"
        , "c.list.443 name", OPTION_OPTIONAL, ARG_STR, &c443_list },
    { "t", "The name of c.list.8080 file"
        , "c.list.8080 name", OPTION_OPTIONAL, ARG_STR, &c8080_list },
    { "o", "The timeout for reachable detection, in millisecond, default 300."
        " Set 0 to skip detection"
        , "timeout value", OPTION_OPTIONAL, ARG_NUM, &timeout},
    {NULL, NULL, NULL, 0, 0, NULL}
};
static char description[] = {"Linux NAT log file analyzer\nVersion 1.2\n"};

//--------------------- Global Datas -------------------------------
static char *default_w_list = "w.list";
static char *default_b_list = "b.list";
static char *default_c_list = "c.list";
static char *default_c80_list = "c.list.80";
static char *default_c443_list = "c.list.443";
static char *default_c8080_list = "c.list.8080";
static uint32_t timeout_value = 300;
static bool w_list_exist = true;
static bool b_list_exist = true;

const uint32_t MAX_LINE_COUNT = MAX_LOG_LINE;
const uint32_t MAX_PATH_LEN = (PATH_MAX-4); //counting the backup suffix, "orig"
bool check_files();
bool process_logfile();
bool sort();
bool output();

int main(int argc, char*argv[])
{
    if (init_application(argc, argv, options, description) < 0) {
        exit(-1);
    }

    if(g_default_log != NULL){
        log_free(g_default_log);
    }
    g_default_log = log_new_stdout(LOG_LEVEL_WARNING);
    if (g_default_log == NULL) {
        exit(-1);
    }

    if (timeout != NULL) {
        timeout_value = strtol(timeout, NULL, 10);
    }

    if (!check_files()) {
        LOG(LOG_LEVEL_ERROR, "Invalid parameter(s)");
        exit(-1);
    }

    init_caches(0);

    if ((w_list_exist && !build_wb_list(w_list))
        || (b_list_exist && !build_wb_list(b_list))
        || !build_a_list(a_list)
        || !build_map_from_c_list(c_list, ALL_PORT)
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
bool create_file(const char *name)
{
    int fd = creat(name, 00666);
    if (fd == -1) {
        LOG(LOG_LEVEL_ERROR, "Create file %s failed", name);
    }
    return fd != -1;
}

bool file_exist_valid(const char *name, int mode)
{
    assert(name!=NULL);
    if (-1 == access(name, mode)) {
        //LOG(LOG_LEVEL_ERROR
        //        , "%s does not exist or permission denied, %s"
        //        , name, strerror(errno));
        return false;
    }

    return true;
}

typedef enum {
    W_LIST = 1
    , B_LIST
    , C_LIST
    , C_LIST_80
    , C_LIST_443
    , C_LIST_8080
} file_type_t;

bool type2name(file_type_t type, char **name)
{
    assert(name!=NULL);

    switch (type)
    {
        case W_LIST:
            *name = default_w_list;
            break;
        case B_LIST:
            *name = default_b_list;
            break;
        case C_LIST:
            *name = default_c_list;
            break;
        case C_LIST_80:
            *name = default_c80_list;
            break;
        case C_LIST_443:
            *name = default_c443_list;
            break;
        case C_LIST_8080:
            *name = default_c8080_list;
            break;
        default:
            return false;
    }

    return true;
}

bool check_file(file_type_t type, char **name)
{
    assert(name!=NULL);

    if (*name) {
        if (!file_exist_valid(*name, R_OK)){
            LOG(LOG_LEVEL_ERROR
                    , "%s, permission denied, %s"
                    , *name, strerror(errno));
            return false;
        }
    } else {
        if (!type2name(type, name))
            return false;

        if (!file_exist_valid(*name, F_OK)) {
            if (type == W_LIST) {
                w_list_exist = false;
                LOG(LOG_LEVEL_WARNING
                        , "%s is absent, no w.list will be used", *name);
            } else if (type == B_LIST) {
                b_list_exist = false;
                LOG(LOG_LEVEL_WARNING
                        , "%s is absent, no b.list will be used", *name);
            } else {
                LOG(LOG_LEVEL_WARNING
                        , "%s is absent, will create it automatically", *name);
                if (!create_file(*name)) {
                    LOG(LOG_LEVEL_ERROR, "Failed to create %s", *name);
                    return false;
                }
            }

        }
    }
    return true;
}

bool check_files()
{
    if (!check_file(C_LIST, &c_list)
        || !check_file(C_LIST_80, &c80_list)
        || !check_file(C_LIST_443, &c443_list)
        || !check_file(C_LIST_8080, &c8080_list)) {
        LOG(LOG_LEVEL_ERROR, "Checking c.list files failed");
        return false;
    }
    if (!check_file(W_LIST, &w_list) || !check_file(B_LIST, &b_list)) {
        LOG(LOG_LEVEL_ERROR, "Checking w.list/b.list failed");
        return false;
    }

    if (!file_exist_valid(log_file, R_OK)) {
        LOG(LOG_LEVEL_ERROR
                , "%s doesn't exist, or permission denied", log_file);
        return false;
    }
    if (!file_exist_valid(a_list, R_OK)) {
        LOG(LOG_LEVEL_ERROR
                , "%s doesn't exist, or permission denied", a_list);
        return false;
    }

    return true;
}

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

        bool valiable = true;
        if ((timeout_value != 0)
            && (type == TCP_PORT_80
                || type == TCP_PORT_443
                || type == TCP_PORT_8080)) {
            valiable = is_addr_reachable(addr, type, timeout_value);
        }

        if (valiable) {
            record2map(addr, &record, type);
            // All records should be put into common maps.
            if (type != ALL_PORT)
                record2map(addr, &record, ALL_PORT);
        }
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
                    && dump_list(ALL_PORT, c_list))
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
