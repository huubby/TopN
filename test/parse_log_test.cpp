#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "nat_log_parser.h"
#include "log.h"

int
main(int argc, char *argv[])
{
    FILE *input_file = fopen("../test/nat.log.test", "r");
    if (!input_file) {
        LOG(LOG_LEVEL_ERROR, "Open log file failed");
        exit(-1);
    }
    assert(input_file != NULL);

    size_t      len;
    char        *line = NULL;
    ssize_t linelen = getline(&line, &len, input_file);
    while (linelen != -1) {
        line[linelen-1]='\0';
        uint32_t addr = 0;
        logrecord_t record;
        port_type_t type;
        if (!parse_log(line, &addr, &record, &type)) {
            LOG(LOG_LEVEL_ERROR, "parse failed");
        }

        printf("Addr: %u, count: %u, bytes: %lu, type: %d\n"
                , addr, record.count, record.bytes, type);
        linelen = getline(&line, &len, input_file);
    }

    return 0;
}
