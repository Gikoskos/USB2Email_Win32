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
        CFG_STR("From", NULL, CFGF_NONE),
        CFG_STR("To", NULL, CFGF_NONE),
        CFG_STR("Cc", NULL, CFGF_NONE),
        CFG_STR("Subject", NULL, CFGF_NONE),
        CFG_STR("Body", NULL, CFGF_NONE),
        CFG_STR("Password", NULL, CFGF_NONE),
        CFG_STR("SMTP_server", NULL, CFGF_NONE),
        CFG_INT("Port_number", 0, CFGF_NONE),
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
#ifdef DEBUG
        if (cfg_err == CFG_PARSE_ERROR)
            fprintf(stderr, "Error at parsing %s\n", cfg_filename);
        else
            fprintf(stderr, "Couldn't find configuration file with filename %s\n", cfg_filename);
#endif
        cfg_free(U2MConf);
        return FALSE;
    }

    char *temp[7] = {
        cfg_getstr(U2MConf, "From"),
        cfg_getstr(U2MConf, "To"),
        cfg_getstr(U2MConf, "Cc"),
        cfg_getstr(U2MConf, "Subject"),
        cfg_getstr(U2MConf, "Body"),
        cfg_getstr(U2MConf, "Password"),
        cfg_getstr(U2MConf, "SMTP_server")
    };

#ifdef DEBUG
    printf("%s\n%s\n%s\n%s\n%s\n%s\n%ld\n", temp[0], temp[1], temp[2],
           temp[3], temp[4], temp[5], cfg_getint(U2MConf, "Port_number"));
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
        CC = malloc(sizeof(temp[2])*strlen(temp[2]) + 1);
        strcpy(CC, temp[2]);
    }
    if (temp[3]) {
        SUBJECT = malloc(sizeof(temp[3])*strlen(temp[3]) + 1);
        strcpy(SUBJECT, temp[3]);
    }
    if (temp[4]) {
        BODY = malloc(sizeof(temp[4])*strlen(temp[4]) + 1);
        strcpy(BODY, temp[4]);
    }
    if (temp[5]) {
        pass = malloc(sizeof(temp[5])*strlen(temp[5]) + 1);
        strcpy(pass, temp[5]);
    }
    if (temp[6]) {
        SMTP_SERVER = malloc(sizeof(temp[6])*strlen(temp[6]) + 1);
        strcpy(SMTP_SERVER, temp[6]);
    }

    int signed_port = cfg_getint(U2MConf, "Port_number");
    PORT = (UINT)signed_port;

    cfg_free(U2MConf);
    return TRUE;
}

BOOL saveConfFile(VOID)
{
    FILE *U2Mconf_file = fopen(cfg_filename, "w");

    if (!U2Mconf_file) return FALSE;

    cfg_opt_t email_opts[] = {
        CFG_STR("From", FROM, CFGF_NONE),
        CFG_STR("To", TO, CFGF_NONE),
        CFG_STR("Cc", CC, CFGF_NONE),
        CFG_STR("Subject", SUBJECT, CFGF_NONE),
        CFG_STR("Body", BODY, CFGF_NONE),
        CFG_STR("Password", pass, CFGF_NONE),
        CFG_STR("SMTP_server", SMTP_SERVER, CFGF_NONE),
        CFG_INT("Port_number", (int)PORT, CFGF_NONE),
        CFG_END()
    };

    cfg_t *U2MConf = cfg_init(email_opts, CFGF_NONE);
    cfg_print(U2MConf, U2Mconf_file);

    fclose(U2Mconf_file);
    cfg_free(U2MConf);
    return TRUE;
}