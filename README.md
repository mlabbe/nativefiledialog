# Native File Dialog #

A tiny C library that invokes native file open and save dialogs.  Write dialog code once and have it pop up native dialogs on all supported platforms.  Avoid linking large dependencies like wxWidgets.

Features:

 - Lean C API, static library -- no ObjC, no C++ necessary.
 - Zlib license.
 - Consistent UTF-8 support on all platforms.
 - Simple, universal file filter syntax.
 - Multiple selection support.
 - 64-bit and 32-bit friendly.
 - GCC, Clang and Visual Studio supported.
 - No unnecessary dependencies (GTK+3 on Linux)
 - Support for Vista's modern `IFileDialog` on Windows.
 - Support for Cocoa APIs on OS X.
 - GTK+3 dialog on Linux.
 - Tested, works alongside [www.libsdl.org](SDL2) on all platforms, for the game developers out there.

## Example Usage ##

    ```C
    #include <nfd.h>
    #include <stdio.h>
    #include <stdlib.h>

    int main( void )
    {
        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_OpenDialog( NULL, NULL, &outPath );
        
        if ( result == NFD_OKAY ) {
            puts("Success!");
            puts(outPath);
            free(outPath);
        }
        else if ( result == NFD_CANCEL ) {
            puts("User pressed cancel.");
        }
        else {
            printf("Error: %s\n", NFD_GetError() );
        }

        return 0;
    }
    ```

See `NFD.h` for more options.

## Building ##

## Usage ##

See `NFD.h` for API calls.  See `tests/*.c` for example code.

# Copyright and Credit #

Copyright &copy; 2014 [Frogtoss Games](http://www.frogtoss.com), Inc.
File `LICENSE` covers all files in this repo.

Native File Dialog by Michael Labbe
<mike@frogtoss dot com>
