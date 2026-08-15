

I find building on the Windows platform challenging because of the number of issues that you have to contend with.

This document is specially here to talk and to serve as a reminder to myself what the ENVIRONMENT looks like as a standard for development.

Assumption:

Installation using a MingGW (not Cygwin).  The reason for this is quite simple.  The design choice had to do more with the portability of the binaries created with a compiler on Windows.  I found it easier to use MingGW instead of Cygwin.  It *MIGHT* be possible for me to have gone to 11 and tweaked libaries, but I choose to pivot and using MingGW.

management of a few things.

* gcc (MinGW-W64 x86_64-msvcrt-posix-seh, built by Brecht Sanders, r3) 15.2.0

* coreutils-5.3.0.exe

* make-3.81.exe

* gcc-arm-none-eabi-10.3-2021.10-win32.exe

  

For MingW:
I installed it in C:\Program Files\GNU\bin

For GNU:

Assumes installation of a few basic tools

I just install in c:\program files\GNU

Yes the binaries go in the "bin" folder.

This has not changed in a while:
gcc-arm-none-eabi-10.3-2021.10-win32.exe

default installation location is: C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin>



side note:  When editing path

Hit Window-S (not windows R) ; type CMD; right click on it and run as admin.

Then issue "rundll32.exe sysdm.cpl,EditEnvironmentVariables"

That's the shortest path I know of to getting you to a place to edit the fundamentals.  Running this as admin is essential if you want to edit the systems PATH as opposed to the users PATH variable.