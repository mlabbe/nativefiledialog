/*
  Native File Dialog

  http://www.frogtoss.com/labs
 */

#include <wchar.h>
#include <assert.h>
#include <windows.h>
#include <ShObjIdl.h>

#include "nfd_common.h"

#define UNICODE

// allocs the space in outPath
static void CopyWCharToNFDChar( const wchar_t *inStr, nfdchar_t **outPath )
{
    int inStrCharacterCount = static_cast<int>(wcslen(inStr)); 
    int bytesNeeded = WideCharToMultiByte( CP_UTF8, 0,
                                           inStr, inStrCharacterCount,
                                           NULL, 0, NULL, NULL );

    assert( !*outPath );
    *outPath = (nfdchar_t*)NFDi_Malloc( bytesNeeded );
    if ( !*outPath )
        return;

    WideCharToMultiByte( CP_UTF8, 0,
                         inStr, inStrCharacterCount,
                         *outPath, bytesNeeded,
                         NULL, NULL );
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

    // todo: set file types here

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
