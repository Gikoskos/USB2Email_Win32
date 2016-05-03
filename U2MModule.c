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
#include "quickmail.h"


/*#ifdef DEFINE_GUID
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
            0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
#else
#error DEFINE_GUID is not defined.
#endif*/

typedef struct thread_args {
    user_input_data usr;
    HWND hwnd;
    DWORD thrdID;
} thread_args;


//the prefix of every U2M log's file name
#define U2MLOG_PREFIX TEXT("Logs\\U2MLog")
//maximum number of U2M log files that we're allowed to save on the disk
#define MAX_NUMBER_OF_U2M_LOGS  10000

/*** Globals ***/
ULONG scanned_usb_ids[MAX_CONNECTED_USB][2];
HANDLE u2mMainThread;

static SRWLOCK U2Msrw_lock = SRWLOCK_INIT;
static ULONG curr_filename = 1; //current log file number
static LONG Logging_System_Enabled = TRUE;
/* pointer to the thread arguments
 * malloc'd and filled with data before the thread is created and deleted */
static thread_args *tmp = NULL;


/*******************************************
* Prototypes for functions with local scope *
 *******************************************/
BOOL USBisConnected();
UINT CALLBACK U2MThreadSingle(LPVOID dat);
//UINT CALLBACK U2MThreadMulti(LPVOID dat);
BOOL SendEmail(user_input_data *user_dat, char *errmsg_out);
UsbDevStruct *find(unsigned long vendor, unsigned long device);
BOOL GetDevIDs(ULONG *vid, ULONG *pid, TCHAR *devpath);
BOOL WriteToU2MLogFile(TCHAR *Logfile_name);
VOID InitU2MLogging(VOID);
VOID EnableU2MLogging(BOOL new_state);
BOOL U2MLoggingIsEnabled(VOID);


BOOL InitU2MThread(user_input_data *user_dat, HWND hwnd)
{
    if (onoff == TRUE) return FALSE;

    UINT u2mthrdID;

    if (!user_dat->FROM) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_53, ID_ERR_MSG_54, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (!user_dat->SMTP_SERVER) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_55, ID_ERR_MSG_54, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (!user_dat->usb_id_selection[0] || !user_dat->usb_id_selection[1]) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_56, ID_ERR_MSG_54, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (!user_dat->pass) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_57, ID_ERR_MSG_54, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    FreeModuleHeap();

    tmp = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(thread_args));

    tmp->usr = *user_dat;
    tmp->hwnd = hwnd;
    tmp->thrdID = GetThreadId(GetCurrentThread());

    if (u2mMainThread) CloseHandle(u2mMainThread);

    u2mMainThread = (HANDLE)_beginthreadex(NULL, 0, U2MThreadSingle, (LPVOID)tmp, 0, &u2mthrdID);
    return TRUE;
}

/*sends a single e-mail when the device is detected and waits
 *it's removed*/
UINT CALLBACK U2MThreadSingle(LPVOID dat)
{
    thread_args *args = (thread_args *)dat;
    UINT failed_emails = 0;
    char email_err_buff[255];

    email_err_buff[0] = '\0';
    if (args->hwnd == NULL) {
        _endthreadex(1);
        return 1;
    }

    while ((WaitForSingleObject(u2m_StartStop_event, (DWORD)args->usr.TIMEOUT) == WAIT_TIMEOUT) &&
           (failed_emails <= args->usr.MAX_FAILED_EMAILS)) {
        if (GetConnectedUSBDevs(NULL, args->usr.usb_id_selection[0],
                                args->usr.usb_id_selection[1], IS_USB_CONNECTED)) {
            if (U2MLoggingIsEnabled()) {
                WriteToU2MLogFile(U2MLOG_PREFIX);
            }

            //disable the StartStop button while the e-mail is being sent, by sending
            //a custom message to the main window
            SendMessageTimeout(args->hwnd, WM_ENABLE_STARTSTOP,
                               (WPARAM)0, (LPARAM)0, SMTO_NORMAL, 0, NULL);
            if (SendEmail(&(args->usr), email_err_buff) == FALSE) {
                failed_emails++;
            }
            SendMessageTimeout(args->hwnd, WM_ENABLE_STARTSTOP,
                               (WPARAM)0, (LPARAM)0, SMTO_NORMAL, 0, NULL);
            //if the maximum number of failed e-mails has been reached at this pointer
            //there's no need to enter the second loop; the thread exits
            if (failed_emails > args->usr.MAX_FAILED_EMAILS) {
                if (email_err_buff[0]) {
                    MessageBoxA(args->hwnd, email_err_buff, "Failed sending the e-mail!", MB_OK | MB_ICONERROR);
                }
                break;
            }

            while (GetConnectedUSBDevs(NULL, args->usr.usb_id_selection[0],
                   args->usr.usb_id_selection[1], IS_USB_CONNECTED)) {
                if (WaitForSingleObject(u2m_StartStop_event, 200) != WAIT_TIMEOUT) {
                    _endthreadex(0);
                    return 0; //if u2m_StartStop_event is signaled, it means that the STARTSTOP button has
                }             //been already pushed and there's no need to potentially send the
            }                 //message in line 143
        }
    }

    //InterlockedDecrement(&Logging_System_Enabled);
    if (failed_emails > args->usr.MAX_FAILED_EMAILS) {
        SendMessageTimeout(args->hwnd, WM_STARTSTOP_CONTROL,
                           (WPARAM)0, (LPARAM)0, SMTO_NORMAL, 0, NULL);
    }
    _endthreadex(0);
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

BOOL SendEmail(user_input_data *user_dat, char *errmsg_out)
{
    BOOL retvalue = TRUE;
    quickmail mailobj = quickmail_create(user_dat->FROM, user_dat->SUBJECT);

    quickmail_add_to(mailobj, user_dat->TO);

    if (user_dat->CC)
        quickmail_add_cc(mailobj, user_dat->CC);

    quickmail_add_header(mailobj, "Importance: High");
    /*quickmail_add_header(mailobj, "X-Priority: 5");
    quickmail_add_header(mailobj, "X-MSMail-Priority: Low");*/
    quickmail_set_body(mailobj, user_dat->BODY);

    const char* errmsg;
#ifdef DEBUG
    quickmail_set_debug_log(mailobj, stderr);
#endif
    if (user_dat->PORT == 465) { //smtps
        if ((errmsg = quickmail_send_secure(mailobj, user_dat->SMTP_SERVER, user_dat->PORT,
                                            user_dat->FROM, user_dat->pass)) != NULL) {
            retvalue = FALSE;
        }
    } else if ((errmsg = quickmail_send(mailobj, user_dat->SMTP_SERVER, user_dat->PORT, user_dat->FROM, user_dat->pass)) != NULL) {
        retvalue = FALSE;
    }

    if (errmsg && errmsg_out) {
        if (FAILED(StringCchCopyNA(errmsg_out, 255, errmsg, (size_t)lstrlenA(errmsg)))) {
            errmsg_out[0] = '\0';
        }
    }
    quickmail_destroy(mailobj);
    return retvalue;
}

BOOL U2MLoggingIsEnabled(VOID)
{
    BOOL retvalue;

    AcquireSRWLockShared(&U2Msrw_lock);
    retvalue = Logging_System_Enabled;
    ReleaseSRWLockShared(&U2Msrw_lock);
    return retvalue;
}

VOID EnableU2MLogging(BOOL new_state)
{
    AcquireSRWLockExclusive(&U2Msrw_lock);
    Logging_System_Enabled = new_state;
    ReleaseSRWLockExclusive(&U2Msrw_lock);
}

VOID FreeModuleHeap(VOID)
{
    if (tmp != (thread_args*)NULL) {
        HeapFree(GetProcessHeap(), 0, tmp);
        tmp = NULL;
    }
}

VOID InitU2MLogging(VOID)
{
    EnableU2MLogging(TRUE);

    //find if there are already other U2M log files in the folder and increase the global filename number accordingly
    if (curr_filename == 1) {
        while (1) {
            TCHAR temp_fn[255];
            WIN32_FIND_DATA u2m_log_data;
            LARGE_INTEGER Logfile_sz;

            StringCchPrintf(temp_fn, 255, U2MLOG_PREFIX TEXT("_%ld.txt"), curr_filename + 1);
            HANDLE curr_u2m_log = FindFirstFile(temp_fn, &u2m_log_data);

            //if we found a U2M log file with the next to current filename
            if (curr_u2m_log != INVALID_HANDLE_VALUE) {
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
        EnableU2MLogging(FALSE);
    } else {
        CreateDirectory(TEXT("Logs"), NULL);
    }
}

BOOL WriteToU2MLogFile(TCHAR *Logfile_name)
{
#define MAX_TOWRITE 255
    HANDLE hLogfile;
    DWORD dwRet;
    SYSTEMTIME curr_time;
    TCHAR to_write[MAX_TOWRITE], filename[255];
    size_t to_write_len;
    OVERLAPPED log_inf = {.Offset = 0xffffffff, .OffsetHigh = 0xffffffff}; //to write to the end of the file
    LARGE_INTEGER Logfile_sz; //the size of the current file

    GetLocalTime(&curr_time);
    StringCchPrintf(filename, 255, TEXT("%s_%ld.txt"), Logfile_name, curr_filename);
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
    StringCchPrintf(to_write, MAX_TOWRITE,
                    TEXT("-- %02d:%02d:%02d.%04d\tDAY:%02d, MONTH:%02d, YEAR:%d --\r\n\r\n"),
                    curr_time.wDay, curr_time.wMonth, curr_time.wYear, curr_time.wHour,
                    curr_time.wMinute, curr_time.wSecond, curr_time.wMilliseconds);
    StringCbLength(to_write, MAX_TOWRITE, &to_write_len);
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

BOOL GetConnectedUSBDevs(HWND hDlg, ULONG VendorID, ULONG ProductID, USHORT flag)
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
        __MsgBoxGetLastError(hDlg, TEXT("hDevInfo"), __LINE__);
#endif
        return FALSE;
    }

    DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    dwMemberIdx = 0;
    if (!SetupDiEnumDeviceInterfaces(hUSBDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE,
         dwMemberIdx, &DevIntfData)) {
#ifdef DEBUG
        __MsgBoxGetLastError(hDlg, TEXT("SetupDiEnumDeviceInterfaces()"), __LINE__);
#endif
        return FALSE;
    }

    while (GetLastError() != ERROR_NO_MORE_ITEMS) {
        DevData.cbSize = sizeof(DevData);
        SetupDiGetDeviceInterfaceDetail(hUSBDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

        DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, dwSize);
        DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (SetupDiGetDeviceInterfaceDetail(hUSBDevInfo, &DevIntfData,
            DevIntfDetailData, dwSize, &dwSize, &DevData)) {

            if (!GetDevIDs(&vID, &dID, DevIntfDetailData->DevicePath)) goto SKIP_DEVICE;

            switch (flag) {
                case FILL_USB_LISTVIEW:
                    if (hDlg != NULL) {
                        UsbDevStruct *new = UsbFind(vID, dID);

                        scanned_usb_ids[idx][0] = vID;
                        scanned_usb_ids[idx][1] = dID;
                        // if the device isn't found in the USB list
                        // then we will show a localized 'Unknown device/vendor' string
                        // with the device's ID
                        if (new == NULL) {
                            //first, we create the device string in the form of "Unknown device(0xabcd)"
                            TCHAR device_to_add[255];
                            TCHAR devIDstr[] = {
                                TEXT('('), TEXT('0'), TEXT('x'),
                                DevIntfDetailData->DevicePath[21],
                                DevIntfDetailData->DevicePath[22],
                                DevIntfDetailData->DevicePath[23],
                                DevIntfDetailData->DevicePath[24],
                                TEXT(')'), TEXT('\0')
                            };

                            LoadLocaleErrMsg(device_to_add, 33);
                            StringCchCat(device_to_add, sizeof(device_to_add) / sizeof(device_to_add[0]), devIDstr);

                            //then we look to see if the usb list has the vendor's name, at least, because a normal user
                            //won't be able to identify easily their USB device from its IDs. Any kind of identification
                            //at this point can be useful.
                            new = UsbFind(vID, 0xabcd);
                            if (!new) {
                                //if we didn't find any vendor, we default to the
                                //Unknown device/vendor strings
                                TCHAR vendor_to_add[255];
                                TCHAR venIDstr[] = {
                                    TEXT('('), TEXT('0'), TEXT('x'),
                                    DevIntfDetailData->DevicePath[12],
                                    DevIntfDetailData->DevicePath[13],
                                    DevIntfDetailData->DevicePath[14],
                                    DevIntfDetailData->DevicePath[15],
                                    TEXT(')'), TEXT('\0')
                                };

                                LoadLocaleErrMsg(vendor_to_add, 32);
                                StringCchCat(vendor_to_add, sizeof(vendor_to_add) / sizeof(vendor_to_add[0]), venIDstr);
                                AddDeviceToUSBListView(hDlg, device_to_add, vendor_to_add);
                            } else {
                                //if we found a vendor then we send the strings to the USB list view
                                AddDeviceToUSBListView(hDlg, device_to_add, new->Vendor);
                            }
                        } else {
                            AddDeviceToUSBListView(hDlg, new->Device, new->Vendor);
                        }

                        idx++;
                    }
                    break;
                case IS_USB_CONNECTED:
                    if ((VendorID == vID && ProductID == dID) || (VendorID == vID && ProductID == 0xabcd)) {
                        HeapFree(GetProcessHeap(), 0, DevIntfDetailData);
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
            __MsgBoxGetLastError(hDlg, TEXT("SetupDiGetDeviceInterfaceDetail()"), __LINE__);
#endif
            HeapFree(GetProcessHeap(), 0, DevIntfDetailData);
            SetupDiDestroyDeviceInfoList(hUSBDevInfo);
            return FALSE;
        }
SKIP_DEVICE:
        SetupDiEnumDeviceInterfaces(hUSBDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE,
             ++dwMemberIdx, &DevIntfData);
        HeapFree(GetProcessHeap(), 0, DevIntfDetailData);
    }
#ifdef DEBUG
    if (!SetupDiDestroyDeviceInfoList(hUSBDevInfo)) {
        __MsgBoxGetLastError(hDlg, TEXT("SetupDiDestroyDeviceInfoList()"), __LINE__);
    }
#else
    SetupDiDestroyDeviceInfoList(hUSBDevInfo);
#endif
    return FALSE;
}
