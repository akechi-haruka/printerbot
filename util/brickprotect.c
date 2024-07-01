#include <windows.h>

#include "util/dprintf.h"
#include "util/brickprotect.h"

void ConfirmBricking(){
    dprintf("-----------------------------------------------------\n");
    dprintf("WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!\n");
    dprintf("-----------------------------------------------------\n");
    dprintf("POSSIBLE FATAL OPERATION THAT MAY BRICK YOUR DEVICE!\n");
    dprintf("YOU REALLY SHOULD NOT DO THIS.\n");
    dprintf("TO CONFIRM THIS OPERATION, STEP THROUGH THE int3\n");
    dprintf("INSTRUCTION AND DISCARD THE EXCEPTION\n");
    DebugBreak();
    Sleep(5000);
}