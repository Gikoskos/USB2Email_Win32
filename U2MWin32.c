/******************************************\
*              mainWindow.c                *
*        George Koskeridis (C)2015         *
\******************************************/

#include "usb2mail.h"

const char szClassName[] = "USB2MailClass";

HINSTANCE g_hInst;


/**************************
*Input field formatted data*
 **************************/
char *pass, *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER;


/********************
*Input field raw data*
 ********************/
char *USER, *RECEIVER, *CC_RAW, *PORT_STR, *SMTP_STR;
UINT PORT;

char *USBdev;

INT usb_idx;


/**********************
*AutoCheck Button Bools*
 **********************/
BOOL ValidEmailCheck = FALSE;
BOOL USBRefresh = TRUE;


HDC hdc;
PAINTSTRUCT ps;

/*Is TRUE when the service is running and FALSE when it's not*/
volatile BOOL onoff = FALSE;


/********************
*Main Window controls*
 ********************/
HWND USBListButton, EMAILButton, STARTSTOP, time_track;


#define IDC_CHOOSEUSBBUTTON     40027
#define IDC_EMAILBUTTON         40028
#define IDC_STARTSTOP           40029
#define IDC_TIMETRACK           40030
#define IDC_HELPOK              40031

#define T_MIN                       0
#define T_MAX                      20
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
HWND WINAPI CreateToolTip(int toolID, HWND hDlg, PTSTR pszText);
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

HWND WINAPI CreateToolTip(int toolID, HWND hDlg, PTSTR pszText)
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
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = hDlg;
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)hwndTool;
    toolInfo.lpszText = pszText;
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
    return hwndTip;
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
	if (!tmp1) {
		MessageBox(hwnd, "SMTP server field is empty!", "Error!", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	GetFieldText(hwnd, IDC_PORTFIELD, &tmp2);
	if (!tmp2) {
		free(tmp1);
		MessageBox(hwnd, "SMTP network port field is empty!", "Error!", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	} else if (strlen(tmp2) > 5) {
		free(tmp1);
		free(tmp2);
		MessageBox(hwnd, "Invalid SMTP network port number!", "Error!", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	PORT = 0;
	UINT c;
	for (size_t i = 0; i < strlen(tmp2) ; i++) {
		c = tmp2[i] - '0';
		PORT = c*((UINT)pow(10, (strlen(tmp2) - (i+1)))) + PORT;
	}

	if (PORT > 65535) {
		MessageBox(hwnd, "Invalid SMTP network port number!", "Error!", MB_OK | MB_ICONEXCLAMATION);
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
		MessageBox(hwnd, "FROM field is empty!", "Error!", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (ValidEmailCheck) { 
		if (!isValidDomain(tmp, NO_SEPARATOR)) {
			MessageBox(hwnd, "Invalid e-mail address on FROM field!", "Error!", MB_OK | MB_ICONEXCLAMATION);
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
		MessageBox(hwnd, "TO field is empty!", "Error!", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (ValidEmailCheck) { 
		if (!isValidDomain(tmp, NO_SEPARATOR)) {
			MessageBox(hwnd, "Invalid e-mail address on TO field!", "Error!", MB_OK | MB_ICONEXCLAMATION);
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
			MessageBox(hwnd, "Invalid e-mail address on CC field!", "Error!", MB_OK | MB_ICONEXCLAMATION);
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
			"No Subject", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
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
			"No e-mail body", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
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
		MessageBox(hwnd, "No password entered", "Error!", MB_OK | MB_ICONEXCLAMATION);
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
	HICON about_usb_icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USB2MAILICONLARGE));
	switch (msg) {
		case WM_INITDIALOG:
			SendDlgItemMessage(hwnd, IDUSB2MAIL, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)about_usb_icon);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDUSB2MAIL:
					EndDialog(hwnd, IDUSB2MAIL);
					break;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK PwdDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			if (pass)
				SetDlgItemText(hwnd, IDC_PWDFIELD, "*****************");
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					if (parsePwdField(hwnd))
						EndDialog(hwnd, wParam);
					else
						ClearPwd();
					break;
				case IDCANCEL:
					EndDialog(hwnd, wParam);
					break;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
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
			return TRUE;
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
					break;
				case IDCANCEL:
					EndDialog(hwnd, wParam);
					break;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK USBDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HBITMAP refresh_bitmap = LoadImage(NULL,
	"icons\\refreshbitmap.bmp", IMAGE_BITMAP, 0, 0,
	LR_DEFAULTSIZE | LR_LOADFROMFILE | LR_LOADTRANSPARENT);
	switch (msg) {
		case WM_INITDIALOG:
			SendDlgItemMessage(hwnd, IDUSBREFRESH, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)refresh_bitmap);
			fillUSBlist(hwnd);
			SendDlgItemMessage(hwnd, IDC_USBDEVLIST, LB_SETCURSEL, (WPARAM)usb_idx, (LPARAM)0);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDUSBREFRESH:
					SendDlgItemMessage(hwnd, IDC_USBDEVLIST, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
					fillUSBlist(hwnd);
					break;
				case IDOK:
					ClearUSBSelection();
					OnButtonClickGetSelection(hwnd);
					EndDialog(hwnd, wParam);
					break;
				case IDCANCEL:
					EndDialog(hwnd, wParam);
					break;
			}
			switch (HIWORD(wParam)) {
				case LBN_DBLCLK:
					OnButtonClickGetSelection(hwnd);
					EndDialog(hwnd, wParam);
					break;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK EmailDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND FROM_ttip ATTRIB_UNUSED,
		 TO_ttip ATTRIB_UNUSED,
		 CC_ttip ATTRIB_UNUSED,
		 SUBJECT_ttip ATTRIB_UNUSED,
		 BODY_ttip ATTRIB_UNUSED;
	switch (msg) {
		case WM_INITDIALOG:
			FROM_ttip = CreateToolTip(IDC_FROMFIELD, hwnd, "E-mail address of sender");
			TO_ttip = CreateToolTip(IDC_TOFIELD, hwnd, "E-mail address of recipient");
			CC_ttip = CreateToolTip(IDC_CCFIELD, hwnd, "Group of addresses to send to. Multiple e-mails are seperated with ';'");
			SUBJECT_ttip = CreateToolTip(IDC_SUBJECTFIELD, hwnd, "Subject of the e-mail");
			BODY_ttip = CreateToolTip(IDC_MESSAGEFIELD, hwnd, "Body of the e-mail");
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
			return TRUE;
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
					break;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK HelpDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					EndDialog(hwnd, wParam);
					break;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_CREATE:
			{
				USBListButton = CreateWindowEx(0, "BUTTON", "Choose USB device",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
					30, 30, 200, 30, hwnd, (HMENU)IDC_CHOOSEUSBBUTTON,
					(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
				if (!USBListButton) {
					MessageBox(NULL, "USBLISTButton creation failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}
				
				EMAILButton = CreateWindowEx(0, "BUTTON", "Configure E-Mail to send",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
					300, 30, 200, 30, hwnd, (HMENU)IDC_EMAILBUTTON, 
					(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
				if (!EMAILButton) {
					MessageBox(NULL, "EMAILButton creation failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}
				
				STARTSTOP = CreateWindowEx(0, "BUTTON", "Start",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
					300, 200, 200, 50, hwnd, (HMENU)IDC_STARTSTOP, 
					(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
				if (!STARTSTOP) {
					MessageBox(NULL, "STARTSTOP creation failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}
				
				time_track = CreateWindowEx(0, TRACKBAR_CLASS, "Set waiting time after an e-mail is sent",
					WS_CHILD | WS_VISIBLE,
					30, 200, 200, 30, hwnd, (HMENU)IDC_TIMETRACK, g_hInst, NULL);
				if (!time_track) {
					MessageBox(NULL, "Trackbar creation failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}	
				
				SendMessage(time_track, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(T_MIN, T_MAX));
				SendMessage(time_track, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)4);
				SendMessage(time_track, TBM_SETSEL, (WPARAM)FALSE, (LPARAM)MAKELONG(T_MIN, T_MAX));
				SendMessage(time_track, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)T_MIN);
				
				//SetFocus(time_track);
			}
		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDM_ABOUT:
					InitAboutDialog(hwnd);
					break;
				case IDM_PREF:
					InitPreferencesDialog(hwnd);
					break;
				case IDM_PASSWORD:
					InitPasswordDialog(hwnd);
					break;
				case IDC_CHOOSEUSBBUTTON:
					InitUSBDialog(hwnd);
					break;
				case IDC_STARTSTOP:
					onoff = (onoff == TRUE)?FALSE:TRUE;
					if (InitU2MThread()) {
						if (onoff) {
							if (!SetWindowText(STARTSTOP, "Stop")) {
								MessageBox(NULL, "Failure at changing label!", "Error!", MB_ICONERROR | MB_OK);
							}
						}
						else {
							if (!SetWindowText(STARTSTOP, "Start")) {
								MessageBox(NULL, "Failure at changing label!", "Error!", MB_ICONERROR | MB_OK);
							}
						}
					}
					break;
				case IDC_EMAILBUTTON:
					InitEmailDialog(hwnd);
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
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
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
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
	wc.lpszClassName = szClassName;

	if (!RegisterClassEx(&wc)) {
			MessageBox(NULL, "Main Window class registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return -1;
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, szClassName, "USB2Mail Win",
	WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
	CW_USEDEFAULT, CW_USEDEFAULT, 550, 350, NULL, NULL, hInstance, NULL);

	if (!hwnd) {
		MessageBox(NULL, "Window creation failed!", "Error!", MB_ICONERROR | MB_OK);
		return -2;
	}
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (bRet) {
		bRet = GetMessage(&Msg, NULL, 0, 0);
		
		if (bRet == -1) {
			MessageBox(NULL, "Message queue error", "Error!", MB_ICONERROR | MB_OK);
			return -3;
		} else if (bRet == -2) {
			return -2;
		}

		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}
