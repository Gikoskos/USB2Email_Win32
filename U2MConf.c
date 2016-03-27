/******************************************
*                U2MConf.c                 *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "U2MWin32.h"
#include "confuse.h"


char *cfg_filename = "U2M.conf";
TCHAR *registry_path = _T("Software\\USB2Email");
TCHAR *reg_subkeys[] = {
    _T("TrayIcon"), _T("VendorID"),
    _T("DeviceID"), _T("Autostart"),
    _T("Timeout"), _T("Max_failed_Emails"),
    _T("Check_for_Valid_Email"), _T("USB_Auto_Refresh")
};

/* functions for handling the configuration file */
BOOL parseConfFile(user_input_data *user_dat)
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

    cfg_t *U2MConf;
    U2MConf = cfg_init(email_opts, CFGF_NONE);
    int cfg_err = cfg_parse(U2MConf, cfg_filename);

    if (cfg_err) {
#if 0
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

#if 0
    printf("%s\n%s\n%s\n%s\n%s\n%s\n%ld\n", temp[0], temp[1], temp[2],
           temp[3], temp[4], temp[5], cfg_getint(U2MConf, "Port_number"));
#endif
    if (user_dat->FROM) free(user_dat->FROM);
    if (user_dat->TO) free(user_dat->TO);
    if (user_dat->CC) free(user_dat->CC);
    if (user_dat->SUBJECT) free(user_dat->SUBJECT);
    if (user_dat->BODY) free(user_dat->BODY);
    user_dat->FROM = user_dat->TO = user_dat->CC = user_dat->SUBJECT = user_dat->BODY = NULL;

    if (temp[0]) {
        user_dat->FROM = malloc(sizeof(temp[0])*strlen(temp[0]) + 1);
        StringCchCopyA(user_dat->FROM, 255, temp[0]);
    }
    if (temp[1]) {
        user_dat->TO = malloc(sizeof(temp[1])*strlen(temp[1]) + 1);
        StringCchCopyA(user_dat->TO, 255, temp[1]);
    }
    if (temp[2]) {
        user_dat->CC = malloc(sizeof(temp[2])*strlen(temp[2]) + 1);
        StringCchCopyA(user_dat->CC, 255, temp[2]);
    }
    if (temp[3]) {
        user_dat->SUBJECT = malloc(sizeof(temp[3])*strlen(temp[3]) + 1);
        StringCchCopyA(user_dat->SUBJECT, 255, temp[3]);
    }
    if (temp[4]) {
        user_dat->BODY = malloc(sizeof(temp[4])*strlen(temp[4]) + 1);
        StringCchCopyA(user_dat->BODY, 255, temp[4]);
    }
    if (temp[5]) {
        user_dat->pass = malloc(sizeof(temp[5])*strlen(temp[5]) + 1);
        StringCchCopyA(user_dat->pass, 255, temp[5]);
    }
    if (temp[6]) {
        user_dat->SMTP_SERVER = malloc(sizeof(temp[6])*strlen(temp[6]) + 1);
        StringCchCopyA(user_dat->SMTP_SERVER, 255, temp[6]);
    }

    int signed_port = cfg_getint(U2MConf, "Port_number");
    user_dat->PORT = (UINT)signed_port;

    cfg_free(U2MConf);
    return TRUE;
}

BOOL saveConfFile(user_input_data user_dat)
{
    FILE *U2Mconf_file = fopen(cfg_filename, "w");

    if (!U2Mconf_file) return FALSE;

    cfg_opt_t email_opts[] = {
        CFG_STR("From", user_dat.FROM, CFGF_NONE),
        CFG_STR("To", user_dat.TO, CFGF_NONE),
        CFG_STR("Cc", user_dat.CC, CFGF_NONE),
        CFG_STR("Subject", user_dat.SUBJECT, CFGF_NONE),
        CFG_STR("Body", user_dat.BODY, CFGF_NONE),
        CFG_STR("Password", user_dat.pass, CFGF_NONE),
        CFG_STR("SMTP_server", user_dat.SMTP_SERVER, CFGF_NONE),
        CFG_INT("Port_number", (int)user_dat.PORT, CFGF_NONE),
        CFG_END()
    };

    cfg_t *U2MConf = cfg_init(email_opts, CFGF_NONE);
    cfg_print(U2MConf, U2Mconf_file);

    fclose(U2Mconf_file);
    cfg_free(U2MConf);

    return TRUE;
}

/* functions for handling data in the registry */

BOOL WriteDataToU2MReg(user_input_data user_dat)
{
    HKEY U2MRegkey = NULL, WinAuto = NULL;
    TCHAR U2MPath[1024];

    if (RegCreateKeyEx(HKEY_CURRENT_USER, registry_path, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL,
                       &U2MRegkey, NULL) != ERROR_SUCCESS) return FALSE;

    RegSetValueEx(U2MRegkey, reg_subkeys[0], 0, REG_DWORD, (BYTE*)&user_dat.TrayIcon, sizeof(user_dat.TrayIcon));
    RegSetValueEx(U2MRegkey, reg_subkeys[1], 0, REG_DWORD, (BYTE*)&user_dat.usb_id_selection[0], sizeof(user_dat.usb_id_selection[0]));
    RegSetValueEx(U2MRegkey, reg_subkeys[2], 0, REG_DWORD, (BYTE*)&user_dat.usb_id_selection[1], sizeof(user_dat.usb_id_selection[1]));
    RegSetValueEx(U2MRegkey, reg_subkeys[3], 0, REG_DWORD, (BYTE*)&user_dat.Autostart, sizeof(user_dat.Autostart));
    RegSetValueEx(U2MRegkey, reg_subkeys[4], 0, REG_DWORD, (BYTE*)&user_dat.TIMEOUT, sizeof(user_dat.TIMEOUT));
    RegSetValueEx(U2MRegkey, reg_subkeys[5], 0, REG_DWORD, (BYTE*)&user_dat.MAX_FAILED_EMAILS, sizeof(user_dat.MAX_FAILED_EMAILS));
    RegSetValueEx(U2MRegkey, reg_subkeys[6], 0, REG_DWORD, (BYTE*)&user_dat.ValidEmailCheck, sizeof(user_dat.ValidEmailCheck));
    RegSetValueEx(U2MRegkey, reg_subkeys[7], 0, REG_DWORD, (BYTE*)&user_dat.USBRefresh, sizeof(user_dat.USBRefresh));

    if (RegCloseKey(U2MRegkey) != ERROR_SUCCESS) return FALSE;

    if (!GetModuleFileName(0, U2MPath, 1024)) return FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 
                     0, KEY_ALL_ACCESS, &WinAuto) != ERROR_SUCCESS) return FALSE;

    if (user_dat.Autostart) {
        RegSetValueEx(WinAuto, _T("USB2Email"), 0, REG_SZ, (LPBYTE)U2MPath, sizeof(TCHAR)*(_tcslen(U2MPath) + 1));
    } else {
        RegDeleteKeyValue(WinAuto, NULL, _T("USB2Email"));
    }

    if (RegCloseKey(WinAuto) != ERROR_SUCCESS) return FALSE;
    return TRUE;
}

BOOL GetU2MRegData(user_input_data *user_dat)
{
    HKEY U2MRegkey = NULL;
    DWORD RegDisposition;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, registry_path, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &U2MRegkey, &RegDisposition) != ERROR_SUCCESS) return FALSE;

    if (RegDisposition == REG_CREATED_NEW_KEY) {
        goto CLOSE_KEY;
    } else if (RegDisposition == REG_OPENED_EXISTING_KEY) {
        DWORD sub_data[8];
        DWORD data_size = (DWORD)sizeof(DWORD);

        for (int i = 0; i < 8; i++) {
            if (RegQueryValueEx(U2MRegkey, reg_subkeys[i], NULL, NULL, 
                                (BYTE*)&sub_data[i], &data_size) != ERROR_SUCCESS) goto CLOSE_KEY;
        }
        user_dat->TrayIcon = (BOOL)(sub_data[0] | 0x0);
        user_dat->usb_id_selection[0] = (UINT)sub_data[1];
        user_dat->usb_id_selection[1] = (UINT)sub_data[2];
        user_dat->Autostart = (BOOL)(sub_data[3] | 0x0);
        user_dat->TIMEOUT = (UINT)sub_data[4];
        user_dat->MAX_FAILED_EMAILS = (UINT)sub_data[5];
        user_dat->ValidEmailCheck = (BOOL)(sub_data[6] | 0x0);
        user_dat->USBRefresh = (BOOL)(sub_data[7] | 0x0);
    }

CLOSE_KEY:
    if (RegCloseKey(U2MRegkey) != ERROR_SUCCESS) return FALSE;

    if (RegDisposition == REG_CREATED_NEW_KEY) return WriteDataToU2MReg(*user_dat);
    else return TRUE;
}
