/*
 Native File Dialog

 http://www.frogtoss.com/labs
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dlfcn.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "nfd.h"
#include "nfd_common.h"

const char NOMEM_MSG[] = "Out of memory.";

static int exec (const char* command, int argc, const char** arguments, const char *workdingDirectory, size_t size, char* stdoutBuffer)
{
    int status;
    int link[2];
    if (pipe(link) < 0) {
        return -1;
    }

    const pid_t childPid = fork();

    if (childPid < 0)
    {
        return -1;
    }

    if (childPid == 0)
    {
        int cmdargc = 0;
        int i;
        const char* argv[64];
        argv[cmdargc++] = command;
        for (i = 0; i < argc; ++i)
        {
            argv[cmdargc++] = arguments[i];
            if (cmdargc >= 63)
            {
                break;
            }
        }
        argv[argc] = NULL;
        dup2(link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        if (workdingDirectory) {
            chdir(workdingDirectory);
        }
        execv(command, (char* const*)(argv));
        exit(-1);
    }

    wait(&status);

    if (!WIFEXITED(status))
    {
        return -1;
    }

    if (WEXITSTATUS(status) != 0)
    {
        return -1;
    }

    close(link[1]);
    read(link[0], stdoutBuffer, size);
    return 0;
}

nfdresult_t NFD_OpenDialog( const char *filterList, const nfdchar_t *defaultPath, nfdchar_t **outPath )
{
    const char *arguments[] =
    {
        "--file-selection"
    };
    char buf[1024];
    if (exec("zenity", 1, arguments, defaultPath, sizeof(buf), buf) != 0)
    {
        NFDi_SetError("Failed to execute zenity");
        return NFD_ERROR;
    }
    buf[sizeof(buf) - 1] = '\0';
    *outPath = NFDi_Malloc(strlen(buf) + 1);
    memcpy(*outPath, buf, strlen(buf) + 1);
    return NFD_OKAY;
}

nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList, const nfdchar_t *defaultPath, nfdpathset_t *outPaths )
{
    const char *arguments[] =
    {
        "--file-selection",
        "--multiple",
        "--separator=|"
    };
    char buf[4096];
    if (exec("zenity", 1, arguments, defaultPath, sizeof(buf), buf) != 0)
    {
        NFDi_SetError("Failed to execute zenity");
        return NFD_ERROR;
    }
    buf[sizeof(buf) - 1] = '\0';
    outPaths->count = 1; // TODO: split buf by |
    outPaths->indices = NFDi_Malloc(sizeof(size_t) * outPaths->count);
    if (!outPaths->indices)
    {
        NFDi_SetError(NOMEM_MSG);
        return NFD_ERROR;
    }

    size_t bufSize = strlen(buf) + 1;
    outPaths->buf = NFDi_Malloc(sizeof(nfdchar_t) * bufSize);

    nfdchar_t *p_buf = outPaths->buf;
    char *p_sep = buf;
    int count = 0;
    for (;;)
    {
        char *sep = strchr(p_sep, '|');
        if (sep)
        {
            *sep = '\0';
        }
        size_t byteLen = strlen(p_sep) + 1;
        memcpy(p_buf, p_sep, byteLen);
        const ptrdiff_t index = p_buf - outPaths->buf;
        outPaths->indices[count++] = (size_t) index;
        if (!sep)
        {
            break;
        }
        p_sep = sep + 1;
    }
    return NFD_OKAY;
}

nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList, const nfdchar_t *defaultPath, nfdchar_t **outPath )
{
    const char *arguments[] =
    {
        "--file-selection",
        "--save"
    };
    char buf[1024];
    if (exec("zenity", 1, arguments, defaultPath, sizeof(buf), buf) != 0)
    {
        NFDi_SetError("Failed to execute zenity");
        return NFD_ERROR;
    }
    buf[sizeof(buf) - 1] = '\0';
    *outPath = NFDi_Malloc(strlen(buf) + 1);
    memcpy(*outPath, buf, strlen(buf) + 1);
    return NFD_OKAY;
}

nfdresult_t NFD_PickFolder( const nfdchar_t *defaultPath, nfdchar_t **outPath )
{
    const char *arguments[] = {
        "--file-selection",
        "--directory"
    };
    char buf[1024];
    if (exec("zenity", 1, arguments, defaultPath, sizeof(buf), buf) != 0)
    {
        NFDi_SetError("Failed to execute zenity");
        return NFD_ERROR;
    }
    buf[sizeof(buf) - 1] = '\0';
    *outPath = NFDi_Malloc(strlen(buf) + 1);
    memcpy(*outPath, buf, strlen(buf) + 1);
    return NFD_OKAY;
}
