 /******************************************\
 *              mainWindow.c                *
 *        George Koskeridis (C)2015         *
 \******************************************/

#include "resources\resource.h"
#include "usb2mail.h"

const char szClassName[] = "USB2Mail";

/*Global of instance*/
HINSTANCE g_hInst;

/***********************\
*Input field global data*
\***********************/
char *pass, *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER;
UINT PORT;

/**********************\
*AutoCheck Button Bools*
\**********************/
BOOL ValidEmailCheck = TRUE;
BOOL USBRefresh = TRUE;


HDC hdc;
PAINTSTRUCT ps;
/*Is TRUE when the service is running and FALSE when it's not*/
BOOL onoff = FALSE;

/*********************\
*Main Window resources*
\*********************/
HWND USBListButton, EMAILButton, STARTSTOP, time_track;
#define IDC_CHOOSEUSBBUTTON     40027
#define IDC_EMAILBUTTON         40028
#define IDC_STARTSTOP           40029
#define IDC_TIMETRACK           40030
#define T_MIN                       0
#define T_MAX                      20

/*Flag for ValidEmailCheck*/
#define NO_SEPARATOR              '\0'


/********************************************\
*Macros to clear all data entered by the user*
\********************************************/
#define ClearPwd()                                               \
while (1) {                                                      \
	if (pass) free(pass);                                        \
	pass = NULL;                                                 \
	break;                                                       \
}

#define ClearPrefs()                                             \
while (1) {                                                      \
	if (SMTP_SERVER) free(SMTP_SERVER);                          \
	SMTP_SERVER = NULL;                                          \
	PORT = 0;                                                    \
	break;                                                       \
}

#define ClearEmailData()                                         \
while (1) {                                                      \
	if (FROM) free(FROM);                                        \
	if (TO) free(TO);                                            \
	if (CC) free(CC);                                            \
	if (SUBJECT) free(SUBJECT);                                  \
	if (BODY) free(BODY);                                        \
	FROM = TO = CC = SUBJECT = BODY = SMTP_SERVER = NULL;        \
	break;                                                       \
}

/*********************\
* Function prototypes *
\*********************/
LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EmailDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK USBDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PwdDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PrefDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL parseEmailDialogFields(HWND hwnd);
BOOL parsePrefDialogFields(HWND hwnd);
HWND WINAPI CreateToolTip(int toolID, HWND hDlg, PTSTR pszText);
BOOL isValidDomain(char *str, char SEPARATOR);
VOID GetFieldText(HWND hwnd, int nIDDlgItem, char **str);

VOID InitEmailDialog(HWND hwnd);
VOID InitAboutDialog(HWND hwnd);
VOID InitPasswordDialog(HWND hwnd);
VOID InitPreferencesDialog(HWND hwnd);
VOID DeleteUSBItem(HWND hwnd, char *s);
VOID AddUSBItem(HWND hwnd, char *s);
VOID RemoveStringFromListBox(HWND hDlg, int nIDDlgItem, char *s);
VOID AddStringToListBox(HWND hDlg, int nIDDlgItem, char *s);
VOID DeleteAll();
VOID GetFieldText(HWND hwnd, int nIDDlgItem, char **str);



VOID GetFieldText(HWND hwnd, int nIDDlgItem, char **str)
{
	int bufsiz = GetWindowTextLength(GetDlgItem(hwnd, nIDDlgItem)) + 1;

	if (bufsiz > 1) {
		(*str) = calloc(bufsiz, 1);
		GetDlgItemText(hwnd, nIDDlgItem, (*str), bufsiz);
	} else {
		if ((*str))
			free(*str);
		(*str) = NULL;
	}
}

VOID DeleteAll()
{
	ClearEmailData();
	ClearPwd();
	ClearPrefs();
}

VOID AddStringToListBox(HWND hDlg, int nIDDlgItem, char *s)
{
	SendDlgItemMessage(hDlg, nIDDlgItem, LB_ADDSTRING, 0, (LPARAM)s);
}

VOID RemoveStringFromListBox(HWND hDlg, int nIDDlgItem, char *s)
{
	SendDlgItemMessage(hDlg, nIDDlgItem, LB_DELETESTRING, 0, (LPARAM)s);
}

VOID AddUSBItem(HWND hwnd, char *s)
{
	AddStringToListBox(hwnd, IDC_USBDEVLIST, s);
}

VOID DeleteUSBItem(HWND hwnd, char *s)
{
	RemoveStringFromListBox(hwnd, IDC_USBDEVLIST, s);
}

VOID getUSBlist(HWND hwnd)
{
	return;
}

VOID InitPreferencesDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PREFDIALOG),
					hwnd, PrefDialogProcedure) == -1) {
		MessageBox(hwnd, "Preferences dialog failed!", "Error!", MB_OK | MB_ICONERROR);
	}
}

VOID InitPasswordDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PWDDIALOG),
					hwnd, PwdDialogProcedure) == -1) {
		MessageBox(hwnd, "Password dialog failed!", "Error!", MB_OK | MB_ICONERROR);
	}
}

VOID InitUSBDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_USBDIALOG),
					hwnd, USBDialogProcedure) == -1) {
		MessageBox(hwnd, "USB dialog failed!", "Error!", MB_OK | MB_ICONERROR);
	}
}

VOID InitAboutDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUTDIALOG),
					hwnd, AboutDialogProcedure) == -1) {
		MessageBox(hwnd, "About dialog failed!", "Error!", MB_OK | MB_ICONERROR);
	}
}

VOID InitEmailDialog(HWND hwnd)
{
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EMAILDIALOG),
					hwnd, EmailDialogProcedure) == -1) {
		MessageBox(hwnd, "Email dialog failed!", "Error!", MB_OK | MB_ICONERROR);
	}
}

BOOL isValidDomain(char *str, char SEPARATOR)
{
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

BOOL parsePrefDialogFields(HWND hwnd)
{
	char *tmp = NULL;
	
	GetFieldText(hwnd, IDC_SERVERURLFIELD, &tmp);
	if (!tmp) {
		MessageBox(hwnd, "SMTP server field is empty!", "Error!", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	SMTP_SERVER = realloc(NULL, strlen(tmp)+1);
	strncpy(SMTP_SERVER, tmp, strlen(tmp)+1);
	free(tmp);
	tmp = NULL;
	
	GetFieldText(hwnd, IDC_PORTFIELD, &tmp);
	if (!tmp) {
		MessageBox(hwnd, "SMTP network port field is empty!", "Error!", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	} else if (strlen(tmp) > 5) {
		free(tmp);
		MessageBox(hwnd, "Invalid SMTP network port number!", "Error!", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	PORT = 0;
	UINT c;
	for (size_t i = 0; i < strlen(tmp) ; i++) {
		c = tmp[i] - '0';
		PORT = c*((UINT)pow(10, (strlen(tmp) - (i+1)))) + PORT;
	}
	free(tmp);

	if (PORT > 65535) {
		MessageBox(hwnd, "Invalid SMTP network port number!", "Error!", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

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
	FROM = realloc(NULL, strlen(tmp)+1);
	strncpy(FROM, tmp, strlen(tmp)+1);
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
	TO = realloc(NULL, strlen(tmp)+1);
	strncpy(TO, tmp, strlen(tmp)+1);
	free(tmp);
	tmp = NULL;
	
	GetFieldText(hwnd, IDC_CCFIELD, &tmp);
	if (!tmp) {
		CC = malloc(2);
		snprintf(CC, 1, "");
	}

	if (ValidEmailCheck) { 
		if (!isValidDomain(tmp, ';')) {
			MessageBox(hwnd, "Invalid e-mail address on CC field!", "Error!", MB_OK | MB_ICONEXCLAMATION);
			free(tmp);
			return FALSE;
		}
	}
	CC = realloc(NULL, strlen(tmp)+1);
	strncpy(CC, tmp, strlen(tmp)+1);
	free(tmp);
	tmp = NULL;
	
	GetFieldText(hwnd, IDC_SUBJECTFIELD, &tmp);
	if (!tmp) {
		if (MessageBox(hwnd, "Are you sure you don't want to have a Subject in your e-mail?", 
			"No Subject", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
			SUBJECT = malloc(2);
			snprintf(SUBJECT, 1, "");
		} else {
			return FALSE;
		}
	} else {
		SUBJECT = realloc(NULL, strlen(tmp)+1);
		strncpy(SUBJECT, tmp, strlen(tmp)+1);
		free(tmp);
		tmp = NULL;
	}
	
	
	return TRUE;
}

INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			return TRUE;
		case WM_COMMAND:
			switch (wParam) {
				case IDOK:
					EndDialog(hwnd, IDOK);
					break;
			}
			break;
		case WM_KEYDOWN:
			switch (wParam) {
				case VK_ESCAPE:
					EndDialog(hwnd, VK_ESCAPE);
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
			return TRUE;
		case WM_COMMAND:
			switch (wParam) {
				case IDOK:
					{
						ClearPwd();
						char *tmp;
						GetFieldText(hwnd, IDC_PWDFIELD, &tmp);
						if (!tmp) {
							MessageBox(hwnd, "No password entered", "Error!", MB_OK | MB_ICONEXCLAMATION);
						} else {
							pass = calloc(strlen(tmp)+1, 1);
							strncpy(pass, tmp, strlen(tmp)+1);
							free(tmp);
							EndDialog(hwnd, wParam);
						}
					}
					break;
				case IDCANCEL:
					ClearPwd();
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
			CheckDlgButton(hwnd, IDC_CHECKVALIDEMAIL, (ValidEmailCheck==TRUE)?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_CHECKUSBREFRESH, (USBRefresh==TRUE)?BST_CHECKED:BST_UNCHECKED);
			return TRUE;
		case WM_COMMAND:
			switch (wParam) {
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
	switch (msg) {
		case WM_INITDIALOG:
			AddUSBItem(hwnd, "TEST1");
			AddUSBItem(hwnd, "Test2");
			AddUSBItem(hwnd, "test3");
			getUSBlist(hwnd);
			return TRUE;
		case WM_COMMAND:
			switch (wParam) {
				case IDOK:
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

INT_PTR CALLBACK EmailDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND FROM_ttip, TO_ttip, CC_ttip, SUBJECT_ttip, BODY_ttip;
	switch (msg) {
		case WM_INITDIALOG:
			FROM_ttip = CreateToolTip(IDC_FROMFIELD, hwnd, "E-mail address of sender");
			TO_ttip = CreateToolTip(IDC_TOFIELD, hwnd, "E-mail address of recipient");
			CC_ttip = CreateToolTip(IDC_CCFIELD, hwnd, "Group of addresses to send to. Multiple e-mails are seperated with ';'");
			SUBJECT_ttip = CreateToolTip(IDC_SUBJECTFIELD, hwnd, "Subject of the e-mail");
			BODY_ttip = CreateToolTip(IDC_MESSAGEFIELD, hwnd, "Body of the e-mail");
			return TRUE;
		case WM_COMMAND:
			switch (wParam) {
				case IDOK:
					ClearEmailData();
					if (parseEmailDialogFields(hwnd))
						EndDialog(hwnd, wParam);
					else
						ClearEmailData();
					break;
				case IDCANCEL:
					ClearEmailData();
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
					WS_CHILD | WS_VISIBLE | TBS_ENABLESELRANGE,
					30, 200, 200, 30, hwnd, (HMENU)IDC_TIMETRACK, g_hInst, NULL);
				if (!time_track) {
					MessageBox(NULL, "Trackbar creation failed!", "Error!", MB_ICONERROR | MB_OK);
					return -2;
				}	
				
				SendMessage(time_track, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(T_MIN, T_MAX));
				SendMessage(time_track, TBM_SETPAGESIZE, 0, (LPARAM)4);
				SendMessage(time_track, TBM_SETSEL, (WPARAM)FALSE, (LPARAM)MAKELONG(T_MIN, T_MAX));
				SendMessage(time_track, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)T_MIN);
				
				SetFocus(time_track);
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
					InitU2M(onoff);
					break;
				case IDC_EMAILBUTTON:
					InitEmailDialog(hwnd);
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

	PORT = 0;
	pass = CC = TO = FROM = SUBJECT = BODY = SMTP_SERVER = NULL;
	g_hInst = hInstance;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.lpfnWndProc = MainWindowProcedure;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USB2MAILICONLARGE));
	wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USB2MAILICONSMALL));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_INACTIVEBORDER);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
	wc.lpszClassName = szClassName;

	if (!RegisterClassEx(&wc)) {
			MessageBox(NULL, "Window Registration Failed!", "Error!",
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
