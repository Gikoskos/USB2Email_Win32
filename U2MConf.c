/******************************************
*                U2MConf.c                 *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "U2MWin32.h"
#include <confuse.h>

char *cfg_filename = "U2M.conf";

BOOL parseConfFile(VOID)
{
    cfg_opt_t email_opts[] = {
        CFG_STR("from", NULL, CFGF_NONE),
        CFG_STR("to", NULL, CFGF_NONE),
        CFG_STR("cc", NULL, CFGF_NONE),
        CFG_STR("subject", NULL, CFGF_NONE),
        CFG_STR("body", NULL, CFGF_NONE),
        CFG_STR("pass", NULL, CFGF_NONE),
        CFG_STR("smtp_server", NULL, CFGF_NONE),
        CFG_INT("port", 0, CFGF_NONE),
        CFG_END()
    };
    /*HANDLE ProcHeap = GetProcessHeap();
    if (!ProcHeap)
        return FALSE;

#define HEAP_ALLOCEX(x) \
HeapAlloc(ProcHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, x)*/

    cfg_t *U2MConf;
    U2MConf = cfg_init(email_opts, CFGF_NONE);
    int cfg_err = cfg_parse(U2MConf, cfg_filename);

    if (cfg_err) {
        if (cfg_err == CFG_PARSE_ERROR)
            fprintf(stderr, "Error at parsing %s\n", cfg_filename);
        else
            fprintf(stderr, "Couldn't find configuration file with filename %s\n", cfg_filename);
        cfg_free(U2MConf);
        return FALSE;
    }

    char *temp[6] = {
        cfg_getstr(U2MConf, "from"),
        cfg_getstr(U2MConf, "to"),
        cfg_getstr(U2MConf, "subject"),
        cfg_getstr(U2MConf, "body"),
        cfg_getstr(U2MConf, "pass"),
        cfg_getstr(U2MConf, "smtp_server")
    };

#ifdef DEBUG
    printf("%s\n%s\n%s\n%s\n%s\n%s\n%ld\n", temp[0], temp[1], temp[2],
           temp[3], temp[4], temp[5], cfg_getint(U2MConf, "port"));
#endif
    ClearEmailData();

    if (temp[0]) {
        FROM = malloc(sizeof(temp[0])*strlen(temp[0]) + 1);
        strcpy(FROM, temp[0]);
    }
    if (temp[1]) {
        TO = malloc(sizeof(temp[1])*strlen(temp[1]) + 1);
        strcpy(TO, temp[1]);
    }
    if (temp[2]) {
        SUBJECT = malloc(sizeof(temp[2])*strlen(temp[2]) + 1);
        strcpy(SUBJECT, temp[2]);
    }
    if (temp[3]) {
        BODY = malloc(sizeof(temp[3])*strlen(temp[3]) + 1);
        strcpy(BODY, temp[3]);
    }
    if (temp[4]) {
        pass = malloc(sizeof(temp[4])*strlen(temp[4]) + 1);
        strcpy(pass, temp[4]);
    }
    if (temp[5]) {
        SMTP_SERVER = malloc(sizeof(temp[5])*strlen(temp[5]) + 1);
        strcpy(SMTP_SERVER, temp[5]);
    }

    int signed_port = cfg_getint(U2MConf, "port");
    PORT = (UINT)signed_port;

    cfg_free(U2MConf);
    return TRUE;
}
