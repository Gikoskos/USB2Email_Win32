/******************************************
*                U2MConf.c                 *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "U2MWin32.h"
#include "confuse.h"


char *cfg_filename = "U2M.conf";
TCHAR *registry_path = TEXT("Software\\USB2Email");
TCHAR *reg_subkeys[] = {
    TEXT("TrayIcon"), TEXT("VendorID"),
    TEXT("DeviceID"), TEXT("Autostart"),
    TEXT("Timeout"), TEXT("Max_failed_Emails"),
    TEXT("Check_for_Valid_Email"), TEXT("USB_Auto_Refresh")
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

    if (temp[0]) {
        size_t len;
        if (SUCCEEDED(StringCbLengthA(temp[0], MAX_BUFFER, &len))) {
            if (user_dat->FROM) HeapFree(GetProcessHeap(), 0, user_dat->FROM);
            user_dat->FROM = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(temp[0])*(len + 1));
            if (user_dat->FROM) StringCchCopyA(user_dat->FROM, MAX_BUFFER, temp[0]);
            else __MsgBoxGetLastError(NULL, TEXT("HeapAlloc()"), __LINE__);
        }
    }
    if (temp[1]) {
        size_t len;
        if (SUCCEEDED(StringCbLengthA(temp[1], MAX_BUFFER, &len))) {
            if (user_dat->TO) HeapFree(GetProcessHeap(), 0, user_dat->TO);
            user_dat->TO = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(temp[1])*(len + 1));
            if (user_dat->TO) StringCchCopyA(user_dat->TO, MAX_BUFFER, temp[1]);
            else __MsgBoxGetLastError(NULL, TEXT("HeapAlloc()"), __LINE__);
        }
    }
    if (temp[2]) {
        size_t len;
        if (SUCCEEDED(StringCbLengthA(temp[2], MAX_BUFFER, &len))) {
            if (user_dat->CC) HeapFree(GetProcessHeap(), 0, user_dat->CC);
            user_dat->CC = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(temp[2])*(len + 1));
            if (user_dat->CC) StringCchCopyA(user_dat->CC, MAX_BUFFER, temp[2]);
            else __MsgBoxGetLastError(NULL, TEXT("HeapAlloc()"), __LINE__);
        }
    }
    if (temp[3]) {
        size_t len;
        if (SUCCEEDED(StringCbLengthA(temp[3], MAX_BUFFER, &len))) {
            if (user_dat->SUBJECT) HeapFree(GetProcessHeap(), 0, user_dat->SUBJECT);
            user_dat->SUBJECT = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(temp[3])*(len + 1));
            if (user_dat->SUBJECT) StringCchCopyA(user_dat->SUBJECT, MAX_BUFFER, temp[3]);
            else __MsgBoxGetLastError(NULL, TEXT("HeapAlloc()"), __LINE__);
        }
    }
    if (temp[4]) {
        size_t len;
        if (SUCCEEDED(StringCbLengthA(temp[4], MAX_BUFFER, &len))) {
            if (user_dat->BODY) HeapFree(GetProcessHeap(), 0, user_dat->BODY);
            user_dat->BODY = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(temp[4])*(len + 1));
            if (user_dat->BODY) StringCchCopyA(user_dat->BODY, MAX_BUFFER, temp[4]);
            else __MsgBoxGetLastError(NULL, TEXT("HeapAlloc()"), __LINE__);
        }
    }
    if (temp[5]) {
        size_t len;
        if (SUCCEEDED(StringCbLengthA(temp[5], MAX_BUFFER, &len))) {
            if (user_dat->pass) HeapFree(GetProcessHeap(), 0, user_dat->pass);
            user_dat->pass = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(temp[5])*(len + 1));
            if (user_dat->pass) StringCchCopyA(user_dat->pass, MAX_BUFFER, temp[5]);
            else __MsgBoxGetLastError(NULL, TEXT("HeapAlloc()"), __LINE__);
        }
    }
    if (temp[6]) {
        size_t len;
        if (SUCCEEDED(StringCbLengthA(temp[6], MAX_BUFFER, &len))) {
            if (user_dat->SMTP_SERVER) HeapFree(GetProcessHeap(), 0, user_dat->SMTP_SERVER);
            user_dat->SMTP_SERVER = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(temp[6])*(len + 1));
            if (user_dat->SMTP_SERVER) StringCchCopyA(user_dat->SMTP_SERVER, MAX_BUFFER, temp[6]);
            else __MsgBoxGetLastError(NULL, TEXT("HeapAlloc()"), __LINE__);
        }
    }

    int signed_port = cfg_getint(U2MConf, "Port_number");
    user_dat->PORT = (UINT)signed_port;

    cfg_free(U2MConf);

#ifdef DEBUG
    printf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n%u\n", user_dat->FROM, user_dat->TO, user_dat->CC,
           user_dat->SUBJECT, user_dat->BODY, user_dat->pass, user_dat->SMTP_SERVER, user_dat->PORT);
#endif
    return TRUE;
}

BOOL saveConfFile(user_input_data *user_dat)
{
    FILE *U2Mconf_file = fopen(cfg_filename, "w");

    if (!U2Mconf_file) return FALSE;

    cfg_opt_t email_opts[] = {
        CFG_STR("From", user_dat->FROM, CFGF_NONE),
        CFG_STR("To", user_dat->TO, CFGF_NONE),
        CFG_STR("Cc", user_dat->CC, CFGF_NONE),
        CFG_STR("Subject", user_dat->SUBJECT, CFGF_NONE),
        CFG_STR("Body", user_dat->BODY, CFGF_NONE),
        CFG_STR("Password", user_dat->pass, CFGF_NONE),
        CFG_STR("SMTP_server", user_dat->SMTP_SERVER, CFGF_NONE),
        CFG_INT("Port_number", (int)user_dat->PORT, CFGF_NONE),
        CFG_END()
    };

    cfg_t *U2MConf = cfg_init(email_opts, CFGF_NONE);
    cfg_print(U2MConf, U2Mconf_file);

    fclose(U2Mconf_file);
    cfg_free(U2MConf);

    return TRUE;
}

/* functions for handling data in the registry */

BOOL WriteDataToU2MReg(user_input_data *user_dat)
{
    HKEY U2MRegkey = NULL, WinAuto = NULL;
    TCHAR U2MPath[1024];

    if (RegCreateKeyEx(HKEY_CURRENT_USER, registry_path, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL,
                       &U2MRegkey, NULL) != ERROR_SUCCESS) return FALSE;

    RegSetValueEx(U2MRegkey, reg_subkeys[0], 0, REG_DWORD, (BYTE*)&user_dat->TrayIcon, sizeof(user_dat->TrayIcon));
    RegSetValueEx(U2MRegkey, reg_subkeys[1], 0, REG_DWORD, (BYTE*)&user_dat->usb_id_selection[0], sizeof(user_dat->usb_id_selection[0]));
    RegSetValueEx(U2MRegkey, reg_subkeys[2], 0, REG_DWORD, (BYTE*)&user_dat->usb_id_selection[1], sizeof(user_dat->usb_id_selection[1]));
    RegSetValueEx(U2MRegkey, reg_subkeys[3], 0, REG_DWORD, (BYTE*)&user_dat->Autostart, sizeof(user_dat->Autostart));
    RegSetValueEx(U2MRegkey, reg_subkeys[4], 0, REG_DWORD, (BYTE*)&user_dat->TIMEOUT, sizeof(user_dat->TIMEOUT));
    RegSetValueEx(U2MRegkey, reg_subkeys[5], 0, REG_DWORD, (BYTE*)&user_dat->MAX_FAILED_EMAILS, sizeof(user_dat->MAX_FAILED_EMAILS));
    RegSetValueEx(U2MRegkey, reg_subkeys[6], 0, REG_DWORD, (BYTE*)&user_dat->ValidEmailCheck, sizeof(user_dat->ValidEmailCheck));
    RegSetValueEx(U2MRegkey, reg_subkeys[7], 0, REG_DWORD, (BYTE*)&user_dat->USBRefresh, sizeof(user_dat->USBRefresh));

    if (RegCloseKey(U2MRegkey) != ERROR_SUCCESS) return FALSE;

    if (!GetModuleFileName(0, U2MPath, 1024)) return FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 
                     0, KEY_ALL_ACCESS, &WinAuto) != ERROR_SUCCESS) return FALSE;

    if (user_dat->Autostart) {
        RegSetValueEx(WinAuto, TEXT("USB2Email"), 0, REG_SZ, (LPBYTE)U2MPath, sizeof(TCHAR)*(_tcslen(U2MPath) + 1));
    } else {
        RegDeleteKeyValue(WinAuto, NULL, TEXT("USB2Email"));
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

    if (RegDisposition == REG_OPENED_EXISTING_KEY) {
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

    if (RegDisposition == REG_CREATED_NEW_KEY) return WriteDataToU2MReg(user_dat);

    return TRUE;
}
