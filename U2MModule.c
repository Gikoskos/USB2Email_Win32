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
#include <process.h>
#include <quickmail.h>
#include <curl/curl.h>


#ifdef DEFINE_GUID
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
            0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
#else
#error DEFINE_GUID is not defined.
#endif



/*** Globals ***/
HANDLE u2mMainThread;
BOOL RUNNING = FALSE;
UINT *thrdID;
UINT scanned_usb_ids[MAX_CONNECTED_USB][2];


/*******************************************
* Prototypes for functions with local scope *
 *******************************************/
BOOL USBisConnected();
UINT CALLBACK U2MThread(LPVOID PTR_TIMEOUT);
BOOL SendEmail(VOID);
int cmp(const void *vp, const void *vq);
UsbDevStruct *find(unsigned long vendor, unsigned long device);
BOOL GetDevIDs(USHORT *vid, USHORT *pid, char *devpath);


BOOL InitU2MThread()
{
    if (!FROM) {
        MessageBox(NULL, "You have to set an e-mail to send, first.", "Can't start service!", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    if (!SMTP_SERVER) {
        MessageBox(NULL, "SMTP server domain and port aren't set.", "Can't start service!", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    if (usb_id_selection[0] && usb_id_selection[1]) {
        MessageBox(NULL, "No USB device selected.", "Can't start service!", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    if (!pass) {
        MessageBox(NULL, "No password set.", "Can't start service!", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    onoff = TRUE;
    u2mMainThread = (HANDLE)_beginthreadex(NULL, 0, U2MThread, (LPVOID)&TIMEOUT, 0, thrdID);

    return TRUE;
}

UINT CALLBACK U2MThread(LPVOID PTR_TIMEOUT)
{
    UINT timeout = *((UINT*)PTR_TIMEOUT);

    while (onoff) {
        RUNNING = TRUE;
        if (ConnectedUSBDevs(NULL, IS_USB_CONNECTED)) {
            BOOL ret = SendEmail();
            RUNNING = FALSE;
            if (!ret) {
                onoff = FALSE;
                return 0;
            }
            if (EMAIL_PAUSE) {
                Sleep(EMAIL_PAUSE*1000);
            }
        }
        if (onoff) {
            Sleep(timeout);
        } else {
            return 0;
        }
    }
    return 0;
}

BOOL SendEmail(VOID)
{
    BOOL retvalue = TRUE;
    quickmail_initialize();
    quickmail mailobj = quickmail_create(FROM, "Alarm e-mail");

    quickmail_add_to(mailobj, TO);

    if (CC)
        quickmail_add_cc(mailobj, CC);


    quickmail_add_header(mailobj, "Importance: Low");
    quickmail_add_header(mailobj, "X-Priority: 5");
    quickmail_add_header(mailobj, "X-MSMail-Priority: Low");
    quickmail_set_body(mailobj, "This is a test e-mail.");
    //quickmail_add_body_memory(mailobj, NULL, "This is a test e-mail.\nThis mail was sent using libquickmail.", 64, 0);
    //quickmail_add_body_memory(mailobj, "text/html", "This is a <b>test</b> e-mail.<br/>\nThis mail was sent using <u>libquickmail</u>.", 80, 0);

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

    DeleteScannedUSBIDs();
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
                        if (usb_id_selection[0] == vID && usb_id_selection[1] == dID) {
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
