/******************************************
*                U2MWin32.c                *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "usb2mail.h"

const char szClassName[] = "USB2MailClass";

HINSTANCE g_hInst;


/**************************
*Input field formatted data*
 **************************/
char *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER;

/********************
*Input field raw data*
 ********************/
char *pass, *USER, *RECEIVER, *CC_RAW, *PORT_STR, *SMTP_STR;
UINT PORT;

char *USBdev;
UINT TIMEOUT;
INT usb_idx;

/*******************************
*Bools to check enabled controls*
 *******************************/
BOOL ValidEmailCheck = FALSE;
BOOL USBRefresh = TRUE;
BOOL STARTSTOPstate = TRUE;

HDC hdc;
PAINTSTRUCT ps;

/*Is TRUE when the service is running and FALSE when it's not*/
UINT onoff = FALSE;


/********************
*Main Window controls*
 ********************/
HWND USBListButton, EMAILButton, STARTSTOP, time_track, ttrack_tooltip, ttrack_label;
TOOLINFO ttrack_struct;
char ttrack_tooltip_text[50];

UINT EMAIL_PAUSE = 0;

#define IDC_CHOOSEUSBBUTTON     40027
#define IDC_EMAILBUTTON         40028
#define IDC_STARTSTOP           40029
#define IDC_TIMETRACK           40030

/*Trackbar limits*/
#define T_MIN                     200
#define T_MAX                    2000

/*Flag for ValidEmailCheck*/
#define NO_SEPARATOR              '\0'

/********************************************
*Macros to clear all data entered by the user*
 ********************************************/

#define ClearPwd()                                                     \
while (1) {                                                            \
	if (pass) free(pass);                                              \
	pass = NULL;                                                       \
	break;                                                             \
}

#define ClearPrefs()                                                   \
while (1) {                                                            \
	if (SMTP_SERVER) free(SMTP_SERVER);                                \
	if (PORT_STR) free(PORT_STR);                                      \
	if (SMTP_STR) free(SMTP_STR);                                      \
	SMTP_STR = NULL;                                                   \
	PORT_STR = NULL;                                                   \
	SMTP_SERVER = NULL;                                                \
	PORT = 0;                                                          \
	break;                                                             \
}

#define ClearUSBSelection()                                            \
while (1) {                                                            \
	if (USBdev) free(USBdev);                                          \
	USBdev = NULL;                                                     \
	usb_idx = -1;                                                      \
	break;                                                             \
}

#define ClearEmailData()                                               \
while (1) {                                                            \
	if (FROM) free(FROM);                                              \
	if (TO) free(TO);                                                  \
	if (CC) free(CC);                                                  \
	if (SUBJECT) free(SUBJECT);                                        \
	if (BODY) free(BODY);                                              \
	if (USER) free(USER);                                              \
	if (RECEIVER) free(RECEIVER);                                      \
	if (CC_RAW) free(CC_RAW);                                          \
	FROM = TO = CC = SUBJECT = BODY = USER = RECEIVER = CC_RAW = NULL; \
	break;                                                             \
}

/*******************************************
* Prototypes for functions with local scope *
 *******************************************/
LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HelpDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EmailDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK USBDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PwdDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PrefDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL parseEmailDialogFields(HWND hwnd);
BOOL parsePrefDialogFields(HWND hwnd);
BOOL parsePwdField(HWND hwnd);
HWND WINAPI CreateBaloonToolTip(int toolID, HWND hDlg, LPTSTR pszText);
HWND WINAPI CreateTrackingToolTip(int toolID, HWND hDlg, LPTSTR pszText);
BOOL isValidDomain(char *str, char SEPARATOR);
VOID GetFieldText(HWND hwnd, int nIDDlgItem, char **str);

VOID InitEmailDialog(HWND hwnd);
VOID InitAboutDialog(HWND hwnd);
VOID InitPasswordDialog(HWND hwnd);
VOID InitPreferencesDialog(HWND hwnd);
VOID InitHelpWindow(HWND hwnd);

VOID DeleteUSBItem(HWND hwnd, char *s);
VOID AddUSBItem(HWND hwnd, char *s);
VOID RemoveStringFromListBox(HWND hDlg, int nIDDlgItem, char *s);
VOID AddStringToListBox(HWND hDlg, int nIDDlgItem, char *s);
VOID DeleteAll();
VOID GetFieldText(HWND hwnd, int nIDDlgItem, char **str);

//@TODO: Implement ErrorCodes functions
/*char *ErrorCodes(int i)
{

}

char *ErrorCodeDlgTitles(int i)
{

}*/


VOID GetFieldText(HWND hwnd, int nIDDlgItem, char **str)
{
	int bufsiz = GetWindowTextLength(GetDlgItem(hwnd, nIDDlgItem)) + 1;

	if (bufsiz > 1) {
		(*str) = calloc(bufsiz, 1);
		GetDlgItemText(hwnd, nIDDlgItem, (*str), bufsiz);
	} else {
		if (*str)
			free(*str);
		(*str) = NULL;
	}
}

VOID ChangeSTARTSTOPText()
{
	if (onoff) {
		if (!SetWindowText(STARTSTOP, "Stop")) {
			MessageBox(NULL, "Failure at changing label!", "Error!", MB_ICONERROR | MB_OK);
		}
	} else {
		if (!SetWindowText(STARTSTOP, "Start")) {
			MessageBox(NULL, "Failure at changing label!", "Error!", MB_ICONERROR | MB_OK);
		}
	}
}

VOID DeleteAll()
{
	ClearEmailData();
	ClearPwd();
	ClearPrefs();
	ClearUSBSelection();
}

VOID AddStringToListBox(HWND hDlg, int nIDDlgItem, char *s)
{
	if (s)
		SendDlgItemMessage(hDlg, nIDDlgItem, LB_ADDSTRING, (WPARAM)0, (LPARAM)s);
}

VOID RemoveStringFromListBox(HWND hDlg, int nIDDlgItem, char *s)
{
	if (s)
		SendDlgItemMessage(hDlg, nIDDlgItem, LB_DELETESTRING, (WPARAM)0, (LPARAM)s);
}

VOID AddUSBItem(HWND hwnd, char *s)
{
	AddStringToListBox(hwnd, IDC_USBDEVLIST, s);
}

VOID DeleteUSBItem(HWND hwnd, char *s)
{
	RemoveStringFromListBox(hwnd, IDC_USBDEVLIST, s);
}

VOID InitPreferencesDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PREFDIALOG),
					hwnd, PrefDialogProcedure) == -1) {
		MessageBox(hwnd, "Preferences dialog failed to open!", "Error!", MB_OK | MB_ICONERROR);
	}
}

VOID InitPasswordDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PWDDIALOG),
					hwnd, PwdDialogProcedure) == -1) {
		MessageBox(hwnd, "Password dialog failed to open!", "Error!", MB_OK | MB_ICONERROR);
	}
}

VOID InitUSBDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_USBDIALOG),
					hwnd, USBDialogProcedure) == -1) {
		MessageBox(hwnd, "USB dialog failed to open!", "Error!", MB_OK | MB_ICONERROR);
	}
}

VOID InitAboutDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUTDIALOG),
					hwnd, AboutDialogProcedure) == -1) {
		MessageBox(hwnd, "About dialog failed to open!", "Error!", MB_OK | MB_ICONERROR);
	}
}

VOID InitEmailDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EMAILDIALOG),
					hwnd, EmailDialogProcedure) == -1) {
		MessageBox(hwnd, "Email dialog failed to open!", "Error!", MB_OK | MB_ICONERROR);
	}
}

VOID InitHelpDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HELPDIALOG),
					hwnd, HelpDialogProcedure) == -1) {
		MessageBox(hwnd, "Help dialog failed to open!", "Error!", MB_OK | MB_ICONERROR);
	}
}

BOOL isValidDomain(char *str, char SEPARATOR)
{
	if (strchr(str, ' '))
		return FALSE;

	char *c = strchr(str, '@') + 1;

	if (!c)
		return FALSE;
	if (c[0] == '.')
		return FALSE;
	if(strchr(c, '@'))
		return isValidDomain(strchr(c, '@'), SEPARATOR);

	char *d = c;
	c = strchr(c, '.');
	if (!c)
		return FALSE;
	if (c[0] == '\0' || d[1] == c[0])
		return FALSE;

	if (SEPARATOR != NO_SEPARATOR) {
		if (strchr(c, SEPARATOR))
			return isValidDomain(strchr(c, SEPARATOR), SEPARATOR);
	}
	return TRUE;
}

HWND WINAPI CreateBaloonToolTip(int toolID, HWND hDlg, LPTSTR pszText)
{
	if (!toolID || !hDlg || !pszText)
		return FALSE;
	HWND hwndTool = GetDlgItem(hDlg, toolID);

	HWND hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
								WS_POPUP |TTS_ALWAYSTIP | TTS_BALLOON,
								CW_USEDEFAULT, CW_USEDEFAULT,
								CW_USEDEFAULT, CW_USEDEFAULT,
								hDlg, NULL, 
								g_hInst, NULL);

	if (!hwndTool || !hwndTip)
		return (HWND)NULL;             

	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(TTTOOLINFO_V1_SIZE);
	toolInfo.hwnd = hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = pszText;
	SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
	return hwndTip;
}

HWND WINAPI CreateTrackingToolTip(int toolID, HWND hDlg, LPTSTR pszText)
{
	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, 
							WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 
							CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
							hDlg, NULL, g_hInst,NULL);

	if (!hwndTT)
		return NULL;

	ttrack_struct.cbSize   = sizeof(TOOLINFO);
	ttrack_struct.uFlags   = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	ttrack_struct.hwnd     = hDlg;
	ttrack_struct.hinst    = g_hInst;
	ttrack_struct.lpszText = pszText;
	ttrack_struct.uId      = (UINT_PTR)hDlg;

	GetClientRect (hDlg, &ttrack_struct.rect);

	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ttrack_struct);		
	return hwndTT;
}

void OnButtonClickGetSelection(HWND hwnd)
{
	usb_idx = (int)SendMessage(GetDlgItem(hwnd, IDC_USBDEVLIST), LB_GETCURSEL, (WPARAM)0, (LPARAM)0);

	if (usb_idx == LB_ERR)
		return;

	int str_len = (int)SendMessage(GetDlgItem(hwnd, IDC_USBDEVLIST), LB_GETTEXTLEN, (WPARAM)usb_idx, 0);
	USBdev = malloc(str_len + 1);
	SendMessage(GetDlgItem(hwnd, IDC_USBDEVLIST), LB_GETTEXT, (WPARAM)usb_idx, (LPARAM)USBdev);
}

BOOL parsePrefDialogFields(HWND hwnd)
{
	char *tmp1 = NULL, *tmp2 = NULL;

	GetFieldText(hwnd, IDC_SERVERURLFIELD, &tmp1);
	if (!tmp1 && !onoff) {
		MessageBox(hwnd, "SMTP server field is empty!", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	GetFieldText(hwnd, IDC_PORTFIELD, &tmp2);
	if (!tmp2) {
		free(tmp1);
		MessageBox(hwnd, "SMTP network port field is empty!", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	} else if (strlen(tmp2) > 5) {
		free(tmp1);
		free(tmp2);
		MessageBox(hwnd, "Invalid SMTP network port number!", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	PORT = 0;
	UINT c;
	for (size_t i = 0; i < strlen(tmp2) ; i++) {
		c = tmp2[i] - '0';
		PORT = c*((UINT)pow(10, (strlen(tmp2) - (i+1)))) + PORT;
	}

	if (PORT > 65535) {
		MessageBox(hwnd, "Invalid SMTP network port number!", "Error!", MB_OK | MB_ICONERROR);
		free(tmp1);
		free(tmp2);
		return FALSE;
	}
	SMTP_STR = realloc(NULL, strlen(tmp1)+1);
	strncpy(SMTP_STR, tmp1, strlen(tmp1)+1);
	PORT_STR = realloc(NULL, strlen(tmp2)+1);
	strncpy(PORT_STR, tmp2, strlen(tmp2)+1);

	/*Create a string of this format "smtp://domain.name:PORT" to use with curl*/
	SMTP_SERVER = realloc(NULL, strlen(tmp1)+strlen(tmp2)+2);
	snprintf(SMTP_SERVER, strlen(tmp1)+strlen(tmp2)+2, "%s:%s", tmp1, tmp2);
	free(tmp1);
	free(tmp2);
	return TRUE;
}

BOOL parseEmailDialogFields(HWND hwnd)
{
	char *tmp = NULL;


	GetFieldText(hwnd, IDC_FROMFIELD, &tmp);
	if (!tmp) {
		MessageBox(hwnd, "FROM field is empty!", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	if (ValidEmailCheck) { 
		if (!isValidDomain(tmp, NO_SEPARATOR)) {
			MessageBox(hwnd, "Invalid e-mail address on FROM field!", "Error!", MB_OK | MB_ICONERROR);
			free(tmp);
			return FALSE;
		}
	}
	FROM = realloc(NULL, strlen(tmp)+3);
	USER = realloc(NULL, strlen(tmp)+1);
	strncpy(USER, tmp, strlen(tmp)+1);
	snprintf(FROM, strlen(tmp)+3, "<%s>", tmp);
	free(tmp);
	tmp = NULL;

	GetFieldText(hwnd, IDC_TOFIELD, &tmp);
	if (!tmp) {
		MessageBox(hwnd, "TO field is empty!", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	if (ValidEmailCheck) { 
		if (!isValidDomain(tmp, NO_SEPARATOR)) {
			MessageBox(hwnd, "Invalid e-mail address on TO field!", "Error!", MB_OK | MB_ICONERROR);
			free(tmp);
			return FALSE;
		}
	}
	TO = realloc(NULL, strlen(tmp)+3);
	RECEIVER = realloc(NULL, strlen(tmp)+1);
	strncpy(RECEIVER, tmp, strlen(tmp)+1);
	snprintf(TO, strlen(tmp)+3, "<%s>", tmp);
	free(tmp);
	tmp = NULL;

	GetFieldText(hwnd, IDC_CCFIELD, &tmp);

	if (ValidEmailCheck && tmp) { 
		if (!isValidDomain(tmp, ';')) {
			MessageBox(hwnd, "Invalid e-mail address on CC field!", "Error!", MB_OK | MB_ICONERROR);
			free(tmp);
			return FALSE;
		}
	} else if (!tmp) {
		CC = NULL;
	} else {
		CC = realloc(NULL, strlen(tmp)+3);
		CC_RAW = realloc(NULL, strlen(tmp)+1);
		strncpy(CC_RAW, tmp, strlen(tmp)+1);
		snprintf(CC, strlen(tmp)+3, "<%s>", tmp);
		free(tmp);
		tmp = NULL;
	}

	GetFieldText(hwnd, IDC_SUBJECTFIELD, &tmp);
	if (!tmp) {
		if (MessageBox(hwnd, "Are you sure you don't want to have a Subject in your e-mail?", 
			"No Subject", MB_YESNO | MB_ICONASTERISK) == IDYES) {
			SUBJECT = malloc(2);
			snprintf(SUBJECT, 2, "");
		} else {
			return FALSE;
		}
	} else {
		SUBJECT = realloc(NULL, strlen(tmp)+1);
		strncpy(SUBJECT, tmp, strlen(tmp)+1);
		free(tmp);
		tmp = NULL;
	}


	GetFieldText(hwnd, IDC_MESSAGEFIELD, &tmp);
	if (!tmp) {
		if (MessageBox(hwnd, "Are you sure you want to send a blank message?",
			"No e-mail body", MB_YESNO | MB_ICONASTERISK) == IDYES) {
			BODY = malloc(2);
			snprintf(BODY, 2, "");
		} else {
			return FALSE;
		}
	} else {
		BODY = realloc(NULL, strlen(tmp)+1);
		strncpy(BODY, tmp, strlen(tmp)+1);
		free(tmp);
	}

	return TRUE;
}

BOOL parsePwdField(HWND hwnd)
{
	char *tmp = NULL;

	GetFieldText(hwnd, IDC_PWDFIELD, &tmp);
	if (!tmp) {
		MessageBox(hwnd, "No password entered", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	} else {
		pass = realloc(NULL, strlen(tmp)+1);
		strncpy(pass, tmp, strlen(tmp)+1);
		free(tmp);
	}
	return TRUE;
}

INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HICON about_usb_icon;
	switch (msg) {
		case WM_INITDIALOG:
			about_usb_icon = (HICON)LoadIcon(GetModuleHandle(NULL), 
				MAKEINTRESOURCE(IDI_USB2MAILICONLARGE));
			SendDlgItemMessage(hwnd, IDUSB2MAIL, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)about_usb_icon);
			return (INT_PTR)TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDUSB2MAIL:
					EndDialog(hwnd, IDUSB2MAIL);
					return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK PwdDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			if (pass)
				SetDlgItemText(hwnd, IDC_PWDFIELD, pass);
			return (INT_PTR)TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ClearPwd();
					if (parsePwdField(hwnd))
						EndDialog(hwnd, wParam);
					else
						ClearPwd();
					return (INT_PTR)TRUE;
				case IDCANCEL:
					EndDialog(hwnd, wParam);
					return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK PrefDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			if (SMTP_STR)
				SetDlgItemText(hwnd, IDC_SERVERURLFIELD, SMTP_STR);
			if (PORT_STR)
				SetDlgItemText(hwnd, IDC_PORTFIELD, PORT_STR);
			CheckDlgButton(hwnd, IDC_CHECKVALIDEMAIL, (ValidEmailCheck==TRUE)?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_CHECKUSBREFRESH, (USBRefresh==TRUE)?BST_CHECKED:BST_UNCHECKED);
			SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 50));
			SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);
			return (INT_PTR)TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ValidEmailCheck = IsDlgButtonChecked(hwnd, IDC_CHECKVALIDEMAIL);
					USBRefresh = IsDlgButtonChecked(hwnd, IDC_CHECKUSBREFRESH);
					ClearPrefs();
					if (parsePrefDialogFields(hwnd))
						EndDialog(hwnd, wParam);
					else
						ClearPrefs();
					return (INT_PTR)TRUE;
				case IDCANCEL:
					EndDialog(hwnd, wParam);
					return (INT_PTR)TRUE;
			}
		case WM_HSCROLL:
			EMAIL_PAUSE = (UINT)SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_GETPOS, (WPARAM)0, (LPARAM)0);
			break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK USBDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HBITMAP refresh_bitmap;
	switch (msg) {
		case WM_INITDIALOG:
			refresh_bitmap = (HBITMAP)LoadImage(NULL, "icons\\refreshbitmap.bmp",
				IMAGE_BITMAP, 0, 0,	LR_DEFAULTSIZE | LR_LOADFROMFILE | LR_LOADTRANSPARENT);
			SendDlgItemMessage(hwnd, IDUSBREFRESH, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)refresh_bitmap);
			fillUSBlist(hwnd);
			SendDlgItemMessage(hwnd, IDC_USBDEVLIST, LB_SETCURSEL, (WPARAM)usb_idx, (LPARAM)0);
			return (INT_PTR)TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDUSBREFRESH:
					SendDlgItemMessage(hwnd, IDC_USBDEVLIST, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
					fillUSBlist(hwnd);
					return (INT_PTR)TRUE;
				case IDOK:
					ClearUSBSelection();
					OnButtonClickGetSelection(hwnd);
					EndDialog(hwnd, wParam);
					return (INT_PTR)TRUE;
				case IDCANCEL:
					EndDialog(hwnd, wParam);
					return (INT_PTR)TRUE;
			}
			switch (HIWORD(wParam)) {
				case LBN_DBLCLK:
					OnButtonClickGetSelection(hwnd);
					EndDialog(hwnd, wParam);
					return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK EmailDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND FROM_ttip ATTRIB(unused),
		 TO_ttip ATTRIB(unused),
		 CC_ttip ATTRIB(unused),
		 SUBJECT_ttip ATTRIB(unused),
		 BODY_ttip ATTRIB(unused);
	switch (msg) {
		case WM_INITDIALOG:
			FROM_ttip = CreateBaloonToolTip(IDC_FROMFIELD, hwnd, 
			"E-mail address of sender");
			TO_ttip = CreateBaloonToolTip(IDC_TOFIELD, hwnd, 
			"E-mail address of recipient");
			CC_ttip = CreateBaloonToolTip(IDC_CCFIELD, hwnd, 
			"Group of addresses to send to. Multiple e-mails are seperated with ';'");
			SUBJECT_ttip = CreateBaloonToolTip(IDC_SUBJECTFIELD, hwnd, 
			"Subject of the e-mail");
			BODY_ttip = CreateBaloonToolTip(IDC_MESSAGEFIELD, hwnd,
			"Body of the e-mail");
			if (FROM)
				SetDlgItemText(hwnd, IDC_FROMFIELD, USER);
			if (TO)
				SetDlgItemText(hwnd, IDC_TOFIELD, RECEIVER);
			if (CC)
				SetDlgItemText(hwnd, IDC_CCFIELD, CC_RAW);
			if (SUBJECT)
				SetDlgItemText(hwnd, IDC_SUBJECTFIELD, SUBJECT);
			if (BODY)
				SetDlgItemText(hwnd, IDC_MESSAGEFIELD, BODY);
			return (INT_PTR)TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ClearEmailData();
					if (parseEmailDialogFields(hwnd))
						EndDialog(hwnd, wParam);
					else
						ClearEmailData();
					break;
				case IDCANCEL:
					EndDialog(hwnd, wParam);
					return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK HelpDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					EndDialog(hwnd, wParam);
					return (INT_PTR)TRUE;
			}
			break;
		case WM_CLOSE:
			EndDialog(hwnd, wParam);
			return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_CREATE:
			{
				InitCommonControls();
				USBListButton = CreateWindowEx(0, "BUTTON", "Choose USB device",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
					30, 30, 200, 30, hwnd, (HMENU)IDC_CHOOSEUSBBUTTON,
					(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
				if (!USBListButton) {
					MessageBox(NULL, "USB list button failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}

				EMAILButton = CreateWindowEx(0, "BUTTON", "Configure E-Mail to send",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
					300, 30, 200, 30, hwnd, (HMENU)IDC_EMAILBUTTON, 
					(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
				if (!EMAILButton) {
					MessageBox(NULL, "E-mail button failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}

				STARTSTOP = CreateWindowEx(0, "BUTTON", "Start",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
					300, 200, 200, 50, hwnd, (HMENU)IDC_STARTSTOP, 
					g_hInst, NULL);
				if (!STARTSTOP) {
					MessageBox(NULL, "Start/Stop button failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}

				time_track = CreateWindowEx(0, TRACKBAR_CLASS, "",
					WS_CHILD | WS_VISIBLE | TBS_TOOLTIPS | TBS_NOTICKS | TBS_HORZ,
					30, 200, 200, 30, hwnd, (HMENU)IDC_TIMETRACK,
					g_hInst, NULL);
				if (!time_track) {
					MessageBox(NULL, "Trackbar failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}

				ttrack_label = CreateWindow("STATIC", 
					"Set waiting interval between each scan of all USB devices in milliseconds",
					WS_VISIBLE | WS_CHILD | SS_CENTER,
					30, 140, 200, 60, hwnd, NULL, g_hInst, NULL);
				if (!ttrack_label) {
					MessageBox(NULL, "Trackbar label failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}
				/*ttrack_tooltip = CreateTrackingToolTip(IDC_TIMETRACK, time_track,
					ttrack_tooltip_text);*/

				SendMessage(time_track, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ttrack_struct);
				SendMessage(time_track, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(T_MIN, T_MAX));
				SendMessage(time_track, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)4);
				SendMessage(time_track, TBM_SETSEL, (WPARAM)FALSE, (LPARAM)MAKELONG(T_MIN, T_MAX));
				SendMessage(time_track, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)1000);
				//SendMessage(time_track, TBM_SETTOOLTIPS, (WPARAM)ttrack_tooltip, (LPARAM)0);
				UpdateWindow(hwnd);
				SetActiveWindow(hwnd);
			}
		case WM_MOUSEMOVE:
			break;
		case WM_PAINT:
			if ((hdc = BeginPaint(hwnd, &ps))) {
				EndPaint(hwnd, &ps);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDM_ABOUT:
					InitAboutDialog(hwnd);
					break;
				case IDM_PREF:
					if (!onoff)
						InitPreferencesDialog(hwnd);
					else
						MessageBox(hwnd, "Can't change preferences while the service is running.", 
							"Service is running!", MB_ICONEXCLAMATION | MB_OK);
					break;
				case IDM_PASSWORD:
					if (!onoff)
						InitPasswordDialog(hwnd);
					else
						MessageBox(hwnd, "Can't change password while the service is running.", 
							"Service is running!", MB_ICONEXCLAMATION | MB_OK);
					break;
				case IDC_CHOOSEUSBBUTTON:
					if (!onoff)
						InitUSBDialog(hwnd);
					else
						MessageBox(hwnd, "Can't change USB device while the service is running.", 
							"Service is running!", MB_ICONEXCLAMATION | MB_OK);
					break;
				case IDC_STARTSTOP:
					onoff = (onoff == TRUE)?FALSE:TRUE;
					if (InitU2MThread()) {
						ChangeSTARTSTOPText();
						PostMessage(hwnd, WM_PAINT, wParam, lParam);
					}
					break;
				case IDC_EMAILBUTTON:
					if (!onoff)
						InitEmailDialog(hwnd);
					else
						MessageBox(hwnd, "Can't change e-mail while the service is running.", 
							"Service is running!", MB_ICONEXCLAMATION | MB_OK);
					break;
				case IDM_H_ELP1:
					InitHelpDialog(hwnd);
					break;
				case IDM_EXIT:
					PostMessage(hwnd, WM_CLOSE, wParam, lParam);
					break;
			}
			break;
		case WM_CLOSE:
			if (MessageBox(hwnd, "Are you sure you want to quit?", 
					"Quiting...", MB_ICONASTERISK | MB_YESNO) == IDNO)
				break;
			DeleteAll();
			AnimateWindow(hwnd, 100, AW_HIDE | AW_CENTER);
			DestroyWindow(hwnd);
			break;
		case WM_HSCROLL:
			TIMEOUT = (UINT)SendMessage(time_track, TBM_GETPOS, (WPARAM)0, (LPARAM)0);
			break;
		case WM_DESTROY:
			DeleteAll();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;
	BOOL bRet = TRUE;

	/* Global initializations */
	PORT = 0;
	pass = CC = TO = FROM = SUBJECT = BODY = SMTP_SERVER = USBdev = USER = PORT_STR = SMTP_STR = RECEIVER = CC_RAW = NULL;
	g_hInst = hInstance;
	usb_idx = -1;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.lpfnWndProc = MainWindowProcedure;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USB2MAILICONMEDIUM));
	wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USB2MAILICONSMALL));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszClassName = szClassName;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);

	if (!RegisterClassEx(&wc)) {
			MessageBox(NULL, "Main Window class registration Failed!", "Error!",
			MB_ICONERROR | MB_OK);
		return -1;
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, szClassName, "USB2Mail Win",
	WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
	CW_USEDEFAULT, CW_USEDEFAULT, 550, 350, NULL, NULL, hInstance, NULL);

	if (!hwnd) {
		MessageBox(NULL, "Window creation failed!", "Error!", MB_ICONERROR | MB_OK);
		return -2;
	}
	AnimateWindow(hwnd, 200, AW_CENTER);
	while (bRet) {
		bRet = GetMessage(&Msg, NULL, 0, 0);

		if (bRet == -1) {
			MessageBox(NULL, "Message queue error", "Error!", MB_ICONERROR | MB_OK);
			return -3;
		} else if (bRet == -2) {
			return -2;
		}

		if (RUNNING == STARTSTOPstate) {
			Button_Enable(STARTSTOP, FALSE);
			UpdateWindow(hwnd);
		} else {
			Button_Enable(STARTSTOP, TRUE);
			UpdateWindow(hwnd);
		}
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}
