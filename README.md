# USB2Email
## Overview

With USB2Email you can send an e-mail whenever a certain USB device is inserted on the computer.
Insert your USB device to the PC, find it and choose it from the list, configure your server's
SMTP settings and the e-mail you want to send and start the service.

More detailed instructions about different functionalities in the application, you will find in
the Help dialog.

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

Also the MSVC libcurl depends on WinSSL instead of OpenSSL and all the libraries and the final executable
link statically with the msvcrt libraries (\MT) meaning that the Visual Studio executable can run without
the Visual C++ Redistributable.

## Libs used

libquickmail (depends on libcurl)

libconfuse

Winapi

### License

see LICENSE

2016 <georgekoskerid@outlook.com>
