/******************************************
*               U2MModule.c                *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "U2MWin32.h"
#include "usb_ids.h"
#include <setupapi.h>
#include <initguid.h>
#include <usbiodef.h> //for GUID_DEVINTERFACE_USB_DEVICE
#include <regstr.h>
#include <quickmail.h>


/*#ifdef DEFINE_GUID
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
            0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
#else
#error DEFINE_GUID is not defined.
#endif*/


//the prefix of ever U2M log file name
#define U2MLOG_PREFIX _T("U2MLog")
//maximum number of U2M log files that we're allowed to save on the disk
#define MAX_NUMBER_OF_U2M_LOGS  10000

/*** Globals ***/
ULONG scanned_usb_ids[MAX_CONNECTED_USB][2];

static ULONG curr_filename = 1; //current log file number
static BOOL Logging_System_Enabled = TRUE;

extern HINSTANCE *g_hInst;

/*******************************************
* Prototypes for functions with local scope *
 *******************************************/
BOOL USBisConnected();
UINT CALLBACK U2MThreadSingle(LPVOID dat);
//UINT CALLBACK U2MThreadMulti(LPVOID dat);
BOOL SendEmail(VOID);
UsbDevStruct *find(unsigned long vendor, unsigned long device);
BOOL GetDevIDs(ULONG *vid, ULONG *pid, TCHAR *devpath);
BOOL WriteToU2MLogFile(TCHAR *Logfile_name);


BOOL InitU2MThread(HWND hwnd)
{
    if (onoff) return FALSE;

    if (!user_dat.FROM) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_53, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_54, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
        return FALSE;
    }
    if (!user_dat.SMTP_SERVER) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_55, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_54, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
        return FALSE;
    }
    if (!user_dat.usb_id_selection[0] || !user_dat.usb_id_selection[1]) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_56, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_54, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
        return FALSE;
    }
    if (!user_dat.pass) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_57, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_54, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
        return FALSE;
    }

    HANDLE u2mMainThread;
    UINT thrdID;

    Logging_System_Enabled = TRUE;

    //find if there are already other U2M log files in the folder and increase the global filename number accordingly
    if (curr_filename == 1) {
        while (1) {
            TCHAR temp_fn[255];
            WIN32_FIND_DATA u2m_log_data;
            LARGE_INTEGER Logfile_sz;

            StringCchPrintf(temp_fn, 255, U2MLOG_PREFIX _T("_%ld.txt"), curr_filename + 1);
            HANDLE curr_u2m_log = FindFirstFile(temp_fn, &u2m_log_data);

            //if we found a U2M log file with the next to current filename
            if (curr_u2m_log != INVALID_HANDLE_VALUE) {
                putchar('s');
                //if this function fails break
                if (!GetFileSizeEx(curr_u2m_log, &Logfile_sz)) {
                    CloseHandle(curr_u2m_log);
                    break;
                }
                //if the next logfile size is bigger than our max size
                if (Logfile_sz.QuadPart > MAX_LOG_FILE_SZ) {
                    CloseHandle(curr_u2m_log);
                    curr_filename++;
                    continue;
                }
            }
            break;
        }
    }

    //if we exceed the maximum number of allowed U2M logs then the logging is disabled
    if (curr_filename - 1 >= MAX_NUMBER_OF_U2M_LOGS) {
        Logging_System_Enabled = FALSE;
    }

    u2mMainThread = (HANDLE)_beginthreadex(NULL, 0, U2MThreadSingle, (LPVOID)hwnd, 0, &thrdID);
    (VOID)u2mMainThread;

    return TRUE;
}

/*sends a single e-mail when the device is detected and waits
 *it's removed*/
UINT CALLBACK U2MThreadSingle(LPVOID dat)
{
    HWND hwnd ATTRIB_UNUSED = (HWND)dat;
    UINT failed_emails = 0;

    while (onoff && (failed_emails <= user_dat.MAX_FAILED_EMAILS)) {
        Sleep((DWORD)user_dat.TIMEOUT);
        if (!onoff) break;

        if (GetConnectedUSBDevs(NULL, IS_USB_CONNECTED)) {
            if (Logging_System_Enabled == TRUE) {
                WriteToU2MLogFile(U2MLOG_PREFIX);
            }

            SendMessageTimeout(hwnd, WM_ENABLE_STARTSTOP, 
                               (WPARAM)0, (LPARAM)0, SMTO_NORMAL, 0, NULL);
            if (!SendEmail()) {
                failed_emails++;
            }
            SendMessageTimeout(hwnd, WM_ENABLE_STARTSTOP, 
                               (WPARAM)0, (LPARAM)0, SMTO_NORMAL, 0, NULL);

            if (failed_emails > user_dat.MAX_FAILED_EMAILS) break; //bad logic but it works

            while (GetConnectedUSBDevs(NULL, IS_USB_CONNECTED)) {
                Sleep(900);
                if (!onoff) 
                    return 0; //if onoff is FALSE, it means that the STARTSTOP button has
            }                 //been already pushed and there's no need to risk sending the 
        }                     //message in line 104, thus disabling the button
    }

    if (failed_emails > user_dat.MAX_FAILED_EMAILS) 
        SendMessageTimeout(hwnd, WM_COMMAND, 
                           MAKEWPARAM((WORD)IDC_STARTSTOP, 0), 
                           (LPARAM)0, SMTO_NORMAL, 0, NULL);
    return 0;
}

/*sends e-mails nonstop before the device is removed
UINT CALLBACK U2MThreadMulti(LPVOID dat)
{
    HWND hwnd ATTRIB_UNUSED = (HWND)dat;
    UINT failed_emails = 0;

    while (onoff && (failed_emails <= user_dat.MAX_FAILED_EMAILS)) {
        Sleep((DWORD)user_dat.TIMEOUT);
        if (!onoff) break;
        if (GetConnectedUSBDevs(NULL, IS_USB_CONNECTED)) {
            SendMessageTimeout(hwnd, WM_ENABLE_STARTSTOP, 
                               (WPARAM)0, (LPARAM)0, SMTO_NORMAL, 0, NULL);
            if (!SendEmail()) failed_emails++;
            SendMessageTimeout(hwnd, WM_ENABLE_STARTSTOP, 
                               (WPARAM)0, (LPARAM)0, SMTO_NORMAL, 0, NULL);
        }
    }
    if (failed_emails > user_dat.MAX_FAILED_EMAILS) 
        SendMessageTimeout(hwnd, WM_COMMAND, 
                           MAKEWPARAM((WORD)IDC_STARTSTOP, 0), 
                           (LPARAM)0, SMTO_NORMAL, 0, NULL);
    return 0;
}*/

BOOL SendEmail(VOID)
{
    BOOL retvalue = TRUE;
    quickmail_initialize();
    quickmail mailobj = quickmail_create(user_dat.FROM, user_dat.SUBJECT);

    quickmail_add_to(mailobj, user_dat.TO);

    if (user_dat.CC)
        quickmail_add_cc(mailobj, user_dat.CC);

    quickmail_add_header(mailobj, "Importance: High");
    /*quickmail_add_header(mailobj, "X-Priority: 5");
    quickmail_add_header(mailobj, "X-MSMail-Priority: Low");*/
    quickmail_set_body(mailobj, user_dat.BODY);

    const char* errmsg;
#ifdef DEBUG
    quickmail_set_debug_log(mailobj, stderr);
#endif
    if ((errmsg = quickmail_send(mailobj, user_dat.SMTP_SERVER, user_dat.PORT, user_dat.FROM, user_dat.pass)) != NULL) {
        retvalue = FALSE;
    }
    quickmail_destroy(mailobj);
    return retvalue;
}

BOOL WriteToU2MLogFile(TCHAR *Logfile_name)
{
    HANDLE hLogfile;
    DWORD dwRet;
    SYSTEMTIME curr_time;
    TCHAR to_write[255], filename[255];
    size_t to_write_len;
    OVERLAPPED log_inf = {.Offset = 0xffffffff, .OffsetHigh = 0xffffffff}; //to write to the end of the file
    LARGE_INTEGER Logfile_sz; //the size of the current file

    GetLocalTime(&curr_time);
    StringCchPrintf(filename, 255, _T("%s_%ld.txt"), Logfile_name, curr_filename);
    hLogfile = CreateFile(filename, FILE_GENERIC_WRITE, FILE_SHARE_WRITE, 
                          NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hLogfile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    //if we failed to get the size of the file, write to it anyway
    if (!GetFileSizeEx(hLogfile, &Logfile_sz)) goto WRITE_TO_LOG;
    //else if the size of the file is bigger than MAX_LOG_FILE_SZ we write to a new file
    if (Logfile_sz.QuadPart > MAX_LOG_FILE_SZ) {
        CloseHandle(hLogfile);
        curr_filename++;
        return WriteToU2MLogFile(Logfile_name);
    }

WRITE_TO_LOG:
    StringCchPrintf(to_write, 255,
                    _T("-- DAY:%02d, MONTH:%02d, YEAR:%d, \tHOUR:%02d, MINUTE:%02d, SECONDS:%02d, MILLISECONDS:%04d --\r\n\r\n"), 
                    curr_time.wDay, curr_time.wMonth, curr_time.wYear, curr_time.wHour, 
                    curr_time.wMinute, curr_time.wSecond, curr_time.wMilliseconds);
    StringCbLength(to_write, 255, &to_write_len);
    WriteFile(hLogfile, to_write, to_write_len, &dwRet, &log_inf);
    CloseHandle(hLogfile);

    return TRUE;
}

BOOL GetDevIDs(ULONG *vid, ULONG *pid, TCHAR *devpath)
{
    if (devpath == NULL)  return FALSE;
    /* precaution to check if devicepath actually has vid and pid stored */
    if (devpath[8] == 'v' && devpath[9] == 'i' && devpath[10] == 'd') {
        TCHAR temp[5];

        temp[4] = '\0';
        temp[0] = devpath[12];
        temp[1] = devpath[13];
        temp[2] = devpath[14];
        temp[3] = devpath[15];
        *vid = _tcstoul(temp, NULL, 16);
        temp[0] = devpath[21];
        temp[1] = devpath[22];
        temp[2] = devpath[23];
        temp[3] = devpath[24];
        *pid = _tcstoul(temp, NULL, 16);
        return TRUE;
    }
    return FALSE;
}

BOOL GetConnectedUSBDevs(HWND hDlg, USHORT flag)
{
    HDEVINFO hUSBDevInfo, hUSBHUBInfo;
    SP_DEVICE_INTERFACE_DATA DevIntfData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
    SP_DEVINFO_DATA DevData;
    DWORD dwSize, dwMemberIdx;
    UINT idx = 0;
    ULONG vID, dID;

    (VOID)hUSBHUBInfo;
    hUSBDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, 
               NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE);

    if (hUSBDevInfo == INVALID_HANDLE_VALUE) {
#ifdef DEBUG
        __MsgBoxGetLastError(_T("hDevInfo"));
#endif
        return FALSE;
    }

    DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    dwMemberIdx = 0;
    if (!SetupDiEnumDeviceInterfaces(hUSBDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE,
         dwMemberIdx, &DevIntfData)) {
#ifdef DEBUG
        __MsgBoxGetLastError(_T("SetupDiEnumDeviceInterfaces() 1st call"));
#endif
        return FALSE;
    }

    while (GetLastError() != ERROR_NO_MORE_ITEMS) {
        DevData.cbSize = sizeof(DevData);
        SetupDiGetDeviceInterfaceDetail(hUSBDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

        DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(dwSize);
        DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (SetupDiGetDeviceInterfaceDetail(hUSBDevInfo, &DevIntfData,
            DevIntfDetailData, dwSize, &dwSize, &DevData)) {

            if (!GetDevIDs(&vID, &dID, DevIntfDetailData->DevicePath)) goto SKIP_DEVICE;

            switch (flag) {
                case FILL_USB_LISTVIEW:
                    if (hDlg != NULL) {
                        UsbDevStruct *new = UsbFind(vID, dID);
                        if (!new) goto SKIP_DEVICE;
                        scanned_usb_ids[idx][0] = vID;
                        scanned_usb_ids[idx][1] = dID;
                        AddDeviceToUSBListView(hDlg, new->Device, new->Vendor);
                        idx++;
                    }
                    break;
                case IS_USB_CONNECTED:
                    if ((user_dat.usb_id_selection[0] == vID && user_dat.usb_id_selection[1] == dID) ||
                        (user_dat.usb_id_selection[0] == vID && user_dat.usb_id_selection[1] == 0xabcd)) {
                        free(DevIntfDetailData);
                        SetupDiEnumDeviceInterfaces(hUSBDevInfo, NULL, 
                                                    &GUID_DEVINTERFACE_USB_DEVICE, 
                                                    ++dwMemberIdx, &DevIntfData);
                        SetupDiDestroyDeviceInfoList(hUSBDevInfo);
                        return TRUE;
                    }
                    break;
                default:
                    break;
            }
        } else {
#ifdef DEBUG
            __MsgBoxGetLastError(_T("SetupDiGetDeviceInterfaceDetail()"));
#endif
            free(DevIntfDetailData);
            SetupDiDestroyDeviceInfoList(hUSBDevInfo);
            return FALSE;
        }
SKIP_DEVICE:
        SetupDiEnumDeviceInterfaces(hUSBDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, 
             ++dwMemberIdx, &DevIntfData);
        free(DevIntfDetailData);
    }
#ifdef DEBUG
    if (!SetupDiDestroyDeviceInfoList(hUSBDevInfo)) {
        __MsgBoxGetLastError(_T("SetupDiDestroyDeviceInfoList()"));
    }
#else
    SetupDiDestroyDeviceInfoList(hUSBDevInfo);
#endif
    return FALSE;
}
