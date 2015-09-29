/**********************************************
* Struct array with USB devices parsed from    *
* this http://www.linux-usb.org/usb.ids list   *
* George Koskeridis for the Public Domain 2015 *
 **********************************************/

#ifndef USB_IDS_H
#define USB_IDS_H

typedef struct {
	unsigned short VendorID;
	unsigned short DeviceID;
	char *Vendor;
	char *Device;
} UsbDevStruct;

extern UsbDevStruct UsbList[];
extern size_t UsbLength;

UsbDevStruct *UsbFind(long vendor, long device);
int UsbListIsSorted(void);

#endif
