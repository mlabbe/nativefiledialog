/*
  Native File Dialog

  http://www.frogtoss.com/labs
 */

#include <assert.h>
#include <windows.h>
#include <shlobj.h>
#include "nfd_common.h"

typedef enum LegacyWinNfdMode
{
    LegacyWinNfdMode_Open
    , LegacyWinNfdMode_Save
    , LegacyWinNfdMode_Multi
} LegacyWinNfdModeEnum;

// allocs the space in outPath -- call free()
// returns number of substrings on success, 0 otherwise
static int CopyMultiTCharToNFDChar( const TCHAR *inStr, nfdchar_t **outStr )
{
    const TCHAR *i;
    int numStrings = 0;
    for (i = inStr; ; ++i)
    {
        // a single terminator marks the end of a substring
        if ( !*i )
        {
            ++numStrings;
        }
        // a double terminator marks the end of the buffer
        if ( !i[0] && !i[1] )
        {
            break;
        }
    }
    if ( !numStrings )
        return 0;
#if defined( UNICODE ) || defined( _UNICODE )
    int inStrCharacterCount = i - inStr; 
    int bytesNeeded = WideCharToMultiByte( CP_UTF8, 0,
                                           inStr, inStrCharacterCount,
                                           NULL, 0, NULL, NULL );    
    assert( bytesNeeded );
    bytesNeeded += 1;

    *outStr = (nfdchar_t*)NFDi_Malloc( bytesNeeded );
    if ( !*outStr )
        return 0;

    int bytesWritten = WideCharToMultiByte( CP_UTF8, 0,
                                            inStr, inStrCharacterCount,
                                            *outStr, bytesNeeded,
                                            NULL, NULL );
    assert( bytesWritten ); _NFD_UNUSED( bytesWritten );
#else // not unicode
    *outStr = malloc( (i - inStr) + 1 );
    if ( !*outStr )
        return 0;
    memcpy(*outStr, inStr, (i - inStr) + 1 );
#endif
    return numStrings;
}

// allocs the space in outPath -- call free()
// returns non-zero on error
static int CopyTCharToNFDChar( const TCHAR *inStr, nfdchar_t **outStr )
{
#if defined( UNICODE ) || defined( _UNICODE )
    int inStrCharacterCount = wcslen(inStr); 
    int bytesNeeded = WideCharToMultiByte( CP_UTF8, 0,
                                           inStr, inStrCharacterCount,
                                           NULL, 0, NULL, NULL );    
    assert( bytesNeeded );
    bytesNeeded += 1;

    *outStr = (nfdchar_t*)NFDi_Malloc( bytesNeeded );
    if ( !*outStr )
        return 1;

    int bytesWritten = WideCharToMultiByte( CP_UTF8, 0,
                                            inStr, -1,
                                            *outStr, bytesNeeded,
                                            NULL, NULL );
    assert( bytesWritten ); _NFD_UNUSED( bytesWritten );
#else // not unicode
    *outStr = strdup( inStr );
    if ( !*outStr )
        return 1;
#endif
    return 0;
}


// allocs the space in outStr -- call free()
// returns non-zero on error
static int CopyNFDCharToTChar( const nfdchar_t *inStr, TCHAR **outStr )
{
#if defined( UNICODE ) || defined( _UNICODE )
    int inStrByteCount = strlen(inStr);
    int charsNeeded = MultiByteToWideChar(CP_UTF8, 0,
                                          inStr, inStrByteCount,
                                          NULL, 0 );    
    assert( charsNeeded );
    assert( !*outStr );
    charsNeeded += 1; // terminator
    
    *outStr = (TCHAR*)NFDi_Malloc( charsNeeded * sizeof(TCHAR) );    
    if ( !*outStr )
        return 1;        

    int ret = MultiByteToWideChar(CP_UTF8, 0,
                                  inStr, inStrByteCount,
                                  *outStr, charsNeeded);
    (*outStr)[charsNeeded-1] = '\0';

#ifdef _DEBUG
    int inStrCharacterCount = NFDi_UTF8_Strlen(inStr);
    assert( ret == inStrCharacterCount );
#else
    _NFD_UNUSED(ret);
#endif
#else // not unicode
    *outStr = strdup( inStr );
    if ( !*outStr )
        return 1;
#endif
    return 0;
}

// converts nfd filter list to win32 filter list
// allocs the space in outStr -- call free()
// returns non-zero on error
static int ConvertNFDFilterList( const nfdchar_t *inStr, TCHAR **outStr )
{
    char result[1024] = {0};
    char *dst;
    const char *src;
    TCHAR *out;
    
    assert( inStr );
    assert( *inStr );
    assert( outStr );
    
    // walk through inStr
    dst = result;
    src = inStr;
    do
    {
        char ext[1024] = {0};
        char *walk;
        char *last = dst;
        
        // lists are delimited by ';'
        strncpy( ext, src, strcspn( src, ";" ) );
        
        // extensions are delimited by ','
        for ( walk = ext; *walk; walk += strcspn( walk, "," ) )
        {
            walk += *walk == ',';
            
            strcat(dst, "*." );
            strncpy( dst + strlen(dst), walk, strcspn( walk, "," ) );
            strcat( dst, ";" );
        }
        
        // overwrite last ';' with '\t' delimiter
        dst[strlen(dst) - 1] = '\t';
        
        // duplicate list (this is for win32's name field)
        memcpy( dst + strlen( dst ), last, strlen( last ) );
        
        // advance to next list
        src += strcspn( src, ";" );
        src += *src == ';';
        dst += strlen( dst );
    } while ( *src );
    
    // this extra delimiter marks the end of the substring buffer
    strcat(result, "\t");
    
    if ( CopyNFDCharToTChar( result, outStr ) )
        return 1;
    
    // overwrite every '\t' delimiter with '\0' delimiter
    for (out = *outStr; *out; ++out)
    {
    #if defined( UNICODE ) || defined( _UNICODE )
        if ( *out == L'\t' )
            *out = L'\0';
    #else // not unicode
        if ( *out == '\t' )
            *out = '\0';
    #endif
    }
    
    return 0;
}

// master function to reduce redundant code
static nfdresult_t LegacyNFDWin( const nfdchar_t *filterList,
                                 const nfdchar_t *defaultPath,
                                 nfdchar_t **outPath,
                                 nfdpathset_t *pathSet,
                                 const LegacyWinNfdModeEnum mode )
{
    nfdresult_t result = NFD_ERROR;
    OPENFILENAME ofn;
    TCHAR buffer[2048];
    TCHAR *path = 0;
    TCHAR *filter = 0;
    
    assert(outPath);
    
    *outPath = 0;

    if ( filterList && *filterList )
    {
        if ( ConvertNFDFilterList( filterList, &filter ) )
        {
            NFDi_SetError("Could not prepare filter list.");
            goto end;
        }
    }
    if ( defaultPath && *defaultPath )
    {
        if ( CopyNFDCharToTChar( defaultPath, &path ) )
        {
            goto end;
        }
    }

    // initialize ofn structure
    ZeroMemory( &ofn, sizeof(ofn) );
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = buffer;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(buffer) / sizeof(buffer[0]);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = path;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if ( mode == LegacyWinNfdMode_Multi )
    {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
    }
    if ( mode == LegacyWinNfdMode_Save )
    {
        ofn.Flags |= OFN_OVERWRITEPROMPT;
    }

    // queue prompts; check results
    if ( ( mode != LegacyWinNfdMode_Save && GetOpenFileName( &ofn ) ) ||
         ( mode == LegacyWinNfdMode_Save && GetSaveFileName( &ofn ) )
    )
    {
        if ( mode == LegacyWinNfdMode_Multi )
        {
            assert(pathSet);
            
            pathSet->count = CopyMultiTCharToNFDChar( ofn.lpstrFile, outPath );
            if ( pathSet->count == 0 )
            {
                goto end;
            }
            
            // the first is the directory, and the others are filenames
            if (pathSet->count > 1)
            {
                pathSet->count -= 1;
            }
        }
        else if ( CopyTCharToNFDChar( ofn.lpstrFile, outPath ) )
        {
            goto end;
        }
        result = NFD_OKAY;
    }
    else
    {
        result = NFD_CANCEL;
    }

end:
    if ( filter )
    {
        NFDi_Free( filter );
    }
    if ( path )
    {
        NFDi_Free( path );
    }

    return result;
}

/* public */

nfdresult_t NFD_OpenDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    return LegacyNFDWin(filterList, defaultPath, outPath, 0, LegacyWinNfdMode_Open);
}

nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t *pathSet )
{
    nfdchar_t *outPath = 0;
    const nfdchar_t *walk;
    size_t bufferLen;
    size_t dirLen;
    size_t i;
    nfdresult_t result = NFD_ERROR;
    
    assert(pathSet);
    
    memset ( pathSet, 0, sizeof(*pathSet) );
    
    // grab file list
    result = LegacyNFDWin(filterList, defaultPath, &outPath, pathSet, LegacyWinNfdMode_Multi);
    if ( result != NFD_OKAY )
    {
        goto end;
    }
    result = NFD_ERROR;
    
    // allocate indices
    pathSet->indices = NFDi_Malloc( pathSet->count * sizeof(*pathSet->indices) );
    if ( !pathSet->indices )
    {
        goto end;
    }
    
    // early exit condition: only one file provided
    if ( pathSet->count == 1)
    {
        pathSet->buf = outPath;
        pathSet->indices[0] = 0;
        return NFD_OKAY;
    }
    
    // walk the buffer
    // (the first substring is the directory, and the others are filenames)
    dirLen = strlen(outPath) + 1;
    walk = outPath + dirLen;
    bufferLen = 0;
    for ( i = 0; i < pathSet->count; ++i )
    {
        pathSet->indices[i] = bufferLen;
        bufferLen += dirLen + strlen(walk) + 1;
        walk += strlen(walk) + 1;
    }
    
    // allocate filename buffer
    pathSet->buf = NFDi_Malloc( bufferLen );
    if ( !pathSet->buf )
    {
        goto end;
    }
    
    // walk the buffer again, propagating the filename buffer this time
    walk = outPath + dirLen;
    for ( i = 0; i < pathSet->count; ++i )
    {
        nfdchar_t *str = pathSet->buf + pathSet->indices[i];
        
        strcpy(str, outPath);
        strcat(str, "\\");
        strcat(str, walk);
        walk += strlen(walk) + 1;
    }
    
    // success
    result = NFD_OKAY;

end:
    if ( result == NFD_ERROR )
    {
        if ( pathSet->indices )
        {
            NFDi_Free( pathSet->indices );
        }
        if ( pathSet->buf )
        {
            NFDi_Free( pathSet->buf );
        }
    }
    if ( outPath )
    {
        NFDi_Free( outPath );
    }
    return result;
}

nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    return LegacyNFDWin(filterList, defaultPath, outPath, 0, LegacyWinNfdMode_Save);
}

nfdresult_t NFD_PickFolder( const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    BROWSEINFO browse;
    LPITEMIDLIST lp;
    TCHAR buffer[2048];
    
    // initialize browse structure
    ZeroMemory( &browse, sizeof(browse) );
    browse.pszDisplayName = buffer;
    browse.iImage = -1;
    browse.ulFlags |= BIF_USENEWUI | BIF_VALIDATE | BIF_EDITBOX;
    
    // user clicked cancel
    if ( !(lp = SHBrowseForFolder( &browse ) ) )
    {
        return NFD_CANCEL;
    }
    if ( !SHGetPathFromIDList( lp, buffer ) )
    {
        NFDi_SetError("Could not create dialog.");
        return NFD_ERROR;
    }

    if ( CopyTCharToNFDChar( buffer, outPath ) )
    {
        return NFD_ERROR;
    }

    return NFD_OKAY;
    _NFD_UNUSED( defaultPath );
}


