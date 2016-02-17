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
BOOL USBisConnected(char *to_find);
UINT __stdcall U2MThread(LPVOID PTR_TIMEOUT);
BOOL SendEmail(VOID);
int cmp(const void *vp, const void *vq);
UsbDevStruct *find(unsigned long vendor, unsigned long device);


BOOL InitU2MThread()
{
	if (!FROM) {
		MessageBox(NULL, "You have to set an e-mail to send, first.", "Can't start service!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	/*if (!SMTP_STR) {
		MessageBox(NULL, "SMTP server domain and port aren't set.", "Can't start service!", MB_ICONERROR | MB_OK);
		return FALSE;
	}*/
	/*if (!USBdev) {
		MessageBox(NULL, "No USB device selected.", "Can't start service!", MB_ICONERROR | MB_OK);
		return FALSE;
	}*/
	if (!pass) {
		MessageBox(NULL, "No password set.", "Can't start service!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	onoff = TRUE;
	u2mMainThread = (HANDLE)_beginthreadex(NULL, 0, U2MThread, (LPVOID)&TIMEOUT, 0, thrdID);

	return TRUE;
}

UINT __stdcall U2MThread(LPVOID PTR_TIMEOUT)
{
	UINT timeout = *((UINT*)PTR_TIMEOUT);
	int i = 0;
	while (onoff) {
		RUNNING = TRUE;
		if (i++ < 3) {
			int ret = SendEmail();
			RUNNING = FALSE;
			if (!ret) {
				onoff = FALSE;
				return 0;
			}
			if (EMAIL_PAUSE) {
				Sleep(EMAIL_PAUSE*1000);
			}
		}
		RUNNING = FALSE;
		break;
		if (onoff) {
			Sleep(timeout);
		} else {
			return 0;
		}
	}
	return 0;
}

BOOL USBisConnected(char *to_find)
{
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD i;
	UINT len = strlen(to_find);

	hDevInfo = SetupDiGetClassDevs(NULL, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES );

	if (hDevInfo == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
		DWORD DataT;
		char *buffer = NULL;
		DWORD buffersize = 0;

	   while (!SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData,
			SPDRP_DEVICEDESC, &DataT, (PBYTE)buffer, buffersize, &buffersize)) {
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				if (buffer) LocalFree(buffer);
				buffer = NULL;
				buffer = LocalAlloc(LPTR,buffersize * 2);
			} else {
				break;
			}
		}
		if (buffer) {
			if (!strncmp(buffer, to_find, len)) {
				LocalFree(buffer);
				return TRUE;
			}
			LocalFree(buffer);
		}
	}
	if (GetLastError() != NO_ERROR && GetLastError() != ERROR_NO_MORE_ITEMS) {
		return FALSE;
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return FALSE;
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

VOID fillUSBlist(HWND hDlg)
{
	HDEVINFO                         hDevInfo;
	SP_DEVICE_INTERFACE_DATA         DevIntfData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
	SP_DEVINFO_DATA                  DevData;
	DWORD dwSize, dwMemberIdx;
	char temp[5];
	UINT idx = 0;
	unsigned short vID, dID;

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
				temp[0] = DevIntfDetailData->DevicePath[12];
				temp[1] = DevIntfDetailData->DevicePath[13];
				temp[2] = DevIntfDetailData->DevicePath[14];
				temp[3] = DevIntfDetailData->DevicePath[15];
				temp[4] = '\0';
				vID = (unsigned short)strtoul(temp, NULL, 16);
				temp[0] = DevIntfDetailData->DevicePath[21];
				temp[1] = DevIntfDetailData->DevicePath[22];
				temp[2] = DevIntfDetailData->DevicePath[23];
				temp[3] = DevIntfDetailData->DevicePath[24];
				dID = (unsigned short)strtoul(temp, NULL, 16);
				UsbDevStruct *new = UsbFind((unsigned long)vID, (unsigned long)dID);
				scanned_usb_ids[idx][0] = vID;
				scanned_usb_ids[idx][1] = dID;
				AddDeviceToUSBListView(hDlg, new->Device, new->Vendor);
			}
			HeapFree(GetProcessHeap(), 0, DevIntfDetailData);
			SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, ++dwMemberIdx, &DevIntfData);
			idx++;
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
}
