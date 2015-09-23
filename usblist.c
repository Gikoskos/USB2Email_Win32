/******************************************\
*                 usblist.c                *
*        George Koskeridis (C)2015         *
\******************************************/

#include "usb2mail.h"
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <process.h>

extern char *pass, *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER, *USBdev;
struct DataStruct {
	char *pwd, *from, *to, *cc, *subj, *body, *server, *dev;
};

/*******************************************\
* Prototypes for functions with local scope *
\*******************************************/
BOOL SendEmail();
UINT __stdcall U2MThread(LPVOID args);


BOOL InitU2MThread()
{
	if (!pass) {
		MessageBox(NULL, "No password set!", "Error!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	if (!FROM) {
		MessageBox(NULL, "No e-mail to sent is set", "Error!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	if (!SMTP_SERVER || !PORT) {
		MessageBox(NULL, "SMTP server domain and port aren't set!", "Error!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	if (!USBdev) {
		MessageBox(NULL, "No USB device selected!", "Error!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	
	struct DataStruct ThreadArgs = {.pwd = pass,
								    .from = FROM,
								    .to = TO,
								    .cc = CC,
								    .subj = SUBJECT,
								    .body = BODY,
							 	    .server = SMTP_SERVER,
								    .dev = USBdev};
	//DWORD thrdaddr;
	uintptr_t u2mMainThread ATTRIB_UNUSED = _beginthreadex(NULL, 0, U2MThread, (LPVOID)&ThreadArgs, 0, NULL);
	
	return TRUE;
}

UINT __stdcall U2MThread(LPVOID args)
{
	//struct DataStruct TA = *((struct DataStruct*)args);
	while (onoff) {
		MessageBox(NULL, "Halt", "Hello from thread!", MB_ICONASTERISK | MB_OK);
		Sleep(3);
	}
	return 1;
}



BOOL ScanUSBs()
{
	return FALSE;
}

BOOL SendEmail()
{
	return FALSE;
}

VOID fillUSBlist(HWND hwnd)
{
	if (USBRefresh) {
		HDEVINFO hDevInfo;
		SP_DEVINFO_DATA DeviceInfoData;
		DWORD i;

		hDevInfo = SetupDiGetClassDevs(NULL, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES );

		if (hDevInfo == INVALID_HANDLE_VALUE) {
			return;
		}

		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
			DWORD DataT;
			char *buffer = NULL;
			DWORD buffersize = 0;

		   while (!SetupDiGetDeviceRegistryProperty(
				hDevInfo,
				&DeviceInfoData,
				SPDRP_DEVICEDESC,
				&DataT,
				(PBYTE)buffer,
				buffersize,
				&buffersize)) {
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
					if (buffer) LocalFree(buffer);
					buffer = LocalAlloc(LPTR,buffersize * 2);
				} else {
					break;
				}
			}
			AddUSBItem(hwnd, buffer);
			if (buffer) LocalFree(buffer);
		}
		if (GetLastError() != NO_ERROR && GetLastError() != ERROR_NO_MORE_ITEMS) {
			return;
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
}
