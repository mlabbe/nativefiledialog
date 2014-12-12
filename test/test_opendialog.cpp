#include <nfd.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef __HAIKU__
	#include <Application.h>
#endif

/* this test should compile on all supported platforms */

#if __HAIKU__
class App : public BApplication {
public:
	App() : BApplication("application/x-vnd.lh-TestOpenDialog") {};
	void ReadyToRun();
};

int main()
{
	App().Run();
}

void
App::ReadyToRun()
#else
int main( void )
#endif
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
#if __HAIKU__
	PostMessage(B_QUIT_REQUESTED);
#else
    return 0;
#endif
}
