CC = gcc
DEBUG = -g -DDEBUG
CFLAGS = -Wall -DUNICODE -D_UNICODE -static-libgcc
INC_WARN_LEVEL = -Wextra -pedantic
OBJ = -o
DBG = build/USB2Email/debug.exe
RLS = build/USB2Email/USB2Email.exe
DWARF2 = -ggdb

LINKER = build/libU2MUsbIDs_dll.a build/libquickmail.dll.a build/libconfuse.a -lsetupapi -lversion -lgdi32 -lole32 -ladvapi32 -lshell32 -lcomctl32
RLS_FLAGS = -mwindows -O1

WINDOW_SOURCE = U2MWin32.c
CONFIG_SOURCE = U2MConf.c
USB2MAIL_SOURCE = U2MModule.c
USBIDS_SOURCE = find_usb.c usb_ids.c

MAIN_RES = main_res.rc

EN_RES = en_resources.rc
GR_RES = gr_resources.rc

WXS = U2MInstaller.wxs
WIXOBJ = U2MInstaller.wixobj


dbg: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE) $(CONFIG_SOURCE) 
	$(CC) $(CFLAGS) $(OBJ) $(DBG) $(DEBUG) $(DWARF2) main_res.o $^ $(LINKER)

rls: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE) $(CONFIG_SOURCE)
	$(CC) $(CFLAGS) $(RLS_FLAGS) $(OBJ) $(RLS) main_res.o $^ $(LINKER)

all_extern: clean locale_dlls usbids_dll compile_icon_res

locale_dlls: compile_en_dll compile_gr_dll

compile_en_dll: compile_en_resources
	$(CC) -shared -o build/USB2Email/U2MLocale_En.dll en_resources.o

compile_gr_dll: compile_gr_resources
	$(CC) -shared -o build/USB2Email/U2MLocale_Gr.dll gr_resources.o

compile_en_resources:
	cd resources & windres $(EN_RES) ..\en_resources.o & cd ..

compile_gr_resources: 
	cd resources & windres $(GR_RES) ..\gr_resources.o & cd ..

compile_icon_res:
	cd resources & windres $(MAIN_RES) ..\main_res.o & cd ..

compile_usbids:
	$(CC) $(CFLAGS) -c $(USBIDS_SOURCE)

usbids_dll: compile_usbids
	$(CC) -shared -o build/USB2Email/U2MUsbIDs.dll find_usb.o usb_ids.o -Wl,--out-implib,build/libU2MUsbIDs_dll.a

installer: candle light

candle: $(WXS)
	candle.exe $^

light: $(WIXOBJ)
	light.exe $^

.PHONY:
clean: cleanobj cleanexe

cleanobj:
	@del *.o *.wixobj *.wixpdb *.a

cleanexe:
	@del build\USB2Email\*.exe build\USB2Email\U2MLocale*.dll build\USB2Email\U2MUsbIDs.dll build\libU2MUsbIDs_dll.a
