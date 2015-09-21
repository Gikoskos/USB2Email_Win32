CC = gcc
CFLAGS = -g -Wall -std=c11 -pedantic
OBJ = -o
DBG = debug.exe
RLS = release.exe
LINKER = -mwindows
WINDOW_SOURCE = U2MWin32.c
USB2MAIL_SOURCE = usb2mail.c
USB_SOURCE = usbtest.c
RESOURCE = resources.rc
RES_OBJ = res.o

dbg: compile_resource debug

rls: compile_resource release

usb: $(USB_SOURCE)
	$(CC) $(CFLAGS) $^ -lsetupapi

debug: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE)
	$(CC) $(CFLAGS) $^ $(RES_OBJ) $(OBJ) $(DBG)

release: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE)
	$(CC) $(CFLAGS) $^ $(RES_OBJ) $(LINKER) $(OBJ) $(RLS)
	
compile_resource:
	cd resources & windres $(RESOURCE) ../$(RES_OBJ) & cd ..

.PHONY:
clean: cleanobj cleanexe

cleanobj:
	@del *.o

cleanexe:
	@del *.exe