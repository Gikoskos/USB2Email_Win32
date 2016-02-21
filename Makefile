CC = gcc
DEBUG = -g -DDEBUG -DUNICODE -D_UNICODE
CFLAGS = -Wall -std=c11
INC_WARN_LEVEL = -Wextra -pedantic
OBJ = -o
DBG = build/debug.exe
RLS = build/USB2EMAILWin32.exe
DWARF2 = -ggdb
LINKER = -lsetupapi -lcomctl32 -lgdi32 -lconfuse -lquickmail
RLS_FLAGS = -mwindows -O2
WINDOW_SOURCE = U2MWin32.c
CONFIG_SOURCE = U2MConf.c
USB2MAIL_SOURCE = U2MModule.c
USBIDS_SOURCE = find_usb.c usb_ids.c
USBIDS_OBJ = find_usb.o usb_ids.o
RESOURCE = resources.rc
RES_OBJ = res.o
WXS = Setup.wxs
WIXOBJ = Setup.wixobj

dbg: clean compile_resource compile_usbids debug

rls: clean compile_resource compile_usbids release

usb: $(USBTEST_SOURCE)
	$(CC) $(CFLAGS) $(DEBUG) $(DWARF2) $^ $(LINKER)

debug: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE) $(CONFIG_SOURCE) $(USBIDS_OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(DBG) $(DEBUG) $(DWARF2) $^ $(RES_OBJ) $(LINKER)

release: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE) $(CONFIG_SOURCE) $(USBIDS_OBJ)
	$(CC) $(CFLAGS) $(RLS_FLAGS) $(OBJ) $(RLS) $^ $(RES_OBJ) $(LINKER)
	
compile_resource:
	cd resources & windres $(RESOURCE) ..\$(RES_OBJ) & cd ..

compile_usbids:
	$(CC) $(CFLAGS) -c $(USBIDS_SOURCE)

installer: candle light

candle: $(WXS)
	candle.exe <

light: $(WIXOBJ)
	light.exe <

ml: smtp-tls.c
	$(CC) $(CFLAGS) $^ -lcurl


.PHONY:
clean: cleanobj cleanexe

cleanobj:
	@del *.o *.wixobj *.wixpdb

cleanexe:
	@del build\*.exe
