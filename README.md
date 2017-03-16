# USB2Email
## Overview

With USB2Email you can send an e-mail whenever a certain USB device is inserted on the computer.
Insert your USB device to the PC, find it and choose it from the list, configure your server's
SMTP settings and the e-mail you want to send and start the service.

USB2Email logs all the times the USB device is detected and writes down to log files a timestamp
of when the USB was inserted.

At the moment the installer isn't finished so the user has to delete the registry key created
by this application manually, in case he wants to remove this application completely from the PC.

The registry key to be deleted is

    HKEY_CURRENT_USER\Software\USB2Email
Detailed instructions about different functionalities in the application, you will find in
the Help dialog.

![Alt Text](http://i.imgur.com/eU5RRgh.gif)

## TODO

* Implement WiX installer

## Language support

English

Greek

If you want to support other languages e-mail me.

## Building

The Makefile compiles with mingw-w64. The executable for mingw-w64 is on the build\USB2Email directory.
The following command compiles a full-featured executable and the DLLs needed:

    mingw32-make.exe all_extern & mingw32-make.exe rls

There's also my Visual Studio 2013 project files in the repo. On the MSVC version, I don't compile the USB IDs 
and functions in a seperate DLL; they are packaged with the final executable.

Also all the external libs on the MSVC version are linked statically with the 
final executable and msvcrt (\MT flag) meaning that the Visual Studio executable can run without
the Visual C++ Redistributable (standalone .exe).

## Libs used

libquickmail (depends on libcurl)

libconfuse

Winapi

### License

see the folder licenses

USB2Email (C) 2016 <georgekoskerid@outlook.com>
