#pragma once

#include "chcfwdl/shim.h"

static FARPROC shim[64];

int (*getFunc())(int, int) { … }

int (*fwdlusb_open())(uint16_t *rResult){
    dprintf_sv("%s", __func__);
    return ((fwdlusb_open)shim[0])(rResult);
}

int fwdlusb_open(uint16_t * rResult);
void fwdlusb_close();
int fwdlusb_listupPrinter(uint8_t * rIdArray);
int fwdlusb_listupPrinterSN(uint64_t * rSerialArray);
int fwdlusb_selectPrinter(uint8_t printerId, uint16_t * rResult);
int fwdlusb_selectPrinterSN(uint64_t printerSN, uint16_t * rResult);
int fwdlusb_getPrinterInfo(uint16_t tagNumber, uint8_t * rBuffer, uint32_t * rLen);
int fwdlusb_status(uint16_t * rResult);
int fwdlusb_statusAll(uint8_t * idArray, uint16_t * rResultArray);
int fwdlusb_resetPrinter(uint16_t * rResult);
int fwdlusb_updateFirmware(uint8_t update, LPCSTR filename, uint16_t * rResult);
int fwdlusb_getFirmwareInfo(uint8_t update, LPCSTR filename, uint8_t * rBuffer, uint32_t * rLen, uint16_t * rResult);
int fwdlusb_MakeThread(uint16_t maxCount);
int fwdlusb_ReleaseThread(uint16_t * rResult);
int fwdlusb_AttachThreadCount(uint16_t * rCount, uint16_t * rMaxCount);
int fwdlusb_getErrorLog(uint16_t index, uint8_t * rData, uint16_t * rResult);

void chcfwdl_shim_install(struct printerbot_config* cfg){
    char path[MAX_PATH];
    sprintf(path, ".\\CHC%dAusb.dll", cfg->from);
    HINSTANCE ptr = LoadLibraryA(path);
    if (ptr == NULL){
        dprintf("LoadLibraryA FAILED: %ld\n", GetLastError());
        exit(1);
        return;
    }
    shim[0] = GetProcAddress(ptr, "fwdlusb_open");
}