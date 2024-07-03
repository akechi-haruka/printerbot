#include <windows.h>
#include "util/dprintf.h"

FARPROC GetProcAddressChecked(HINSTANCE ptr, const char* name){
    FARPROC ret = GetProcAddress(ptr, name);
    if (ret == NULL){
        dprintf("COULD NOT FIND ENTRY POINT NAMED %s!!\n", name);
    }
    return ret;
}