/******************************************
*              U2MLocale_gr.c              *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "U2MCommon.h"

//TCHAR __declspec(dllexport) *LocalizedMessage(int idx);

TCHAR __declspec(dllexport) *t_localized_message[] = {
    _T("Σφάλμα!"), //0
    _T("Η υπηρεσία τρέχει!"), //1
    _T("Message queue error"), //2
    _T("Window creation failed!"), //3
    _T("Main Window class registration Failed!"), //4
    _T("Quiting..."), //5
    _T("Are you sure you want to quit?"), //6
    _T("Can't close the window."), //7
    _T("Can't change e-mail while the service is running."), //8
    _T("Έναρξη"), //9
    _T("Σταματημός"), //10
    _T("Can't change USB device while the service is running."), //11
    _T("Can't change password while the service is running."), //12
    _T("Can't change preferences while the service is running."), //13
    _T("Trackbar label failed!"), //14
    _T("Set waiting interval between each scan of all USB devices in milliseconds"), //15
    _T("Trackbar failed!"), //16
    _T("Start/Stop button failed!"), //17
    _T("E-mail button failed!"), //18
    _T("Configure E-Mail to send"), //19
    _T("USB list button failed!"), //20
    _T("Choose USB device"), //21
    _T("Body of the e-mail"), //22
    _T("Subject of the e-mail"), //23
    _T("Group of addresses to send to. Multiple e-mails are seperated with ';'"), //24
    _T("E-mail address of recipient"), //25
    _T("E-mail address of sender"), //26
    _T("No USB device selected!"), //27
    _T("Συσκευή"), //28
    _T("Προμηθευτής"), //29
    _T("built with "COMPILER_NAME_STR" "COMPILER_VERSION_STR), //30
    _T("USB2EMail Win32 "U2MWin32_VERSION_STR), //31
    _T("Unidentified device"), //32
    _T("Unidentified vendor"), //33
    _T("No password entered"), //34
    _T("No e-mail body"), //35
    _T("Are you sure you want to send a blank message?"), //36
    _T("No Subject"), //37
    _T("Are you sure you don't want to have a Subject in your e-mail?"), //38
    _T("Invalid e-mail address on CC field!"), //39
    _T("Invalid e-mail address on TO field!"), //40
    _T("TO field is empty!"), //41
    _T("Invalid e-mail address on FROM field!"), //42
    _T("FROM field is empty!"), //43
    _T("Invalid SMTP network port number!"), //44
    _T("SMTP network port field is empty!"), //45
    _T("SMTP server field is empty!"), //46
    _T("Help dialog failed to open!"), //47
    _T("Email dialog failed to open!"), //48
    _T("About dialog failed to open!"), //49
    _T("USB dialog failed to open!"), //50
    _T("Password dialog failed to open!"), //51
    _T("Preferences dialog failed to open!"), //52
    _T("You have to set an e-mail to send, first."), //53
    _T("Can't start service!"), //54
    _T("SMTP server domain not set."), //55
    _T("No USB device selected."), //56
    _T("No password set.") //57
};
