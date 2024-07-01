#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include "chcfwdl/shim.h"

#include "printerbot/config.h"

#define SUPER_VERBOSE 1

#include "util/dprintf.h"
#include "util/brickprotect.h"

#define MAX_SHIM_COUNT 255

static FARPROC shim[MAX_SHIM_COUNT];
static struct printerbot_config config;

static uint8_t mainFirmware[0x40] = {0};
static uint8_t paramFirmware[0x40] = {0};

typedef int (*ogfwdlusb_open)(uint16_t *);

int fwdlusb_open(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogfwdlusb_open) shim[0])(rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}

typedef void (*ogfwdlusb_close)();

void fwdlusb_close() {
    dprintf_sv(NAME ": %s\n", __func__);
    ((ogfwdlusb_close) shim[1])();
}

typedef int (*ogfwdlusb_listupPrinter)(uint8_t *rIdArray);

int fwdlusb_listupPrinter(uint8_t *rIdArray) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogfwdlusb_listupPrinter) shim[2])(rIdArray);
    return ret;
}

typedef int (*ogfwdlusb_listupPrinterSN)(uint64_t *rSerialArray);

int fwdlusb_listupPrinterSN(uint64_t *rSerialArray) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogfwdlusb_listupPrinterSN) shim[3])(rSerialArray);
    return ret;
}

typedef int (*ogfwdlusb_selectPrinter)(uint8_t printerId, uint16_t *rResult);

int fwdlusb_selectPrinter(uint8_t printerId, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, printerId);
    int ret = ((ogfwdlusb_selectPrinter) shim[4])(printerId, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}

typedef int (*ogfwdlusb_selectPrinterSN)(uint64_t printerSN, uint16_t *rResult);

int fwdlusb_selectPrinterSN(uint64_t printerSN, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%ld)\n", __func__, printerSN);
    int ret = ((ogfwdlusb_selectPrinterSN) shim[5])(printerSN, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}

typedef int (*ogfwdlusb_getPrinterInfo)(uint16_t tagNumber, uint8_t *rBuffer, uint32_t *rLen);

int fwdlusb_getPrinterInfo(uint16_t tagNumber, uint8_t *rBuffer, uint32_t *rLen) {
    dprintf_sv(NAME ": %s(%d,%d)\n", __func__, tagNumber, *rLen);
    if (tagNumber == kPINFTAG_ENGID) {
        if (config.allow_firmware_write != BRICK_PRINTER_MAGIC_ID) {
            if (*rLen != 0x99) *rLen = 0x99;
            if (rBuffer) {
                memset(rBuffer, 0, *rLen);
                // bootFirmware
                int i = 1;
                memcpy(rBuffer + i, mainFirmware, sizeof(mainFirmware));
                // mainFirmware
                i += 0x26;
                memcpy(rBuffer + i, mainFirmware, sizeof(mainFirmware));
                // printParameterTable
                i += 0x26;
                memcpy(rBuffer + i, paramFirmware, sizeof(paramFirmware));
            }
            return 1;
        } else {
            ConfirmBricking();
        }
    }
    int ret = ((ogfwdlusb_getPrinterInfo) shim[6])(tagNumber, rBuffer, rLen);
    return ret;
}

typedef int (*ogfwdlusb_status)(uint16_t *rResult);

int fwdlusb_status(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogfwdlusb_status) shim[7])(rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}

typedef int (*ogfwdlusb_statusAll)(uint8_t *idArray, uint16_t *rResultArray);

int fwdlusb_statusAll(uint8_t *idArray, uint16_t *rResultArray) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogfwdlusb_statusAll) shim[8])(idArray, rResultArray);
    return ret;
}

typedef int (*ogfwdlusb_resetPrinter)(uint16_t *rResult);

int fwdlusb_resetPrinter(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogfwdlusb_resetPrinter) shim[9])(rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}

int fwdlusb_getFirmwareVersion(const uint8_t *buffer, int size) {
    int8_t a;
    uint32_t b = 0;
    for (int32_t i = 0; i < size; ++i) {
        if (*(int8_t *) (buffer + i) < 0x30 || *(int8_t *) (buffer + i) > 0x39) {
            if (*(int8_t *) (buffer + i) < 0x41 || *(int8_t *) (buffer + i) > 0x46) {
                if (*(int8_t *) (buffer + i) < 0x61 || *(int8_t *) (buffer + i) > 0x66) {
                    return 0;
                }
                a = *(int8_t *) (buffer + i) - 0x57;
            } else {
                a = *(int8_t *) (buffer + i) - 0x37;
            }
        } else {
            a = *(int8_t *) (buffer + i) - 0x30;
        }
        b = a + 0x10 * b;
    }
    return b;
}

int fwdlusb_updateFirmware_main(uint8_t update, LPCSTR filename, uint16_t *rResult) {
    DWORD result;
    HANDLE fwFile = NULL;
    DWORD bytesWritten = 0;

    if (filename) {
        HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        if (hFile == INVALID_HANDLE_VALUE) return 0;
        {
            dprintf("Printer: Firmware update failed (1005, open:%s): %lx\n", filename, GetLastError());
            if (rResult) *rResult = 1005;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }

        DWORD read;
        uint8_t buffer[0x40];
        result = ReadFile(hFile, buffer, 0x40, &read, NULL);
        CloseHandle(hFile);
        if (result && read > 0x24) {
            uint8_t rBuffer[0x40] = {0};

            memcpy(rBuffer, buffer + 0x2, 0x8);
            memcpy(rBuffer + 0x8, buffer + 0xA, 0x10);
            memcpy(rBuffer + 0x18, buffer + 0x20, 0xA);
            *(rBuffer + 0x22) = (uint8_t) fwdlusb_getFirmwareVersion(buffer + 0x1A, 0x2);
            *(rBuffer + 0x23) = (uint8_t) fwdlusb_getFirmwareVersion(buffer + 0x1C, 0x2);
            memcpy(rBuffer + 0x24, buffer + 0x2A, 0x2);

            memcpy(mainFirmware, rBuffer, 0x40);

            fwFile = CreateFileA(config.main_fw_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL, NULL);
            if (fwFile != NULL) {
                WriteFile(fwFile, rBuffer, 0x40, &bytesWritten, NULL);
                CloseHandle(fwFile);
            }

            if (rResult) *rResult = 0;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 1;
        } else {
            dprintf("Printer: Firmware update failed (1005, read): %lx\n", GetLastError());
            if (rResult) *rResult = 1005;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }
    } else {
        dprintf("Printer: Firmware update failed (1006): %lx\n", GetLastError());
        if (rResult) *rResult = 1006;
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        result = 0;
    }

    return result;
}

int fwdlusb_updateFirmware_param(uint8_t update, LPCSTR filename, uint16_t *rResult) {
    DWORD result;
    HANDLE fwFile = NULL;
    DWORD bytesWritten = 0;

    if (filename) {
        HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        if (hFile == INVALID_HANDLE_VALUE) return 0;
        {
            dprintf("Printer: Firmware update failed (1005, open:%s): %lx\n", filename, GetLastError());
            if (rResult) *rResult = 1005;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }

        DWORD read;
        uint8_t buffer[0x40];
        result = ReadFile(hFile, buffer, 0x40, &read, NULL);
        CloseHandle(hFile);
        if (result && read > 0x24) {
            uint8_t rBuffer[0x40] = {0};

            memcpy(rBuffer, buffer + 0x2, 8);
            memcpy(rBuffer + 0x8, buffer + 0xA, 0x10);
            memcpy(rBuffer + 0x18, buffer + 0x20, 0xA);
            memcpy(rBuffer + 0x22, buffer + 0x1A, 0x1);
            memcpy(rBuffer + 0x23, buffer + 0x1C, 0x1);
            memcpy(rBuffer + 0x24, buffer + 0x2A, 0x2);

            memcpy(paramFirmware, rBuffer, 0x40);

            fwFile = CreateFileA(config.param_fw_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL, NULL);
            if (fwFile != NULL) {
                WriteFile(fwFile, rBuffer, 0x40, &bytesWritten, NULL);
                CloseHandle(fwFile);
            }

            if (rResult) *rResult = 0;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 1;
        } else {
            dprintf("Printer: Firmware update failed (1005, read): %lx\n", GetLastError());
            if (rResult) *rResult = 1005;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }
    } else {
        dprintf("Printer: Firmware update failed (1006): %lx\n", GetLastError());
        if (rResult) *rResult = 1006;
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        result = 0;
    }

    return result;
}

typedef int (*ogfwdlusb_updateFirmware)(uint8_t update, LPCSTR filename, uint16_t *rResult);

int fwdlusb_updateFirmware(uint8_t update, LPCSTR filename, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %s)\n", __func__, update, filename);
    if (config.allow_firmware_write != BRICK_PRINTER_MAGIC_ID) {
        if (update == 1) {
            return fwdlusb_updateFirmware_main(update, filename, rResult);
        } else if (update == 3) {
            return fwdlusb_updateFirmware_param(update, filename, rResult);
        } else {
            *rResult = 0;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            return 1;
        }
    } else {
        ConfirmBricking();
        int ret = ((ogfwdlusb_updateFirmware) shim[10])(update, filename, rResult);
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        return ret;
    }
}

int fwdlusb_getFirmwareInfo_main(LPCSTR filename, uint8_t *rBuffer, uint32_t *rLen, uint16_t *rResult) {
    DWORD result;

    if (filename) {
        HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        if (hFile == INVALID_HANDLE_VALUE) return 0;
        {
            if (rResult) *rResult = 1005;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }

        DWORD read;
        uint8_t buffer[0x40];
        result = ReadFile(hFile, buffer, 0x40, &read, NULL);
        if (result && read > 0x24) {
            memcpy(rBuffer, buffer + 0x2, 0x8);
            memcpy(rBuffer + 0x8, buffer + 0xA, 0x10);
            memcpy(rBuffer + 0x18, buffer + 0x20, 0xA);
            *(rBuffer + 0x22) = (uint8_t) fwdlusb_getFirmwareVersion(buffer + 0x1A, 0x2);
            *(rBuffer + 0x23) = (uint8_t) fwdlusb_getFirmwareVersion(buffer + 0x1C, 0x2);
            memcpy(rBuffer + 0x24, buffer + 0x2A, 0x2);

            if (rResult) *rResult = 0;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 1;
        } else {
            if (rResult) *rResult = 1005;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }
    } else {
        if (rResult) *rResult = 1006;
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        result = 0;
    }

    return result;
}

int fwdlusb_getFirmwareInfo_param(LPCSTR filename, uint8_t *rBuffer, uint32_t *rLen, uint16_t *rResult) {
    DWORD result;

    if (filename) {
        HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        if (hFile == INVALID_HANDLE_VALUE) return 0;
        {
            if (rResult) *rResult = 1005;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }

        DWORD read;
        uint8_t buffer[0x40];
        result = ReadFile(hFile, buffer, 0x40, &read, NULL);
        if (result && read > 0x24) {
            memcpy(rBuffer, buffer + 0x2, 8);
            memcpy(rBuffer + 0x8, buffer + 0xA, 0x10);
            memcpy(rBuffer + 0x18, buffer + 0x20, 0xA);
            memcpy(rBuffer + 0x22, buffer + 0x1A, 0x1);
            memcpy(rBuffer + 0x23, buffer + 0x1C, 0x1);
            memcpy(rBuffer + 0x24, buffer + 0x2A, 0x2);

            if (rResult) *rResult = 0;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 1;
        } else {
            if (rResult) *rResult = 1005;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            result = 0;
        }
    } else {
        if (rResult) *rResult = 1006;
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        result = 0;
    }

    return result;
}

typedef int (*ogfwdlusb_getFirmwareInfo)(uint8_t update, LPCSTR filename, uint8_t *rBuffer, uint32_t *rLen,
                                         uint16_t *rResult);

int fwdlusb_getFirmwareInfo(uint8_t update, LPCSTR filename, uint8_t *rBuffer, uint32_t *rLen, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %s)\n", __func__, update, filename);

    if (config.allow_firmware_write != BRICK_PRINTER_MAGIC_ID) {
        if (!rBuffer) {
            *rLen = 38;
            return 1;
        }
        if (*rLen > 38) *rLen = 38;
        if (update == 1) {
            return fwdlusb_getFirmwareInfo_main(filename, rBuffer, rLen, rResult);
        } else if (update == 3) {
            return fwdlusb_getFirmwareInfo_param(filename, rBuffer, rLen, rResult);
        } else {
            if (rResult) *rResult = 0;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            return 1;
        }
    } else {
        ConfirmBricking();
        int ret = ((ogfwdlusb_getFirmwareInfo) shim[11])(update, filename, rBuffer, rLen, rResult);
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        return ret;
    }
}

typedef int (*ogfwdlusb_MakeThread)(uint16_t maxCount);

int fwdlusb_MakeThread(uint16_t maxCount) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, maxCount);
    int ret = ((ogfwdlusb_MakeThread) shim[12])(maxCount);
    return ret;
}

typedef int (*ogfwdlusb_ReleaseThread)(uint16_t *rResult);

int fwdlusb_ReleaseThread(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogfwdlusb_ReleaseThread) shim[13])(rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}

typedef int (*ogfwdlusb_AttachThreadCount)(uint16_t *rCount, uint16_t *rMaxCount);

int fwdlusb_AttachThreadCount(uint16_t *rCount, uint16_t *rMaxCount) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogfwdlusb_AttachThreadCount) shim[14])(rCount, rMaxCount);
    return ret;
}

typedef int (*ogfwdlusb_getErrorLog)(uint16_t index, uint8_t *rData, uint16_t *rResult);

int fwdlusb_getErrorLog(uint16_t index, uint8_t *rData, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, index);
    int ret = ((ogfwdlusb_getErrorLog) shim[15])(index, rData, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


void chcfwdl_shim_install(struct printerbot_config *cfg) {
    assert(cfg != NULL);

    memcpy(&config, cfg, sizeof(*cfg));

    char path[MAX_PATH];
    sprintf(path, ".\\C%dAFWDLusb.dll", cfg->to);
    HINSTANCE ptr = LoadLibraryA(path);
    if (ptr == NULL) {
        dprintf("LoadLibrary(%s) FAILED: %ld\n", path, GetLastError());
        exit(1);
        return;
    }
    int count = 0;
    shim[count++] = GetProcAddress(ptr, "fwdlusb_open");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_close");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_listupPrinter");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_listupPrinterSN");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_selectPrinter");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_selectPrinterSN");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_getPrinterInfo");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_status");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_statusAll");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_resetPrinter");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_updateFirmware");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_getFirmwareInfo");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_MakeThread");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_ReleaseThread");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_AttachThreadCount");
    shim[count++] = GetProcAddress(ptr, "fwdlusb_getErrorLog");
    for (int i = 0; i < count; i++) {
        if (shim[i] == NULL) {
            dprintf(NAME "NON-IMPORTED FUNCTION AT INDEX %d!!\n", i);
        }
    }
    dprintf(NAME ": Shimmed %d functions\n", count);

    // Load firmware if has previously been written to
    DWORD bytesRead = 0;
    HANDLE fwFile = CreateFileA(cfg->main_fw_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, NULL);
    if (fwFile != NULL) {
        ReadFile(fwFile, mainFirmware, 0x40, &bytesRead, NULL);
        CloseHandle(fwFile);
    }

    fwFile = CreateFileA(cfg->param_fw_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if (fwFile != NULL) {
        ReadFile(fwFile, paramFirmware, 0x40, &bytesRead, NULL);
        CloseHandle(fwFile);
    }

    dprintf(NAME ": CHCFWDL shim installed\n");
}

#pragma clang diagnostic pop