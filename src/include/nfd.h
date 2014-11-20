/*
  Native File Dialog

  User API

  http://www.frogtoss.com/labs
 */


#ifndef _NFD_H
#define _NFD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef char nfdchar_t;

typedef struct {
    nfdchar_t *buf;
    size_t *indices; // byte offsets into buf
    size_t count;    // number of indices into buf
}nfdpathset_t;

typedef enum {
    NFD_ERROR,
    NFD_OKAY,
    NFD_CANCEL,
}nfdresult_t;
    

/* nfd_<targetplatform>.c */

nfdresult_t NFD_OpenDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath );
                        
nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t *outPaths );

nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath );

/* nfd_common.c */

const char *NFD_GetError( void );
size_t      NFD_PathSet_GetCount( const nfdpathset_t *pathSet );
nfdchar_t  *NFD_PathSet_GetPath( const nfdpathset_t *pathSet, size_t num );
void        NFD_PathSet_Free( nfdpathset_t *pathSet );


#ifdef __cplusplus
}
#endif

#endif
