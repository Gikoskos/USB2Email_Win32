#include "usb_ids.h"

void *bsearch(const void *key, const void *base, size_t num, size_t size,
              int (*cmp)(const void *key, const void *elt));


static int cmp(const void *vp, const void *vq)
{
	const UsbDevStruct *p = (UsbDevStruct*)vp;
	const UsbDevStruct *q = (UsbDevStruct*)vq;

	if (p->VendorID < q->VendorID)
		return -1;
	if (p->VendorID > q->VendorID)
		return +1;
	if (p->DeviceID < q->DeviceID)
		return -1;
	if (p->DeviceID > q->DeviceID)
		return +1;
	return 0;
}

UsbDevStruct *UsbFind(unsigned long vendor, unsigned long device)
{
	UsbDevStruct key;

	key.VendorID = vendor;
	key.DeviceID = device;
	return (UsbDevStruct*)bsearch(&key, UsbList, UsbLength, sizeof *UsbList, cmp);
}

int UsbListIsSorted(void)
{
	size_t i;

	for (i = 1; i < UsbLength; i++)
		if (cmp(&UsbList[i - 1], &UsbList[i]) > 0)
			return 0;
	return 1;
}

/*
 * A generic implementation of binary search for the Linux kernel
 *
 * Copyright (C) 2008-2009 Ksplice, Inc.
 * Author: Tim Abbott <tabbott@ksplice.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 */
void *bsearch(const void *key, const void *base, size_t num, size_t size,
              int (*cmp)(const void *key, const void *elt))
{
    size_t start = 0, end = num;
    int result;

    while (start < end) {
        size_t mid = start + (end - start) / 2;

        result = cmp(key, base + mid * size);
        if (result < 0)
                end = mid;
        else if (result > 0)
                start = mid + 1;
        else
                return (void *)base + mid * size;
    }

    return NULL;
}