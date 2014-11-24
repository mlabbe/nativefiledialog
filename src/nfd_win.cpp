/*
  Native File Dialog

  http://www.frogtoss.com/labs
 */

#include <wchar.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <ShObjIdl.h>

#include "nfd_common.h"

/* only locally define UNICODE in this compilation unit */
#ifndef UNICODE
#define UNICODE
#endif

// allocs the space in outPath -- call free()
static void CopyWCharToNFDChar( const wchar_t *inStr, nfdchar_t **outStr )
{
    int inStrCharacterCount = static_cast<int>(wcslen(inStr)); 
    int bytesNeeded = WideCharToMultiByte( CP_UTF8, 0,
                                           inStr, inStrCharacterCount,
                                           NULL, 0, NULL, NULL );    
    assert( bytesNeeded );
    assert( !*outStr );
    bytesNeeded += 1;

    *outStr = (nfdchar_t*)NFDi_Malloc( bytesNeeded );
    if ( !*outStr )
        return;

    int ret = WideCharToMultiByte( CP_UTF8, 0,
                                   inStr, -1,
                                   *outStr, bytesNeeded,
                                   NULL, NULL );
    assert( ret ); _NFD_UNUSED(ret);
}

// allocs the space in outStr -- call free()
static void CopyNFDCharToWChar( const nfdchar_t *inStr, wchar_t **outStr )
{
    
    int inStrByteCount = static_cast<int>(strlen(inStr));
    int charsNeeded = MultiByteToWideChar(CP_UTF8, 0,
                                          inStr, inStrByteCount,
                                          NULL, 0 );    
    assert( charsNeeded );
    assert( !*outStr );
    charsNeeded += 1; // terminator
    
    *outStr = (wchar_t*)NFDi_Malloc( charsNeeded * sizeof(wchar_t) );    
    if ( !*outStr )
        return;        

    int ret = MultiByteToWideChar(CP_UTF8, 0,
                                  inStr, inStrByteCount,
                                  *outStr, charsNeeded);
    (*outStr)[charsNeeded-1] = '\0';

#ifdef _DEBUG
    int inStrCharacterCount = static_cast<int>(NFDi_UTF8_Strlen(inStr));
    assert( ret == inStrCharacterCount );
#else
    _NFD_UNUSED(ret);
#endif
}

/* ext is in format "jpg", no wildcards or separators */
static int AppendExtensionToSpecBuf( const char *ext, char *specBuf, size_t specBufLen )
{
    const char SEP[] = ";";
    assert( specBufLen > strlen(ext)+3 );
    
    if ( strlen(specBuf) > 0 )
    {
        strncat( specBuf, SEP, specBufLen - strlen(specBuf) - 1 );
        specBufLen += strlen(SEP);
    }

    char extWildcard[NFD_MAX_STRLEN];
    int bytesWritten = sprintf_s( extWildcard, NFD_MAX_STRLEN, "*.%s", ext );
    assert( bytesWritten == strlen(ext)+2 );
    
    strncat( specBuf, extWildcard, specBufLen - strlen(specBuf) - 1 );

    return NFD_OKAY;
}

static int AddFiltersToDialog( ::IFileOpenDialog *fileOpenDialog, const char *filterList )
{
    const wchar_t EMPTY_WSTR[] = L"";
    const wchar_t WILDCARD[] = L"*.*";

    if ( !filterList || strlen(filterList) == 0 )
        return NFD_OKAY;

    // Count rows to alloc
    size_t filterCount = 1; /* guaranteed to have one filter on a correct, non-empty parse */
    const char *p_filterList;
    for ( p_filterList = filterList; *p_filterList; ++p_filterList )
    {
        if ( *p_filterList == ';' )
            ++filterCount;
    }    

    assert(filterCount);
    if ( !filterCount )
    {
        NFDi_SetError("Error parsing filters.");
        return NFD_ERROR;
    }

    /* filterCount plus 1 because we hardcode the *.* wildcard after the while loop */
    COMDLG_FILTERSPEC *specList = (COMDLG_FILTERSPEC*)NFDi_Malloc( sizeof(COMDLG_FILTERSPEC) * (filterCount + 1) );
    if ( !specList )
    {
        return NFD_ERROR;
    }
    for (size_t i = 0; i < filterCount+1; ++i )
    {
        specList[i].pszName = NULL;
        specList[i].pszSpec = NULL;
    }

    size_t specIdx = 0;
    p_filterList = filterList;
    char typebuf[NFD_MAX_STRLEN] = {0};  /* one per comma or semicolon */
    char *p_typebuf = typebuf;
    char filterName[NFD_MAX_STRLEN] = {0};

    char specbuf[NFD_MAX_STRLEN] = {0}; /* one per semicolon */

    while ( 1 ) 
    {
        if ( NFDi_IsFilterSegmentChar(*p_filterList) )
        {
            /* append a type to the specbuf (pending filter) */
            AppendExtensionToSpecBuf( typebuf, specbuf, NFD_MAX_STRLEN );            

            p_typebuf = typebuf;
            memset( typebuf, 0, sizeof(char)*NFD_MAX_STRLEN );
        }

        if ( *p_filterList == ';' || *p_filterList == '\0' )
        {
            /* end of filter -- add it to specList */

            // Empty filter name -- Windows describes them by extension.            
            specList[specIdx].pszName = EMPTY_WSTR;
            CopyNFDCharToWChar( specbuf, (wchar_t**)&specList[specIdx].pszSpec );
                        
            memset( specbuf, 0, sizeof(char)*NFD_MAX_STRLEN );
            ++specIdx;
            if ( specIdx == filterCount )
                break;
        }

        if ( !NFDi_IsFilterSegmentChar( *p_filterList ))
        {
            *p_typebuf = *p_filterList;
            ++p_typebuf;
        }

        ++p_filterList;
    }

    /* Add wildcard */
    specList[specIdx].pszSpec = WILDCARD;
    specList[specIdx].pszName = EMPTY_WSTR;
    
    fileOpenDialog->SetFileTypes( filterCount+1, specList );

    /* free speclist */
    for ( size_t i = 0; i < filterCount; ++i )
    {
        NFDi_Free( (void*)specList[i].pszSpec );
    }
    NFDi_Free( specList );    

    return NFD_OKAY;
}

/* public */

nfdresult_t NFD_OpenDialog( const char *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    // Init COM library.
    HRESULT result = ::CoInitializeEx(NULL,
                                      ::COINIT_APARTMENTTHREADED |
                                      ::COINIT_DISABLE_OLE1DDE );
    if ( !SUCCEEDED(result))
    {
        NFDi_SetError("Could not initialize COM.");
        return NFD_ERROR;
    }

    ::IFileOpenDialog *fileOpenDialog(NULL);

    // Create dialog
    result = ::CoCreateInstance(::CLSID_FileOpenDialog, NULL,
                                CLSCTX_ALL, ::IID_IFileOpenDialog,
                                reinterpret_cast<void**>(&fileOpenDialog) );
                                
    if ( !SUCCEEDED(result) )
    {
        NFDi_SetError("Could not create dialog.");
        return NFD_ERROR;
    }

    // Build the filter list
    if ( !AddFiltersToDialog( fileOpenDialog, filterList ) )
    {
        return NFD_ERROR;
    }

    // Show the dialog.
    result = fileOpenDialog->Show(NULL);
    nfdresult_t nfdResult = NFD_ERROR;
    if ( SUCCEEDED(result) )
    {
        // Get the file name
        ::IShellItem *shellItem(NULL);
        result = fileOpenDialog->GetResult(&shellItem);
        if ( !SUCCEEDED(result) )
        {
            NFDi_SetError("Could not get shell item from dialog.");
            return NFD_ERROR;
        }
        wchar_t *filePath(NULL);
        result = shellItem->GetDisplayName(::SIGDN_FILESYSPATH, &filePath);
        if ( !SUCCEEDED(result) )
        {
            NFDi_SetError("Could not get file path for selected.");
            return NFD_ERROR;
        }

        CopyWCharToNFDChar( filePath, outPath );
        CoTaskMemFree(filePath);
        if ( !*outPath )
        {
            /* error is malloc-based, error message would be redundant */
            return NFD_ERROR;
        }

        nfdResult = NFD_OKAY;
        
        shellItem->Release();
    }
    else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED) )
    {
        nfdResult = NFD_CANCEL;
    }
    else
    {
        NFDi_SetError("File dialog box show failed.");
        nfdResult = NFD_ERROR;
    }

    ::CoUninitialize();
    
    return nfdResult;
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
