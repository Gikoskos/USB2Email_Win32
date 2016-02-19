/******************************************
*                U2MWin32.c                *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "U2MWin32.h"

const char szClassName[] = "USB2EMailWin32";

HINSTANCE g_hInst;

/********************
*Input field raw data*
 ********************/
char *pass, *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER;
UINT PORT;

/*******************************
*Bools to check enabled controls*
 *******************************/
BOOL ValidEmailCheck = FALSE;
BOOL USBRefresh = FALSE;
BOOL USBdev_scan = FALSE;

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

UINT MAX_FAILED_EMAILS = 0;
UINT TIMEOUT;
UINT usb_idx;

/*Trackbar limits*/
#define T_MIN                     200
#define T_MAX                    2000

/*Flag for ValidEmailCheck*/
#define NO_SEPARATOR              '\0'


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

UINT CALLBACK RefreshUSBThread(LPVOID dat);

BOOL parseEmailDialogFields(HWND hwnd);
BOOL parsePrefDialogFields(HWND hwnd);
BOOL parsePwdField(HWND hwnd);
HWND WINAPI CreateBaloonToolTip(int toolID, HWND hDlg, LPTSTR pszText);
HWND WINAPI CreateTrackingToolTip(int toolID, HWND hDlg, LPTSTR pszText);
BOOL isValidDomain(char *str, char SEPARATOR);
VOID GetFieldText(HWND hwnd, int nIDDlgItem, char **str);
BOOL GetUSBListViewSelection(HWND hwnd);

VOID InitEmailDialog(HWND hwnd);
VOID InitAboutDialog(HWND hwnd);
VOID InitPasswordDialog(HWND hwnd);
VOID InitPreferencesDialog(HWND hwnd);
VOID InitHelpWindow(HWND hwnd);
VOID CenterChild(HWND hwnd);

VOID AddDeviceToUSBListView(HWND hDlg, char *dev_str, char *ven_str);
VOID DeleteDeviceFromUSBListView(HWND hDlg, int nIDDlgItem, char *s);
VOID DeleteAll();
VOID GetFieldText(HWND hwnd, int nIDDlgItem, char **str);

//@TODO: Implement ErrorCodes function
/*void ErrorCodes(int i)
{

}

char *ErrorCodeDlgTitles(int i)
{

}*/


VOID DeleteAll()
{
    ClearEmailData();
    ClearPwd();
    ClearPrefs();
}

VOID DeleteDeviceFromUSBListView(HWND hDlg, int nIDDlgItem, char *s)
{
    if (s)
        SendDlgItemMessage(hDlg, nIDDlgItem, LB_DELETESTRING, (WPARAM)0, (LPARAM)s);
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
        free(tmp2);
        MessageBox(hwnd, "SMTP network port field is empty!", "Error!", MB_OK | MB_ICONERROR);
        return TRUE;
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
    free(tmp2);

    if (PORT > 65535) {
        free(tmp1);
        MessageBox(hwnd, "Invalid SMTP network port number!", "Error!", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    SMTP_SERVER = realloc(NULL, strlen(tmp1)+1);
    snprintf(SMTP_SERVER, strlen(tmp1)+1, "%s", tmp1);
    free(tmp1);
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
    FROM = realloc(NULL, strlen(tmp)+1);
    snprintf(FROM, strlen(tmp)+1, "%s", tmp);
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
    TO = realloc(NULL, strlen(tmp)+1);
    snprintf(TO, strlen(tmp)+1, "%s", tmp);
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
        CC = realloc(NULL, strlen(tmp)+1);
        strncpy(CC, tmp, strlen(tmp)+1);
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

VOID AddDeviceToUSBListView(HWND hDlg, char *dev_str, char *ven_str)
{
    LVITEM dev;

    if (!dev_str)
        dev.pszText = "Unidentified vendor";
    else
        dev.pszText = dev_str;

    dev.mask = LVIF_TEXT | LVIF_STATE;
    dev.state = 0;
    dev.stateMask = 0;
    dev.iSubItem = 0;
    dev.iItem = usb_idx;
    ListView_InsertItem(GetDlgItem(hDlg, IDC_USBDEVLIST), &dev);
    if (!ven_str) {
        ListView_SetItemText(GetDlgItem(hDlg, IDC_USBDEVLIST), usb_idx, 1, "Unidentified device");
    } else {
        ListView_SetItemText(GetDlgItem(hDlg, IDC_USBDEVLIST), usb_idx, 1, ven_str);
    }
    usb_idx++;
}

BOOL GetUSBListViewSelection(HWND hwnd)
{
    INT usb_sel_idx = (INT)SendMessage(GetDlgItem(hwnd, IDC_USBDEVLIST), 
                                       LVM_GETNEXTITEM, -1, LVNI_SELECTED);

    if (usb_sel_idx == -1) return FALSE;

    usb_id_selection[0] = scanned_usb_ids[usb_sel_idx][0];
    usb_id_selection[1] = scanned_usb_ids[usb_sel_idx][1];
    return TRUE;
}

VOID CenterChild(HWND hwnd)
{
    HWND hwndOwner = GetParent(hwnd);
    RECT rc, rcDlg, rcOwner;
    INT final_x, final_y;

    GetWindowRect(hwndOwner, &rcOwner);
    GetWindowRect(hwnd, &rcDlg);
    CopyRect(&rc, &rcOwner);
    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
    OffsetRect(&rc, -rc.left, -rc.top);
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

    final_x = rcOwner.left + (rc.right / 2);
    final_y = rcOwner.top + (rc.bottom / 2);
    if (final_x < 0) final_x = 0;
    if (final_y < 0) final_y = 0;
    SetWindowPos(hwnd, HWND_TOP, final_x, final_y, 0, 0, SWP_NOSIZE);
}

UINT CALLBACK RefreshUSBThread(LPVOID dat)
{
    HWND hwnd = (HWND)dat;

    while (USBdev_scan) {
        Sleep(2000);
        SendMessageTimeout(hwnd, WM_COMMAND, 
                           MAKEWPARAM((WORD)IDUSBREFRESH, 0), 
                           (LPARAM)0, SMTO_NORMAL,
                           0, NULL);
    }
    return 0;
}

INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HICON about_usb_icon;

    switch (msg) {
        case WM_INITDIALOG:
            about_usb_icon = (HICON)LoadIcon(GetModuleHandle(NULL), 
                             MAKEINTRESOURCE(IDI_USB2MAILICONLARGE));

            SetDlgItemText(hwnd, IDC_ABOUT_BUILD, "USB2EMail Win32 "U2MWin32_VERSION_STR);
            SetDlgItemText(hwnd, IDC_ABOUT_COMPILER,"built with "COMPILER_NAME_STR" "COMPILER_VERSION_STR);
            SendDlgItemMessage(hwnd, IDUSB2MAIL, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)about_usb_icon);
            CenterChild(hwnd);
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
            if (SMTP_SERVER)
                SetDlgItemText(hwnd, IDC_SERVERURLFIELD, SMTP_SERVER);
            if (PORT) {
                char PORT_STR[9];
                snprintf(PORT_STR, 9, "%u", PORT);
                SetDlgItemText(hwnd, IDC_PORTFIELD, PORT_STR);
            }
            CheckDlgButton(hwnd, IDC_CHECKVALIDEMAIL, (ValidEmailCheck)?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECKUSBREFRESH, (USBRefresh)?BST_CHECKED:BST_UNCHECKED);
            SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 50));
            SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SAVECONFBUTTON:
                    saveConfFile();
                    return (INT_PTR)TRUE;
                case IDAPPLY:
                case IDOK:
                    ValidEmailCheck = IsDlgButtonChecked(hwnd, IDC_CHECKVALIDEMAIL);
                    USBRefresh = IsDlgButtonChecked(hwnd, IDC_CHECKUSBREFRESH);
                    ClearPrefs();
                    if (parsePrefDialogFields(hwnd)) {
                        if (LOWORD(wParam) == IDOK) {
                            EndDialog(hwnd, wParam);
                        }
                    } else {
                        ClearPrefs();
                    }
                    return (INT_PTR)TRUE;
                case IDCANCEL:
                    EndDialog(hwnd, wParam);
                    return (INT_PTR)TRUE;
            }
        case WM_HSCROLL:
            MAX_FAILED_EMAILS = (UINT)SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_GETPOS, (WPARAM)0, (LPARAM)0);
            break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK USBDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HBITMAP refresh_bitmap;
    LVCOLUMN vendCol, devcCol;
    INITCOMMONCONTROLSEX columnControlClass = {sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES};
    HANDLE refresh_usb_hnd ATTRIB_UNUSED = NULL;
    static INT temp_idx = -1;

    switch (msg) {
        case WM_INITDIALOG:
            refresh_bitmap = (HBITMAP)LoadImage(g_hInst, "icons\\refreshbitmap.bmp",
                                 IMAGE_BITMAP, 0, 0,    LR_DEFAULTSIZE | LR_LOADFROMFILE | LR_LOADTRANSPARENT);
            SendDlgItemMessage(hwnd, IDUSBREFRESH, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)refresh_bitmap);

            temp_idx = -1;
            InitCommonControlsEx(&columnControlClass);
            vendCol.mask = devcCol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            vendCol.iSubItem = 0;
            devcCol.iSubItem = 1;
            vendCol.cx = devcCol.cx = 150;
            vendCol.fmt = LVCFMT_LEFT;
            devcCol.fmt = LVCFMT_LEFT;
            vendCol.pszText = "Device";
            devcCol.pszText = "Vendor";
            ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_USBDEVLIST), LVS_EX_FULLROWSELECT);
            ListView_InsertColumn(GetDlgItem(hwnd, IDC_USBDEVLIST), 0, &vendCol);
            ListView_InsertColumn(GetDlgItem(hwnd, IDC_USBDEVLIST), 1, &devcCol);
            GetConnectedUSBDevs(hwnd, FILL_USB_LISTVIEW);
            CenterChild(hwnd);
            if (USBRefresh) {
                USBdev_scan = TRUE;
                refresh_usb_hnd = (HANDLE)_beginthreadex(NULL, 0, RefreshUSBThread, (LPVOID)hwnd, 0, NULL);
            }
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            usb_idx = 0;
            memset(usb_id_selection, 0, sizeof(UINT)*2);
            switch (LOWORD(wParam)) {
                case IDUSBREFRESH:
                    ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_USBDEVLIST));
                    DeleteScannedUSBIDs();
                    GetConnectedUSBDevs(hwnd, FILL_USB_LISTVIEW);
                    if (temp_idx != -1)
                        ListView_SetItemState(GetDlgItem(hwnd, IDC_USBDEVLIST), (UINT)temp_idx,
                                              LVIS_FOCUSED | LVIS_SELECTED, 0x000f);
                    return (INT_PTR)TRUE;
                case IDOK:
                    if (!GetUSBListViewSelection(hwnd)) {
                        MessageBox(hwnd, "No USB device selected!", "Error!", MB_ICONERROR | MB_OK);
                    } else {
                        DeleteScannedUSBIDs();
                        USBdev_scan = FALSE;
                        EndDialog(hwnd, wParam);
                    }
                    return (INT_PTR)TRUE;
                case IDCANCEL:
                    DeleteScannedUSBIDs();
                    USBdev_scan = FALSE;
                    EndDialog(hwnd, wParam);
                    return (INT_PTR)TRUE;
            }
            return (INT_PTR)TRUE;
        case WM_NOTIFY:
            switch (LOWORD(wParam)) {
                case IDC_USBDEVLIST:
                    switch (((LPNMHDR)lParam)->code) {
                        case NM_DBLCLK:
                            usb_idx = 0;
                            if (!GetUSBListViewSelection(hwnd)) {
                                memset(usb_id_selection, 0, sizeof(usb_id_selection)*2);
                                MessageBox(hwnd, "No USB device selected!", "Error!", MB_ICONERROR | MB_OK);
                            } else {
                                USBdev_scan = FALSE;
                                EndDialog(hwnd, wParam);
                            }
                            return (INT_PTR)TRUE;                            
                        case NM_CLICK:
                            temp_idx = (INT)SendMessage(GetDlgItem(hwnd, IDC_USBDEVLIST), 
                                                        LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED);
                            return (INT_PTR)TRUE;
                    }
            }
            break;
    }
    return (INT_PTR)FALSE;
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
                SetDlgItemText(hwnd, IDC_FROMFIELD, FROM);
            if (TO)
                SetDlgItemText(hwnd, IDC_TOFIELD, TO);
            if (CC)
                SetDlgItemText(hwnd, IDC_CCFIELD, CC);
            if (SUBJECT)
                SetDlgItemText(hwnd, IDC_SUBJECTFIELD, SUBJECT);
            if (BODY)
                SetDlgItemText(hwnd, IDC_MESSAGEFIELD, BODY);
            CenterChild(hwnd);
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
            CenterChild(hwnd);
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
    static HFONT mainwindowcontrol_font_big, mainwindowcontrol_font;

    switch (msg) {
        case WM_CREATE:
            {
                mainwindowcontrol_font_big = CreateFont(
                                24, 0, //height, width
                                0, 0, //escapement, orientation angles
                                FW_DONTCARE, //boldness 0-1000, 400 = normal, 700 = bold
                                FALSE, FALSE, FALSE, //italics, underline, strikeout
                                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, //charset, output precision
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //clipping, output quality
                                VARIABLE_PITCH, TEXT("Trebuchet MS"));

                mainwindowcontrol_font = CreateFont(18, 0, 0, 0, 550,
                                FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, 
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                VARIABLE_PITCH, TEXT("Trebuchet MS"));

                USBListButton = CreateWindowEx(0, "BUTTON", "Choose USB device",
                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                 30, 30, 200, 30, hwnd, (HMENU)IDC_CHOOSEUSBBUTTON,
                                 (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
                if (!USBListButton) {
                    MessageBox(NULL, "USB list button failed!", "Error!", MB_ICONERROR | MB_OK);
                    return -2;
                }
                SendMessage(USBListButton, WM_SETFONT, (WPARAM)mainwindowcontrol_font, (LPARAM)TRUE);

                EMAILButton = CreateWindowEx(0, "BUTTON", "Configure E-Mail to send",
                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                 300, 30, 200, 30, hwnd, (HMENU)IDC_EMAILBUTTON, 
                                 (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
                if (!EMAILButton) {
                    MessageBox(NULL, "E-mail button failed!", "Error!", MB_ICONERROR | MB_OK);
                    return -2;
                }
                SendMessage(EMAILButton, WM_SETFONT, (WPARAM)mainwindowcontrol_font, (LPARAM)TRUE);

                STARTSTOP = CreateWindowEx(0, "BUTTON", "Start",
                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                 300, 200, 200, 50, hwnd, (HMENU)IDC_STARTSTOP, 
                                 g_hInst, NULL);
                if (!STARTSTOP) {
                    MessageBox(NULL, "Start/Stop button failed!", "Error!", MB_ICONERROR | MB_OK);
                    return -2;
                }
                SendMessage(STARTSTOP, WM_SETFONT, (WPARAM)mainwindowcontrol_font_big, (LPARAM)TRUE);

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
                SendMessage(ttrack_label, WM_SETFONT, (WPARAM)mainwindowcontrol_font, (LPARAM)TRUE);

                SendMessage(time_track, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ttrack_struct);
                SendMessage(time_track, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(T_MIN, T_MAX));
                SendMessage(time_track, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)4);
                SendMessage(time_track, TBM_SETSEL, (WPARAM)FALSE, (LPARAM)MAKELONG(T_MIN, T_MAX));
                SendMessage(time_track, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)1000);
                UpdateWindow(hwnd);
            }
        case WM_MOUSEMOVE:
            break;
        case WM_PAINT:
            if ((hdc = BeginPaint(hwnd, &ps))) {
                EndPaint(hwnd, &ps);
            }
            break;
        /* custom message to use from the U2MThread */
        case WM_ENABLE_STARTSTOP:
            EnableWindow(STARTSTOP, !IsWindowEnabled(STARTSTOP));
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
#ifdef DEBUG
                    fprintf(stderr, "USB device %04x:%04x\n\n", usb_id_selection[0], usb_id_selection[1]);
#endif
                    break;
                case IDC_STARTSTOP:
                    if (onoff) {
                        EnableWindow(USBListButton, TRUE);
                        EnableWindow(EMAILButton, TRUE);
                        EnableWindow(time_track, TRUE);
                        EnableWindow(ttrack_label, TRUE);
                        onoff = FALSE;
                    } else {
                        if (InitU2MThread(hwnd)) {
                            EnableWindow(USBListButton, FALSE);
                            EnableWindow(EMAILButton, FALSE);
                            EnableWindow(time_track, FALSE);
                            EnableWindow(ttrack_label, FALSE);
                        }
                    }
                    SetWindowText(STARTSTOP, (!onoff)?"Start":"Stop");
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
            if (onoff) {
                MessageBox(hwnd, "Can't close the window.", 
                           "Service is running!", MB_ICONEXCLAMATION | MB_OK);
                break;
            }
                
#ifndef DEBUG
            if (MessageBox(hwnd, "Are you sure you want to quit?", 
                "Quiting...", MB_ICONASTERISK | MB_YESNO) == IDNO)
                break;
#endif
            DeleteAll();
            DeleteObject(mainwindowcontrol_font_big);
            DeleteObject(mainwindowcontrol_font);
            DestroyWindow(hwnd);
            break;
        case WM_HSCROLL:
            TIMEOUT = (UINT)SendMessage(time_track, TBM_GETPOS, (WPARAM)0, (LPARAM)0);
            break;
        case WM_DESTROY:
            DeleteAll();
            DeleteObject(mainwindowcontrol_font_big);
            DeleteObject(mainwindowcontrol_font);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;
    BOOL bRet = TRUE;
    RECT wrkspace_px;
    INT center_x, center_y;

    /*** Global initializations ***/
    PORT = 0;
    pass = CC = TO = FROM = SUBJECT = BODY = SMTP_SERVER = NULL;
    memset(usb_id_selection, 0, sizeof(UINT)*2);
    TIMEOUT = 1000;

    g_hInst = hInstance;
    usb_idx = 0;

    parseConfFile();

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

    /* Get the coords of the screen without the taskbar */
    SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&wrkspace_px, SPIF_SENDCHANGE);

    /* Coords to center the window in the center of the working area */
    center_x = (wrkspace_px.right - 550)/2;
    center_y = (wrkspace_px.bottom - 350)/2;
    
    if (!RegisterClassEx(&wc)) {
            MessageBox(NULL, "Main Window class registration Failed!", "Error!",
                       MB_ICONERROR | MB_OK);
        return -1;
    }

    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, szClassName, "USB2EMail Win32",
                          WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_EX_LAYERED,
                          center_x, center_y, 550, 350, NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, "Window creation failed!", "Error!", MB_ICONERROR | MB_OK);
        return -2;
    }

    InitCommonControls();
#ifdef DEBUG
    ShowWindow(hwnd, SW_SHOW);
    SetActiveWindow(hwnd);
#else
    AnimateWindow(hwnd, 200, AW_CENTER | AW_ACTIVATE);
#endif
    while (bRet) {
        bRet = GetMessage(&Msg, NULL, 0, 0);

        if (bRet == -1) {
            MessageBox(NULL, "Message queue error", "Error!", MB_ICONERROR | MB_OK);
            return -3;
        } else if (bRet == -2) {
            return -2;
        }

        UpdateWindow(hwnd);
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
