/******************************************\
*                 usblist.c                *
*        George Koskeridis (C)2015         *
\******************************************/

#include "usb2mail.h"
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <process.h>
#include <curl/curl.h>

#define TIMEOUT 1000
#define MSG_LEN 7

struct upload_status {
	int lines_read;
};

static char *payload_text[MSG_LEN] = {NULL};

#define ClearPayloadText()                                       \
while (1) {                                                      \
	for (int i = 0; i < 9; i++)                                  \
		free(payload_text[i]);                                   \
	break;                                                       \
}

/*******************************************\
* Prototypes for functions with local scope *
\*******************************************/
BOOL USBisConnected(char *to_find);
UINT __stdcall U2MThread(/*LPVOID args*/);
static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp);
BOOL SendEmail();
void ConstructPayloadText();


BOOL InitU2MThread()
{
	if (!pass) {
		MessageBox(NULL, "No password set!", "Can't start service!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	if (!FROM) {
		MessageBox(NULL, "No e-mail to sent is set", "Can't start service!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	if (!SMTP_SERVER || !PORT_STR) {
		MessageBox(NULL, "SMTP server domain and port aren't set!", "Can't start service!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	if (!USBdev) {
		MessageBox(NULL, "No USB device selected!", "Can't start service!", MB_ICONERROR | MB_OK);
		return FALSE;
	}
	ConstructPayloadText();
	if (onoff) {
		
		//DWORD thrdaddr;
		uintptr_t u2mMainThread ATTRIB_UNUSED = _beginthreadex(NULL, 0, U2MThread, /*(LPVOID)&ThreadArgs*/NULL, 0, NULL);
	}
	return TRUE;
}

UINT __stdcall U2MThread(VOID)
{
	while (onoff) {
		if (USBisConnected(USBdev)) {
			int ret = SendEmail();
			if (ret == CURLE_LOGIN_DENIED) {
				MessageBox(NULL, "E-mail login credentials denied!", "Error!", MB_ICONERROR | MB_OK);
			}
		}
		Sleep(TIMEOUT);
	}
	ClearPayloadText();
	_endthreadex(0);
	return 0;
}

BOOL USBisConnected(char *to_find)
{
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD i;

	hDevInfo = SetupDiGetClassDevs(NULL, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES );

	if (hDevInfo == INVALID_HANDLE_VALUE) {
		return FALSE;
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
		if (buffer) {
			if (!memcmp(buffer, to_find, strlen(to_find))) {
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

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	const char *data;

	if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
		return 0;
	}

	data = payload_text[upload_ctx->lines_read];

	if (data) {
		size_t len = strlen(data);
		memcpy(ptr, data, len);
		upload_ctx->lines_read++;

		return len;
	}

	return 0;
}

void ConstructPayloadText()
{
	char tmp[MSG_LEN][1024];

	snprintf(tmp[0], strlen(TO) + 9, "To: %s\r\n", TO);

	snprintf(tmp[1], strlen(FROM) + 11, "From: %s\r\n", FROM);
	if (CC)
		snprintf(tmp[2], strlen(CC) + 9, "Cc: %s\r\n", CC);
	else
		snprintf(tmp[2], 9, "Cc: \r\n");
	snprintf(tmp[3], strlen(SUBJECT) + 14, "Subject: %s\r\n", SUBJECT);
	snprintf(tmp[4], 5, "\r\n");
	snprintf(tmp[5], strlen(BODY) + 5, "%s\r\n", BODY);

	for (int i = 0; i < MSG_LEN; i++) {
		if (i == MSG_LEN-1) {
			payload_text[i] = NULL;
			break;
		}
		payload_text[i] = realloc(NULL, strlen(tmp[i]) + 1);
		strncpy(payload_text[i], tmp[i], strlen(tmp[i] + 1));
	}
}

int SendEmail()
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status upload_ctx;

	upload_ctx.lines_read = 0;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, USER);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, pass);

		curl_easy_setopt(curl, CURLOPT_URL, SMTP_SERVER);

		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);

		recipients = curl_slist_append(recipients, TO);
		if (CC)
			recipients = curl_slist_append(recipients, CC);
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

#ifdef DEBUG
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_slist_free_all(recipients);

		curl_easy_cleanup(curl);
	}

	return (int)res;
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
