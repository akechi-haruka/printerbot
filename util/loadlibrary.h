#pragma once

#include <windows.h>

FARPROC GetProcAddressChecked(HANDLE ptr, const char* name);