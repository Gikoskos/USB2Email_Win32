/******************************************
*               U2MModule.c                *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "U2MWin32.h"
#include "usb_ids.h"
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>
#include <regstr.h>
#include <quickmail.h>


#ifdef DEFINE_GUID
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
            0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
#else
#error DEFINE_GUID is not defined.
#endif



/*** Globals ***/
HANDLE u2mMainThread;
UINT *thrdID;
UINT scanned_usb_ids[MAX_CONNECTED_USB][2];


/*******************************************
* Prototypes for functions with local scope *
 *******************************************/
BOOL USBisConnected();
UINT CALLBACK U2MThread(LPVOID dat);
BOOL SendEmail(VOID);
int cmp(const void *vp, const void *vq);
UsbDevStruct *find(unsigned long vendor, unsigned long device);
BOOL GetDevIDs(USHORT *vid, USHORT *pid, char *devpath);


BOOL InitU2MThread(HWND hwnd)
{
    if (!FROM) {
        MessageBox(hwnd, "You have to set an e-mail to send, first.", 
                   "Can't start service!", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    if (!SMTP_SERVER) {
        MessageBox(hwnd, "SMTP server domain not set.", "Can't start service!", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    if (!usb_id_selection[0] || !usb_id_selection[1]) {
        MessageBox(hwnd, "No USB device selected.", "Can't start service!", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    if (!pass) {
        MessageBox(hwnd, "No password set.", "Can't start service!", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    onoff = TRUE;
    u2mMainThread = (HANDLE)_beginthreadex(NULL, 0, U2MThread, (LPVOID)hwnd, 0, thrdID);

    return TRUE;
}

UINT CALLBACK U2MThread(LPVOID dat)
{
    HWND hwnd = (HWND)dat;

    while (onoff) {
        Sleep((DWORD)TIMEOUT);
        if (ConnectedUSBDevs(NULL, IS_USB_CONNECTED)) {
            BOOL ret = SendEmail();
            if (!ret) {
                MessageBox(hwnd, "Failed to send the e-mail.", 
                           "Something happened!", MB_ICONERROR | MB_OK);
                SendMessageTimeout(HWND_BROADCAST, 
                                   WM_COMMAND, 
                                   MAKEWPARAM((WORD)IDC_STARTSTOP, 0), 
                                   (LPARAM)0,
                                   SMTO_NORMAL,
                                   0,
                                   NULL);
                break;
            }
        }
    }
    return 0;
}

BOOL SendEmail(VOID)
{
    BOOL retvalue = TRUE;
    quickmail_initialize();
    quickmail mailobj = quickmail_create(FROM, SUBJECT);

    quickmail_add_to(mailobj, TO);

    if (CC)
        quickmail_add_cc(mailobj, CC);


    quickmail_add_header(mailobj, "Importance: Low");
    quickmail_add_header(mailobj, "X-Priority: 5");
    quickmail_add_header(mailobj, "X-MSMail-Priority: Low");
    quickmail_set_body(mailobj, BODY);

    const char* errmsg;
#ifdef DEBUG
    quickmail_set_debug_log(mailobj, stderr);
#endif
    if ((errmsg = quickmail_send(mailobj, SMTP_SERVER, PORT, FROM, pass)) != NULL) {
        fprintf(stderr, "Error sending e-mail: %s\n", errmsg);
        retvalue = FALSE;
    }
    quickmail_destroy(mailobj);
    return retvalue;
}

BOOL GetDevIDs(USHORT *vid, USHORT *pid, char *devpath)
{
    /* precaution to check if devicepath actually has vid and pid stored */
    if (devpath[8] == 'v' && devpath[9] == 'i' && devpath[10] == 'd') {
        char temp[5];
        temp[4] = '\0';
        temp[0] = devpath[12];
        temp[1] = devpath[13];
        temp[2] = devpath[14];
        temp[3] = devpath[15];
        *vid = (USHORT)strtoul(temp, NULL, 16);
        temp[0] = devpath[21];
        temp[1] = devpath[22];
        temp[2] = devpath[23];
        temp[3] = devpath[24];
        *pid = (USHORT)strtoul(temp, NULL, 16);

        return TRUE;
    }
    return FALSE;
}

BOOL ConnectedUSBDevs(HWND hDlg, USHORT flag)
{
    HDEVINFO                         hDevInfo;
    SP_DEVICE_INTERFACE_DATA         DevIntfData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
    SP_DEVINFO_DATA                  DevData;
    DWORD dwSize, dwMemberIdx;
    UINT idx = 0;
    USHORT vID, dID;

    hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, 
        NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (hDevInfo != INVALID_HANDLE_VALUE) {
        DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        dwMemberIdx = 0;
        SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE,
                                    dwMemberIdx, &DevIntfData);

        while (GetLastError() != ERROR_NO_MORE_ITEMS) {
            DevData.cbSize = sizeof(DevData);
            SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);
            DevIntfDetailData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
            DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData,
                DevIntfDetailData, dwSize, &dwSize, &DevData)) {

                if (!GetDevIDs(&vID, &dID, DevIntfDetailData->DevicePath)) goto SKIP_DEVICE;

                switch (flag) {
                    case FILL_USB_LISTVIEW:
                        if (hDlg != NULL) {
                            UsbDevStruct *new = UsbFind((unsigned long)vID, (unsigned long)dID);
                            scanned_usb_ids[idx][0] = vID;
                            scanned_usb_ids[idx][1] = dID;
                            AddDeviceToUSBListView(hDlg, new->Device, new->Vendor);
                        }
                        break;
                    case IS_USB_CONNECTED:
                        if ((usb_id_selection[0] == vID && usb_id_selection[1] == dID) ||
                            (usb_id_selection[0] == vID && usb_id_selection[1] == 0xabcd)) {
                            HeapFree(GetProcessHeap(), 0, DevIntfDetailData);
                            SetupDiEnumDeviceInterfaces(hDevInfo, NULL, 
                                                        &GUID_DEVINTERFACE_USB_DEVICE, 
                                                        ++dwMemberIdx, &DevIntfData);
                            SetupDiDestroyDeviceInfoList(hDevInfo);
                            return TRUE;
                        }
                        break;
                }
                idx++;
            }
SKIP_DEVICE:
            HeapFree(GetProcessHeap(), 0, DevIntfDetailData);
            SetupDiEnumDeviceInterfaces(hDevInfo, NULL, 
                                        &GUID_DEVINTERFACE_USB_DEVICE, ++dwMemberIdx, &DevIntfData);
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    return FALSE;
}
