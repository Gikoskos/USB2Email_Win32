#include "usb2mail.h"


void InitU2M(BOOL onoff)
{
	if (onoff) {
		printf("FROM: %s\nTO: %s\nCC: %s\nSUBJECT: %s\nBODY: %s\n",
			FROM, TO, CC, SUBJECT, BODY);
		printf("pass: %s\n", pass);
	} else {
	}
}