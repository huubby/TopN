#ifndef __CMD_PARSE_H__
#define __CMD_PARSE_H__

typedef struct 
{
    const char *option;
    const char *description;
    const char *descriptionShort;
    int  mode;
    int  type;
    char **result;
} CommandLineOptions_t;

#define OPTION_OPTIONAL 0
#define OPTION_REQUIRED 1
#define OPTION_BOOLEAN  2

#define ARG_NONE 0
#define ARG_STR  1
#define ARG_NUM  2

extern int init_application(int argc, char *argv[],  CommandLineOptions_t *app_options, char *app_description);

#endif // __CMD_PARSE_H__
