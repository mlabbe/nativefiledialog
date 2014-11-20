/*
  Native File Dialog

  http://www.frogtoss.com/labs
*/

#include <string.h>
#include <gtk/gtk.h>
#include "nfd.h"
#include "nfd_common.h"

nfdresult_t NFD_OpenDialog( const char *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{    
    GtkWidget *dialog;
    /*    GtkFileFilter *filter; */
    nfdresult_t result;

    /* todo: check if it is already initted */
    gtk_init( NULL, NULL );

    /*AddFiltersToDialog();*/

    dialog = gtk_file_chooser_dialog_new( "Open File",
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          "_Cancel", GTK_RESPONSE_CANCEL,
                                          "_Open", GTK_RESPONSE_ACCEPT,
                                          NULL );

    result = NFD_CANCEL;
    if ( gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
    {
        char *filename;

        filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) );

        {
            size_t len = strlen(filename);
            *outPath = NFDi_Malloc( len + 1 );
            memcpy( *outPath, filename, len + 1 );
            if ( !*outPath )
            {
                g_free( filename );
                gtk_widget_destroy(dialog);
                return NFD_ERROR;
            }
        }
        g_free( filename );

        result = NFD_OKAY;
    }

    gtk_widget_destroy(dialog);

    return result;
}

nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t *outPaths )
{
    
    return NFD_ERROR;
}

nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    return NFD_ERROR;
}
