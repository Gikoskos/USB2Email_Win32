CC = gcc
DEBUG = -g -DDEBUG
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
USBLIST = find_usb.c usb_ids.c
USBTEST_SOURCE = usbtest.c
RESOURCE = resources.rc
RES_OBJ = res.o
WXS = Setup.wxs
WIXOBJ = Setup.wixobj

dbg: clean compile_resource debug

rls: compile_resource release

usb: $(USBTEST_SOURCE)
	$(CC) $(CFLAGS) $(DEBUG) $(DWARF2) $^ $(LINKER)

debug: $(USBLIST) $(WINDOW_SOURCE) $(USB2MAIL_SOURCE) $(CONFIG_SOURCE)
	$(CC) $(CFLAGS) $(OBJ) $(DBG) $(DEBUG) $(DWARF2) $^ $(RES_OBJ) $(LINKER)

release: $(USBLIST) $(WINDOW_SOURCE) $(USB2MAIL_SOURCE) $(CONFIG_SOURCE)
	$(CC) $(CFLAGS) $(RLS_FLAGS) $(OBJ) $(RLS) $^ $(RES_OBJ) $(LINKER)
	
compile_resource:
	cd resources & windres $(RESOURCE) ..\$(RES_OBJ) & cd ..

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