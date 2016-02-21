CC = gcc
DEBUG = -g -DDEBUG
CFLAGS = -Wall -std=c11 -DUNICODE -D_UNICODE -static-libgcc
INC_WARN_LEVEL = -Wextra -pedantic
OBJ = -o
DBG = build/debug.exe
RLS = build/USB2Email.exe
DWARF2 = -ggdb

LINKER = -L. -lU2MUsbIDs_dll -lU2MLocale_Gr -lU2MLocale_En -lsetupapi -lcomctl32 -lgdi32 -lconfuse -lquickmail
RLS_FLAGS = -mwindows -O2

WINDOW_SOURCE = U2MWin32.c
CONFIG_SOURCE = U2MConf.c
USB2MAIL_SOURCE = U2MModule.c
USBIDS_SOURCE = find_usb.c usb_ids.c

ICON_RES = icon_res.rc

EN_SOURCE = U2MLocale_en.c
GR_SOURCE = U2MLocale_gr.c
EN_RES = en_resources.rc
GR_RES = gr_resources.rc

WXS = Setup.wxs
WIXOBJ = Setup.wixobj


dbg: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE) $(CONFIG_SOURCE) 
	$(CC) $(CFLAGS) $(OBJ) $(DBG) $(DEBUG) $(DWARF2) $^ icon_res.o $(LINKER)

rls: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE) $(CONFIG_SOURCE)
	$(CC) $(CFLAGS) $(RLS_FLAGS) $(OBJ) $(RLS) $^ icon_res.o $(LINKER)

all_extern: locale_dlls usbids_dll clean compile_icon_res

locale_dlls: compile_en_dll compile_gr_dll

compile_en_dll: compile_en_U2MLocale compile_en_resources
	$(CC) -shared -o U2MLocale_En.dll en_resources.o U2MLocale_En.o
#    -Wl,--out-implib,libU2MLocale_En_dll.a

compile_gr_dll: compile_gr_U2MLocale compile_gr_resources
	$(CC) -shared -o U2MLocale_Gr.dll gr_resources.o U2MLocale_Gr.o
#    -Wl,--out-implib,libU2MLocale_Gr_dll.a

compile_en_U2MLocale: $(EN_SOURCE)
	$(CC) $(CFLAGS) -c $^

compile_en_resources:
	cd resources & windres $(EN_RES) ..\en_resources.o & cd ..

compile_gr_U2MLocale: $(GR_SOURCE)
	$(CC) $(CFLAGS) -c $^

compile_gr_resources: 
	cd resources & windres $(GR_RES) ..\gr_resources.o & cd ..

compile_icon_res:
	cd resources & windres $(ICON_RES) ..\icon_res.o & cd ..

compile_usbids:
	$(CC) $(CFLAGS) -c $(USBIDS_SOURCE)

usbids_dll: compile_usbids
	$(CC) -shared -o U2MUsbIDs.dll find_usb.o usb_ids.o -Wl,--out-implib,libU2MUsbIDs_dll.a

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
