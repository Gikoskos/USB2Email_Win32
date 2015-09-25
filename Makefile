CC = gcc
DEBUG = -g -DDEBUG
CFLAGS = -Wall -std=c11
INC_WARN_LEVEL = -Wextra -pedantic
OBJ = -o
DBG = debug.exe
RLS = USB2EMAILwin32.exe
DWARF2 = -ggdb
LINKER = -lsetupapi -lcurl -lcomctl32
RLS_FLAGS = -mwindows
WINDOW_SOURCE = U2MWin32.c
USB2MAIL_SOURCE = usblist.c
USB_SOURCE = usbtest.c
RESOURCE = resources.rc
RES_OBJ = res.o

dbg: clean compile_resource debug

rls: compile_resource release

usb: $(USB_SOURCE)
	$(CC) $(CFLAGS) $(DEBUG) $(DWARF2) $^

debug: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE)
	$(CC) $(CFLAGS) $(DEBUG) $(DWARF2) $^ $(RES_OBJ) $(LINKER) $(OBJ) $(DBG)

release: $(WINDOW_SOURCE) $(USB2MAIL_SOURCE)
	$(CC) $(CFLAGS) $^ $(RES_OBJ) $(LINKER) $(RLS_FLAGS) $(OBJ) $(RLS)
	
compile_resource:
	cd resources & windres $(RESOURCE) ..\$(RES_OBJ) & cd ..

ml: smtp-tls.c
	$(CC) $(CFLAGS) $^ -lcurl


.PHONY:
clean: cleanobj cleanexe

cleanobj:
	@del *.o

cleanexe:
	@del *.exe