#include <stdio.h>
#include <nfd.h>

/* this test should compile on all supported platforms */

int main( void )
{
    nfdchar_t *outPath = NULL;
    int result = NFD_OpenDialog( "*.pdf;*.jpg", "/Users/mlabbe/Desktop", &outPath );
    if ( result )
    {
        puts("Success!");
        puts(outPath);
    }
    else
    {
        printf("Failure: %s", NFD_GetError() );
    }

    return 0;
}
