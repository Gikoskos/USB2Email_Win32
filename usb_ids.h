/**********************************************
* Struct array with USB devices parsed from    *
* this http://www.linux-usb.org/usb.ids list   *
* George Koskeridis for the Public Domain 2015 *
 **********************************************/

#ifndef USB_IDS_H
#define USB_IDS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned long VendorID;
	unsigned long DeviceID;
	char *Vendor;
	char *Device;
} UsbDevStruct;

extern __declspec(dllexport) UsbDevStruct UsbList[];
extern __declspec(dllexport) size_t UsbLength;

__declspec(dllexport) UsbDevStruct *UsbFind(unsigned long vendor, unsigned long device);
__declspec(dllexport) int UsbListIsSorted(void);

#ifdef __cplusplus
}
#endif

#endif
