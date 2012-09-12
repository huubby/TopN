#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <libgen.h>
#include "types.h"

#include "cmdparse.h"

static void PrintUsage(char *cmdName, CommandLineOptions_t *app_options, char *app_description)
{
    CommandLineOptions_t *options=NULL;

    if(app_description) {
        printf("\n%s\n%s\n", basename(cmdName), app_description);
    } else {
        printf("\n%s\n", basename(cmdName));
    }
    printf("Syntax:\n");
    /* First pass the required parameters. */
    options=app_options;
    printf("%s ", basename(cmdName));
    while(options->option) {
        if(options->mode==1) {
            printf("-%s", options->option);
            if (options->descriptionShort != NULL) {
                printf("<%s>", options->descriptionShort);
            }
            printf(" ");
        }
        options++;
    }
    /* Pass the optional parameters. */
    options=app_options;
    printf("[-help] ");
    while(options->option) {
        if(!options->mode || options->mode==2) {
            printf("[-%s", options->option);
            if (options->descriptionShort != NULL) {
                printf("<%s>", options->descriptionShort);
            }
            printf("] ");
        }
        options++;
    }
    printf("\n\n");
    printf("Reguired parameters:\n");
    /* First pass the required parameters. */
    options=app_options;
    while(options->option) {
        if(options->mode==1) {
            printf("  %-10s:  %s\n",
                    options->option,
                    options->description);
        }
        options++;
    }
    /* Pass the optional parameters. */
    options=app_options;
    printf("Optional parameters:\n");
    printf("  %-10s:  %s\n", "help", "This message.");
    while(options->option) {
        if(!options->mode || options->mode==2) {
            printf("  %-10s:  %s\n",
                    options->option,
                    options->description);
        }
        options++;
    }
    printf("\n");
}

int init_application(int argc, char *argv[],  CommandLineOptions_t *app_options, char *app_description)
{
    bool boFoundArg=false, boFailSettingRequired=false;
    CommandLineOptions_t *options=NULL;
    int cp=0, aLen=0, arg=0;
    char *cc=NULL;

    /* Skip argv[0], it is the program name. */
    for(arg=1; arg<argc; arg++) {
        boFoundArg=false;
        if(argv[arg][0] != '-') {
            printf("option %s doesn't have a \"-\" to indicate that it is an option.\n", argv[arg]);
            PrintUsage(argv[0], app_options, app_description);
            return -1;
        }
        /* Look for general options. */
        if(!strcmp("-help", argv[arg])) {
            PrintUsage(argv[0], app_options, app_description);
            return -1;
        } else {
            /* Ok look for application specific options. */
            options=app_options;
            while(options->option) {
                if(!strncmp(options->option, &argv[arg][1], strlen(options->option))) {
                    if(*options->result != NULL) {
                        printf("Option \"%s\" is already set.\n", options->option);
                        return -1;
                    } else if(options->mode==OPTION_BOOLEAN) {
                        if(strlen(options->option) != strlen(&argv[arg][1])) {
                            printf("Option \"%s\" doesn't match %s.\n", options->option, &argv[arg][1]);
                            return -1;
                        } else {
                            *options->result="1"; /* This type of options are booleans. */
                            boFoundArg=true;
                        }
                    } else {
                        if(strlen(argv[arg]) == (strlen(options->option)+1)) {
                            /* In this case we expect that the option value is the next argument. */
                            if((arg+1) <argc) {
                                if(argv[arg+1][0] == '-') {
                                    printf("Option \"%s\" doesn't have any assignment.\n", options->option);
                                    return -1;
                                }
                                /* Ok, we can assume that the argument is the assignment. */
                                arg++; /* Skip to next argument. */
                                cc=argv[arg];
                                aLen=strlen(argv[arg]);
                            }
                        } else {
                            cc=&argv[arg][strlen(options->option)+1];
                            aLen=strlen(argv[arg])-(strlen(options->option)+1);
                        }
                        /* Strip '=' if present. */
                        for(cp=0;cp<aLen;cp++) {
                            if(cc[cp]=='=') {
                                continue;
                            } else {
                                break;
                            }
                        }
                        if(options->type == ARG_NUM) {
                            char *endptr=NULL;
                            if(strtol(&cc[cp], &endptr, 0)==-1) {
                                // Avoid compile warning
                            }
                            if(endptr == &cc[cp]) {
                                printf("Option \"%s\" expects numbers.\n", options->option);
                                return -1;
                            }
                        }
                        *options->result=&cc[cp];
                        boFoundArg=true;
                        break;
                    }
                }
                options++;
            }
        }
        if(boFoundArg==false) {
            printf("Unknown option: \"%s\"\n", argv[arg]);
            PrintUsage(argv[0], app_options, app_description);
            return -1;
        }
    }
    /* Check that all required parameters has been set. */
    options=app_options;
    while(options->option) {
        if(options->mode==OPTION_REQUIRED) {
            if(*options->result == NULL) {
                printf("Required parameter: \"%s\" is not set.\n", options->option);
                boFailSettingRequired=true;
            }
        }
        options++;
    }
    if(boFailSettingRequired) {
        PrintUsage(argv[0], app_options, app_description);
        return -1;
    }
    return 0;
}
