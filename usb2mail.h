/******************************************
*                usb2mail.h                *
*        George Koskeridis (C)2015         *
 ******************************************/

#pragma once

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <math.h>
#include "resources\resource.h"

#define ATTRIB(x) __attribute__((x))
#define ATTRIB_UNUSED ATTRIB(unused)

extern char *pass, *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER, *USBdev, *USER, *PORT_STR;
extern UINT TIMEOUT;

extern BOOL ValidEmailCheck;
extern BOOL USBRefresh;
volatile extern BOOL onoff;


BOOL InitU2MThread();
VOID fillUSBlist(HWND hwnd);
VOID AddUSBItem(HWND hwnd, char *s);
