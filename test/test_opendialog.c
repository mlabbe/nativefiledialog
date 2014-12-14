#include <nfd.h>

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <string.h>

/* this test should compile on all supported platforms */

int main( void )
{
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog( "png,jpg;pdf", NULL, &outPath );
    if ( result == NFD_OKAY )
    {
        puts("Success!");
        puts(outPath);
        free(outPath);
    }
    else if ( result == NFD_CANCEL )
    {
        puts("User pressed cancel.");
    }
    else 
    {
        printf("Error: %s\n", NFD_GetError() );
    }

	BApplication app("application/x-vnd.bacon");
	printf((const char *)strerror(app.InitCheck()));

    return 0;
}
