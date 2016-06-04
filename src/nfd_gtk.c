/*
  Native File Dialog

  http://www.frogtoss.com/labs
*/

#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <gtk/gtk.h>
#include "nfd.h"
#include "nfd_common.h"

const char INIT_FAIL_MSG[] = "gtk_init_check failed to initilaize GTK+";

/* helper function implementing reliable writes of specific length in
 * POSIX environments. Deals with signal interruptions and short writes. */
static
int NFDi_write(int fd, size_t sz, void const *buf)
{
	int rc;
	size_t tot = 0;
	char const * const p = buf;
	do {
		int write_errno;
		do {
			rc = write(fd, (p + tot), (sz - tot));
			if( 0 > rc ) {
				write_errno = errno;
				switch( write_errno ) {
				case EAGAIN:
					usleep(100);
				case EINTR:
					rc = 0;
				default:
					break;
				}
			}
		} while( 0 > rc );
		if( 0 > rc ) {
			return write_errno;
		} else
		if( (0 == rc) && (tot < sz) ) {
			return ESHUTDOWN;
		}
		tot += rc;
	} while( tot < sz );
	return 0;
}
/* helper function implementing reliable reads of specific length in
 * POSIX environments. Deals with signal interruptions and short reads. */
static
int NFDi_read(int fd, size_t sz, void *buf)
{
	int rc;
	size_t tot = 0;
	char * const p = buf;
	do {
		int read_errno;
		do {
			rc = read(fd, (p + tot), (sz - tot));
			if( 0 > rc ) {
				read_errno = errno;
				switch( read_errno ) {
				case EAGAIN:
					usleep(100);
				case EINTR:
					rc = 0;
				default:
					break;
				}
			}
		} while( 0 > rc );
		if( 0 > rc ) {
			return read_errno;
		} else
		if( (0 == rc) && (tot < sz) ) {
			return ESHUTDOWN;
		}
		tot += rc;
	} while( tot < sz );
	return 0;
}

static void AddTypeToFilterName( const char *typebuf, char *filterName, size_t bufsize )
{
    const char SEP[] = ", ";

    size_t len = strlen(filterName);
    if ( len != 0 )
    {
        strncat( filterName, SEP, bufsize - len - 1 );
        len += strlen(SEP);
    }
    
    strncat( filterName, typebuf, bufsize - len - 1 );
}

static void AddFiltersToDialog( GtkWidget *dialog, const char *filterList )
{
    GtkFileFilter *filter;
    char typebuf[NFD_MAX_STRLEN] = {0};
    const char *p_filterList = filterList;
    char *p_typebuf = typebuf;
    char filterName[NFD_MAX_STRLEN] = {0};
    
    if ( !filterList || strlen(filterList) == 0 )
        return;

    filter = gtk_file_filter_new();
    while ( 1 )
    {
        
        if ( NFDi_IsFilterSegmentChar(*p_filterList) )
        {
            char typebufWildcard[NFD_MAX_STRLEN];
            /* add another type to the filter */
            assert( strlen(typebuf) > 0 );
            assert( strlen(typebuf) < NFD_MAX_STRLEN-1 );
            
            snprintf( typebufWildcard, NFD_MAX_STRLEN, "*.%s", typebuf );
            AddTypeToFilterName( typebuf, filterName, NFD_MAX_STRLEN );
            
            gtk_file_filter_add_pattern( filter, typebufWildcard );
            
            p_typebuf = typebuf;
            memset( typebuf, 0, sizeof(char) * NFD_MAX_STRLEN );
        }
        
        if ( *p_filterList == ';' || *p_filterList == '\0' )
        {
            /* end of filter -- add it to the dialog */
            
            gtk_file_filter_set_name( filter, filterName );
            gtk_file_chooser_add_filter( GTK_FILE_CHOOSER(dialog), filter );

            filterName[0] = '\0';

            if ( *p_filterList == '\0' )
                break;

            filter = gtk_file_filter_new();            
        }

        if ( !NFDi_IsFilterSegmentChar( *p_filterList ) )
        {
            *p_typebuf = *p_filterList;
            p_typebuf++;
        }

        p_filterList++;
    }
    
    /* always append a wildcard option to the end*/

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name( filter, "*.*" );
    gtk_file_filter_add_pattern( filter, "*" );
    gtk_file_chooser_add_filter( GTK_FILE_CHOOSER(dialog), filter );
}

static void SetDefaultPath( GtkWidget *dialog, const char *defaultPath )
{
    if ( !defaultPath || strlen(defaultPath) == 0 )
        return;

    /* GTK+ manual recommends not specifically setting the default path.
       We do it anyway in order to be consistent across platforms.

       If consistency with the native OS is preferred, this is the line
       to comment out. -ml */
    gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(dialog), defaultPath );
}

static nfdresult_t NFDi_pathset_create_from_GSList(
	GSList *fileList,
	size_t *outBufSize,
	nfdpathset_t *pathSet )
{
    size_t bufSize = 0;
    GSList *node;
    nfdchar_t *p_buf;
    size_t count = 0;
    
    assert(fileList);
    assert(pathSet);

    pathSet->count = (size_t)g_slist_length( fileList );
    assert( pathSet->count > 0 );

    pathSet->indices = NFDi_Malloc( sizeof(size_t)*pathSet->count );
    if ( !pathSet->indices )
    {
        return NFD_ERROR;
    }

    /* count the total space needed for buf */
    for ( node = fileList; node; node = node->next )
    {
        assert(node->data);
        bufSize += strlen( (const gchar*)node->data ) + 1;
    }
    *outBufSize = bufSize;

    pathSet->buf = NFDi_Malloc( sizeof(nfdchar_t) * bufSize );

    /* fill buf */
    p_buf = pathSet->buf;
    for ( node = fileList; node; node = node->next )
    {
        nfdchar_t *path = (nfdchar_t*)(node->data);
        size_t byteLen = strlen(path)+1;
        ptrdiff_t index;
        
        memcpy( p_buf, path, byteLen );
        g_free(node->data);

        index = p_buf - pathSet->buf;
        assert( index >= 0 );
        pathSet->indices[count] = (size_t)index;

        p_buf += byteLen;
        ++count;
    }

    g_slist_free( fileList );
    
    return NFD_OKAY;
}

static void WaitForCleanup(void)
{
    while (gtk_events_pending())
        gtk_main_iteration();
}
                                 
/* public */

static
nfdresult_t NFDi_OpenDialog_F( const char *filterList,
                            const nfdchar_t *defaultPath,
			    size_t *outLen,
                            nfdchar_t **outPath )
{    
    GtkWidget *dialog;
    nfdresult_t result;

    if ( !gtk_init_check( NULL, NULL ) )
    {
        NFDi_SetError(INIT_FAIL_MSG);
        return NFD_ERROR;
    }

    dialog = gtk_file_chooser_dialog_new( "Open File",
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          "_Cancel", GTK_RESPONSE_CANCEL,
                                          "_Open", GTK_RESPONSE_ACCEPT,
                                          NULL );

    /* Build the filter list */
    AddFiltersToDialog(dialog, filterList);

    /* Set the default path */
    SetDefaultPath(dialog, defaultPath);

    result = NFD_CANCEL;
    if ( gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
    {
        char *filename;

        filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) );

        {
            size_t len = strlen(filename) + 1;
	    *outLen = len;
            *outPath = NFDi_Malloc(len);
            memcpy( *outPath, filename, len);
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

    WaitForCleanup();
    gtk_widget_destroy(dialog);
    WaitForCleanup();

    return result;
}

nfdresult_t NFD_OpenDialog( const char *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
	int status;
	int fd_pipe[2];
	size_t len;
	pid_t pid;

	if( pipe(fd_pipe) ) { return NFD_ERROR; }
	pid = fork();
	if( 0 > pid ) {
		close(fd_pipe[0]);
		close(fd_pipe[1]);
		return NFD_ERROR;
	}
	if( !pid ) {
		nfdresult_t result;
		nfdchar_t *buf = NULL;
		/* closing stdin, -out and -err to shut up GTK */
		close(0); close(1); close(2);
		/* close the reading end of the pipe in the producer process */
		close(fd_pipe[0]);
		prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
		result = NFDi_OpenDialog_F(filterList, defaultPath, &len, &buf);
		
		if( NFD_ERROR != result ) {
			(void)(0
			|| NFDi_write(fd_pipe[1], sizeof(len), &len)
			|| NFDi_write(fd_pipe[1], len, buf)
			);
		}
		close(fd_pipe[1]);
		_exit( result );
	}
	/* close the writing end of the pipe in the parent process. */
	close(fd_pipe[1]);

	/* read the result from the child process */
	if( !NFDi_read(fd_pipe[0], sizeof(len), &len) ) {
		void *buf = NFDi_Malloc(len);
		if( buf ){
			if( !NFDi_read(fd_pipe[0], len, buf) ) {
				*outPath = buf;
			} else {
				free(buf);
				goto fail;
			}
		} else {
			goto fail;
		}
	} else {
		goto fail;
	}

	/* read the result back from the child */
	close(fd_pipe[0]);
	waitpid(pid, &status, 0);
	if( WIFEXITED(status) ) {
		return WEXITSTATUS(status);
	} else
	if( WIFSIGNALED(status) ) {
		return NFD_ERROR;
	}
fail:
	return NFD_ERROR;
}


static
nfdresult_t NFDi_OpenDialogMultiple_F( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
				    size_t *outBufSize,
                                    nfdpathset_t *outPaths )
{
    GtkWidget *dialog;
    nfdresult_t result;

    if ( !gtk_init_check( NULL, NULL ) )
    {
        NFDi_SetError(INIT_FAIL_MSG);
        return NFD_ERROR;
    }

    dialog = gtk_file_chooser_dialog_new( "Open Files",
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          "_Cancel", GTK_RESPONSE_CANCEL,
                                          "_Open", GTK_RESPONSE_ACCEPT,
                                          NULL );
    gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER(dialog), TRUE );

    /* Build the filter list */
    AddFiltersToDialog(dialog, filterList);

    /* Set the default path */
    SetDefaultPath(dialog, defaultPath);

    result = NFD_CANCEL;
    if ( gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
    {
        GSList *fileList = gtk_file_chooser_get_filenames( GTK_FILE_CHOOSER(dialog) );
        if ( NFDi_pathset_create_from_GSList( fileList, outBufSize, outPaths ) == NFD_ERROR )
        {
            gtk_widget_destroy(dialog);
            return NFD_ERROR;
        }
        
        result = NFD_OKAY;
    }

    WaitForCleanup();
    gtk_widget_destroy(dialog);
    WaitForCleanup();

    return result;
}

nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t *outPaths )
{
	int status;
	int fd_pipe[2];
	size_t indices_sz, buf_sz;
	pid_t pid;

	if( !outPaths ) { return NFD_ERROR; }

	if( pipe(fd_pipe) ) { return NFD_ERROR; }
	pid = fork();
	if( 0 > pid ) {
		close(fd_pipe[0]);
		close(fd_pipe[1]);
		return NFD_ERROR;
	}
	if( !pid ) {
		nfdresult_t result;
		/* closing stdin, -out and -err to shut up GTK */
		close(0); close(1); close(2);
		/* close the reading end of the pipe in the producer process */
		close(fd_pipe[0]);
		prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
		result = NFDi_OpenDialogMultiple_F(filterList, defaultPath, &buf_sz, outPaths);
		
		if( NFD_ERROR != result ) {
			indices_sz = outPaths->count * sizeof(*outPaths->indices);
			(void)(0
			|| NFDi_write(fd_pipe[1], sizeof(outPaths->count), &outPaths->count)
			|| NFDi_write(fd_pipe[1], indices_sz, outPaths->indices)
			|| NFDi_write(fd_pipe[1], sizeof(buf_sz), &buf_sz)
			|| NFDi_write(fd_pipe[1], buf_sz, outPaths->buf)
			);
		}
		close(fd_pipe[1]);
		_exit( result );
	}
	/* close the writing end of the pipe in the parent process. */
	close(fd_pipe[1]);

	outPaths->buf = NULL;
	outPaths->indices = NULL;
	/* read the result from the child process */
	if( NFDi_read(fd_pipe[0], sizeof(outPaths->count), &outPaths->count) ) {
		goto fail;
	}
	indices_sz = outPaths->count * sizeof(*outPaths->indices);
	outPaths->indices = NFDi_Malloc(indices_sz);
	if( !outPaths->indices ){
		goto fail;
	}
	if( NFDi_read(fd_pipe[0], indices_sz, outPaths->indices) ) {
		goto fail;
	}
	if( NFDi_read(fd_pipe[0], sizeof(buf_sz), &buf_sz) ) {
		goto fail;
	}
	outPaths->buf = NFDi_Malloc(buf_sz);
	if( !outPaths->buf ) {
		goto fail;
	}
	if( NFDi_read(fd_pipe[0], buf_sz, outPaths->buf) ) {
		goto fail;
	}

	/* read the result back from the child */
	close(fd_pipe[0]);
	waitpid(pid, &status, 0);
	if( WIFEXITED(status) ) {
		return WEXITSTATUS(status);
	} else
	if( WIFSIGNALED(status) ) {
		return NFD_ERROR;
	}
fail:
	free(outPaths->buf);
	outPaths->buf = NULL;
	free(outPaths->indices);
	outPaths->indices = NULL;
	return NFD_ERROR;
}

static
nfdresult_t NFDi_SaveDialog_F(
	const nfdchar_t *filterList,
	const nfdchar_t *defaultPath,
	size_t *outLen,
	nfdchar_t **outPath )
{
    GtkWidget *dialog;
    nfdresult_t result;

    if ( !gtk_init_check( NULL, NULL ) )
    {
        NFDi_SetError(INIT_FAIL_MSG);
        return NFD_ERROR;
    }

    dialog = gtk_file_chooser_dialog_new( "Save File",
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          "_Cancel", GTK_RESPONSE_CANCEL,
                                          "_Save", GTK_RESPONSE_ACCEPT,
                                          NULL ); 
    gtk_file_chooser_set_do_overwrite_confirmation( GTK_FILE_CHOOSER(dialog), TRUE );

    /* Build the filter list */    
    AddFiltersToDialog(dialog, filterList);

    /* Set the default path */
    SetDefaultPath(dialog, defaultPath);
    
    result = NFD_CANCEL;    
    if ( gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT )
    {
        char *filename;
        filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) );
        
        {
            size_t const len = strlen(filename) + 1;
	    *outLen = len;
            *outPath = NFDi_Malloc( len );
            memcpy( *outPath, filename, len );
            if ( !*outPath )
            {
                g_free( filename );
                gtk_widget_destroy(dialog);
                return NFD_ERROR;
            }
        }
        g_free(filename);

        result = NFD_OKAY;
    }

    WaitForCleanup();
    gtk_widget_destroy(dialog);
    WaitForCleanup();
    
    return result;
}

nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
	int status;
	int fd_pipe[2];
	size_t len;
	pid_t pid;

	if( pipe(fd_pipe) ) { return NFD_ERROR; }
	pid = fork();
	if( 0 > pid ) {
		close(fd_pipe[0]);
		close(fd_pipe[1]);
		return NFD_ERROR;
	}
	if( !pid ) {
		nfdresult_t result;
		nfdchar_t *buf = NULL;
		/* closing stdin, -out and -err to shut up GTK */
		close(0); close(1); close(2);
		/* close the reading end of the pipe in the producer process */
		close(fd_pipe[0]);
		prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
		result = NFDi_SaveDialog_F(filterList, defaultPath, &len, &buf);
		
		if( NFD_ERROR != result ) {
			(void)(0
			|| NFDi_write(fd_pipe[1], sizeof(len), &len)
			|| NFDi_write(fd_pipe[1], len, buf)
			);
		}
		close(fd_pipe[1]);
		_exit( result );
	}
	/* close the writing end of the pipe in the parent process. */
	close(fd_pipe[1]);

	/* read the result from the child process */
	if( !NFDi_read(fd_pipe[0], sizeof(len), &len) ) {
		void *buf = NFDi_Malloc(len);
		if( buf ){
			if( !NFDi_read(fd_pipe[0], len, buf) ) {
				*outPath = buf;
			} else {
				free(buf);
				goto fail;
			}
		} else {
			goto fail;
		}
	} else {
		goto fail;
	}

	/* read the result back from the child */
	close(fd_pipe[0]);
	waitpid(pid, &status, 0);
	if( WIFEXITED(status) ) {
		return WEXITSTATUS(status);
	} else
	if( WIFSIGNALED(status) ) {
		return NFD_ERROR;
	}
fail:
	return NFD_ERROR;
}
