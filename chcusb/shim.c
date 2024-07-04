#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <util/imagemanipulation.h>
#include <util/dump.h>
#include <util/loadlibrary.h>
#include "chcusb/shim.h"
#include "printerbot/config.h"

#define SUPER_VERBOSE 1

#include "util/dprintf.h"
#include "util/brickprotect.h"
#include "printerbot/rfid-board.h"

#define MAX_SHIM_COUNT 255

static FARPROC shim[MAX_SHIM_COUNT];
static struct printerbot_config config;

static uint8_t mainFirmware[0x40] = {0};
static uint8_t paramFirmware[0x40] = {0};

static int printid_status = 0;

// dragons
static int counter_for_31 = 0;
static bool page_started = false;

typedef int (*ogchcusb_MakeThread)(uint16_t maxCount);

int chcusb_MakeThread(uint16_t maxCount) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, maxCount);
    int ret = ((ogchcusb_MakeThread) shim[0])(maxCount);
    return ret;
}


typedef int (*ogchcusb_open)(uint16_t *rResult);

int chcusb_open(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_open) shim[1])(rResult);

    if (ret == 1 && *rResult == 0) {
        if (config.rfid_port > 0) {
            rfid_close();
            if (FAILED(rfid_connect(config.rfid_port, config.rfid_baud))) {
                *rResult = ERROR_RFID_FAIL;
            }
        }
    }

    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef void (*ogchcusb_close)();

void chcusb_close() {
    dprintf_sv(NAME ": %s\n", __func__);
    ((ogchcusb_close) shim[2])();

    if (config.rfid_port > 0) {
        rfid_close();
    }
}


typedef int (*ogchcusb_ReleaseThread)(uint16_t *rResult);

int chcusb_ReleaseThread(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_ReleaseThread) shim[3])(rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_listupPrinter)(uint8_t *rIdArray);

int chcusb_listupPrinter(uint8_t *rIdArray) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_listupPrinter) shim[4])(rIdArray);
    return ret;
}


typedef int (*ogchcusb_listupPrinterSN)(uint64_t *rSerialArray);

int chcusb_listupPrinterSN(uint64_t *rSerialArray) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_listupPrinterSN) shim[5])(rSerialArray);
    return ret;
}


typedef int (*ogchcusb_selectPrinter)(uint8_t printerId, uint16_t *rResult);

int chcusb_selectPrinter(uint8_t printerId, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, printerId);
    int ret = ((ogchcusb_selectPrinter) shim[6])(printerId, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_selectPrinterSN)(uint64_t printerSN, uint16_t *rResult);

int chcusb_selectPrinterSN(uint64_t printerSN, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%ld)\n", __func__, printerSN);
    int ret = ((ogchcusb_selectPrinterSN) shim[7])(printerSN, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_getPrinterInfo)(uint16_t tagNumber, uint8_t *rBuffer, uint32_t *rLen);

int chcusb_getPrinterInfo(uint16_t tagNumber, uint8_t *rBuffer, uint32_t *rLen) {
    dprintf_sv(NAME ": %s(%d, %d)\n", __func__, tagNumber, *rLen);
    int ret = 1;

    if (tagNumber == kPINFTAG_PRINTSTANDBY) {

        // FIXME?: I REALLY REALLY REALLY REALLY do not like this at all, but do more cards need to be sacrificed?
        // Like I don't know why the 310 has this weird status flag... Neither do I know what the deal with the 330's "1007" status is as I can't find that anywhere else...
        if (config.data_manipulation && config.from == 310 && page_started && printid_status > 2212){
            dprintf(NAME ": Querying real standby status...\n");
            ret = ((ogchcusb_getPrinterInfo) shim[8])(tagNumber, rBuffer, rLen);
            dprintf(NAME ": OK (%d)\n", rBuffer[0]);
            if (rBuffer[0] == 0x03 && counter_for_31-- > 0){
                dprintf(NAME ": Setting 0x31 flag\n");
                rBuffer[0] = 0x31;
            } else {
                dprintf(NAME ": Setting 0x00 flag\n");
                rBuffer[0] = 0x00;
            }
        }
    } else {
        ret = ((ogchcusb_getPrinterInfo) shim[8])(tagNumber, rBuffer, rLen);
    }

#if SUPER_VERBOSE
    dump(rBuffer, *rLen);
#endif
    return ret;
}

typedef int (*ogchcusb_imageformat_310)(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width,
                                        uint16_t height, uint8_t *inputImage, uint16_t *rResult);

typedef int (*ogchcusb_imageformat_330)(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width,
                                        uint16_t height, uint16_t *rResult);


int chcusb_imageformat_310(uint16_t format, uint16_t ncomp, uint16_t depth, uint16_t width, uint16_t height,
                           uint8_t *inputImage, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d, %d, %d, %d)\n", __func__, format, ncomp, depth, width, height);

    width = config.to_width;
    height = config.to_height;

    // use correct declaration based on model
    // TODO: use the parameters passed here instead of config
    int ret;
    if (config.to == 310) {
        ret = ((ogchcusb_imageformat_310) shim[9])(format, ncomp, depth, width, height, inputImage, rResult);
    } else if (config.to == 330) {
        ret = ((ogchcusb_imageformat_330) shim[9])(format, ncomp, depth, width, height, rResult);
    } else {
        dprintf(NAME ": Unknown target printer: %d\n", config.to);
        *rResult = ERROR_PRINTER_PARAMETER_ERROR;
        ret = 1;
    }
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_setmtf)(int32_t *mtf);

int chcusb_setmtf(int32_t *mtf) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_setmtf) shim[10])(mtf);
    return ret;
}


typedef int (*ogchcusb_makeGamma)(uint16_t k, uint8_t *intoneR, uint8_t *intoneG, uint8_t *intoneB);

int chcusb_makeGamma(uint16_t k, uint8_t *intoneR, uint8_t *intoneG, uint8_t *intoneB) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, k);
    int ret = ((ogchcusb_makeGamma) shim[11])(k, intoneR, intoneG, intoneB);
    return ret;
}


typedef int (*ogchcusb_setIcctable)(LPCSTR icc1, LPCSTR icc2, uint16_t intents, uint8_t *intoneR, uint8_t *intoneG,
                                    uint8_t *intoneB, uint8_t *outtoneR, uint8_t *outtoneG, uint8_t *outtoneB,
                                    uint16_t *rResult);

int chcusb_setIcctable(LPCSTR icc1, LPCSTR icc2, uint16_t intents, uint8_t *intoneR, uint8_t *intoneG, uint8_t *intoneB,
                       uint8_t *outtoneR, uint8_t *outtoneG, uint8_t *outtoneB, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%s, %s, %d)\n", __func__, icc1, icc2, intents);
    int ret = ((ogchcusb_setIcctable) shim[12])(icc1, icc2, intents, intoneR, intoneG, intoneB, outtoneR, outtoneG,
                                                outtoneB, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_copies)(uint16_t copies, uint16_t *rResult);

int chcusb_copies(uint16_t copies, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, copies);
    int ret = ((ogchcusb_copies) shim[13])(copies, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);

    page_started = false;

    return ret;
}


typedef int (*ogchcusb_status)(uint16_t *rResult);

int chcusb_status(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_status) shim[14])(rResult);

    // FIXME: I really don't like this either!
    if (config.data_manipulation && config.from == 310 && page_started && printid_status > 2212 && *rResult == 1007){
        dprintf(NAME ": Setting status to 0\n");
        *rResult = 0;
        ret = 1;
    }

    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_statusAll)(uint8_t *idArray, uint16_t *rResultArray);

int chcusb_statusAll(uint8_t *idArray, uint16_t *rResultArray) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_statusAll) shim[15])(idArray, rResultArray);
    return ret;
}

typedef int (*ogchcusb_startpage)(uint16_t postCardState, uint16_t *pageId, uint16_t *rResult);

int chcusb_startpage(uint16_t postCardState, uint16_t *pageId, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d)\n", __func__, postCardState, *pageId);

    // 330 does not know state 1
    if (config.to == 330 && postCardState == 1 && config.data_manipulation) {
        postCardState = 3;
        dprintf_sv(NAME ": Convert postCardState to %d\n", postCardState);
    }

    page_started = true;
    counter_for_31 = 3;

    int ret = ((ogchcusb_startpage) shim[16])(postCardState, pageId, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_endpage)(uint16_t *rResult);

int chcusb_endpage(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_endpage) shim[17])(rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_write)(uint8_t *data, uint32_t *writeSize, uint16_t *rResult);

int chcusb_write(uint8_t *data, uint32_t *writeSize, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, *writeSize);

    if (config.from_width == config.to_width && config.from_height == config.to_height) {

        // if we aren't resizing, just skip everything

        int ret = ((ogchcusb_write) shim[18])(data, writeSize, rResult);
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        return ret;

    } else {

        uint32_t orig_writeSize = *writeSize;
        uint32_t len = 0;
        uint8_t *resized = bicubicresize(data, config.from_width, config.from_height, config.to_width, config.to_height,
                                         &len, 3);
        dprintf(NAME ": Image resized from %d to %d bytes\n", *writeSize, len);
        *writeSize = len;

        // okay so this is stupid as since the printer buffer may fill, we can't just return the amount of resized bytes since we don't know the relation between written resized bytes and real bytes, plus calling a partial bicubicresize will crash us, so we write the blocks ourselves until the printer has accepted all bytes and return the full amount of real bytes (that we actually didn't write)

        uint32_t written = 0;
        int ret;
        do {
            ret = ((ogchcusb_write) shim[18])(resized + written, writeSize, rResult);
            dprintf_sv(NAME ": %d bytes written to printer\n", *writeSize);
            written += *writeSize;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            if (*rResult != 0) {
                return ret;
            }
        } while (written < len);
        *writeSize = orig_writeSize;

        free(resized);

        return ret;

    }
}


typedef int (*ogchcusb_writeLaminate)(uint8_t *data, uint32_t *writeSize, uint16_t *rResult);

int chcusb_writeLaminate(uint8_t *data, uint32_t *writeSize, uint16_t *rResult) {
    // TODO CHC320?
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_writeLaminate) shim[19])(data, writeSize, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_writeHolo)(uint8_t *data, uint32_t *writeSize, uint16_t *rResult);

int chcusb_writeHolo(uint8_t *data, uint32_t *writeSize, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, *writeSize);

    if (config.from_width == config.to_width && config.from_height == config.to_height) {

        // if we aren't resizing, just skip everything

        int ret = ((ogchcusb_write) shim[20])(data, writeSize, rResult);
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        return ret;

    } else {

        uint32_t orig_writeSize = *writeSize;
        uint32_t len = 0;
        uint8_t *resized = bicubicresize(data, config.from_width, config.from_height, config.to_width, config.to_height,
                                         &len, 1);
        dprintf(NAME ": Image resized from %d to %d bytes\n", *writeSize, len);
        *writeSize = len;

        // okay so this is stupid as since the printer buffer may fill, we can't just return the amount of resized bytes since we don't know the relation between written resized bytes and real bytes, plus calling a partial bicubicresize will crash us, so we write the blocks ourselves until the printer has accepted all bytes and return the full amount of real bytes (that we actually didn't write)

        uint32_t written = 0;
        int ret;
        do {
            ret = ((ogchcusb_write) shim[20])(resized + written, writeSize, rResult);
            dprintf_sv(NAME ": %d bytes written to printer\n", *writeSize);
            written += *writeSize;
            SUPER_VERBOSE_RESULT_PRINT(*rResult);
            if (*rResult != 0) {
                return ret;
            }
        } while (written < len);
        *writeSize = orig_writeSize;

        free(resized);

        return ret;

    }
}


typedef int (*ogchcusb_setPrinterInfo)(uint16_t tagNumber, uint8_t *rBuffer, uint32_t *rLen, uint16_t *rResult);

int chcusb_setPrinterInfo(uint16_t tagNumber, uint8_t *rBuffer, uint32_t *rLen, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d)\n", __func__, tagNumber, *rLen);
#if SUPER_VERBOSE
    dump(rBuffer, *rLen);
#endif

    if (tagNumber == 0 && config.data_manipulation) { // PAPERINFO
        // FIXME: this should be a struct
        rBuffer[0] = 0x02; // ???
        rBuffer[1] = config.to_width;
        rBuffer[2] = config.to_width >> 8;
        rBuffer[3] = config.to_height;
        rBuffer[4] = config.to_height >> 8;
    } else if (tagNumber == 20 && config.data_manipulation) { // POLISHINFO
        uint8_t byte = rBuffer[0];
        // this controls holo printing somehow?
        if (config.to == 310) {
            // 310, 0x05=holo, 0x02=normal
            if (byte == 0x11 || byte == 0x15) {
                rBuffer[0] = 0x05;
            } else if (byte == 0x02 || byte == 0x12){
                rBuffer[0] = 0x02;
            }
        } else if (config.to == 320) {
            // 320, 0x15=holo, 0x12=normal
            if (byte == 0x11 || byte == 0x05) {
                rBuffer[0] = 0x15;
            } else if (byte == 0x02){
                rBuffer[0] = 0x12;
            }
        } else if (config.to == 330) {
            // 330, 0x11=holo, 0x02=normal
            if (byte == 0x05 || byte == 0x15){
                rBuffer[0] = 0x11;
            } else if (byte == 0x02 || byte == 0x12){
                rBuffer[0] = 0x02;
            }
        }
    }

    int ret = ((ogchcusb_setPrinterInfo) shim[21])(tagNumber, rBuffer, rLen, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_getGamma)(LPCSTR filename, uint8_t *r, uint8_t *g, uint8_t *b, uint16_t *rResult);

int chcusb_getGamma(LPCSTR filename, uint8_t *r, uint8_t *g, uint8_t *b, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%s)\n", __func__, filename);
    int ret = ((ogchcusb_getGamma) shim[22])(filename, r, g, b, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_getMtf)(LPCSTR filename, int32_t *mtf, uint16_t *rResult);

int chcusb_getMtf(LPCSTR filename, int32_t *mtf, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%s)\n", __func__, filename);
    int ret = ((ogchcusb_getMtf) shim[23])(filename, mtf, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_cancelCopies)(uint16_t pageId, uint16_t *rResult);

int chcusb_cancelCopies(uint16_t pageId, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, pageId);
    int ret = ((ogchcusb_cancelCopies) shim[24])(pageId, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_setPrinterToneCurve)(uint16_t type, uint16_t number, uint16_t *data, uint16_t *rResult);

int chcusb_setPrinterToneCurve(uint16_t type, uint16_t number, uint16_t *data, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d)\n", __func__, type, number);
    int ret = ((ogchcusb_setPrinterToneCurve) shim[25])(type, number, data, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_getPrinterToneCurve)(uint16_t type, uint16_t number, uint16_t *data, uint16_t *rResult);

int chcusb_getPrinterToneCurve(uint16_t type, uint16_t number, uint16_t *data, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d)\n", __func__, type, number);
    int ret = ((ogchcusb_getPrinterToneCurve) shim[26])(type, number, data, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_blinkLED)(uint16_t *rResult);

int chcusb_blinkLED(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_blinkLED) shim[27])(rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_resetPrinter)(uint16_t *rResult);

int chcusb_resetPrinter(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_resetPrinter) shim[28])(rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_AttachThreadCount)(uint16_t *rCount, uint16_t *rMaxCount);

int chcusb_AttachThreadCount(uint16_t *rCount, uint16_t *rMaxCount) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_AttachThreadCount) shim[29])(rCount, rMaxCount);
    return ret;
}


typedef int (*ogchcusb_getPrintIDStatus)(uint16_t pageId, uint8_t *rBuffer, uint16_t *rResult);

int chcusb_getPrintIDStatus(uint16_t pageId, uint8_t *rBuffer, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, pageId);
    int ret = ((ogchcusb_getPrintIDStatus) shim[30])(pageId, rBuffer, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
#if SUPER_VERBOSE
    dump(rBuffer, 8);
#endif

    printid_status = (rResult[7] << 8) | rResult[6];

    return ret;
}


typedef int (*ogchcusb_setPrintStandby)(uint16_t position, uint16_t *rResult);

int chcusb_setPrintStandby(uint16_t position, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, position);
    int ret = ((ogchcusb_setPrintStandby) shim[31])(position, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);

    page_started = false;

    return ret;
}


typedef int (*ogchcusb_testCardFeed)(uint16_t mode, uint16_t times, uint16_t *rResult);

int chcusb_testCardFeed(uint16_t mode, uint16_t times, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d)\n", __func__, mode, times);
    int ret = ((ogchcusb_testCardFeed) shim[32])(mode, times, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_exitCard)(uint16_t *rResult);

int chcusb_exitCard(uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_exitCard) shim[33])(rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);

    page_started = false;

    return ret;
}


typedef int (*ogchcusb_getCardRfidTID)(uint8_t *rCardTID, uint16_t *rResult);

int chcusb_getCardRfidTID(uint8_t *rCardTID, uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);

    int ret = 1;
    if (config.rfid_port > 0) {
        if (FAILED(rfid_get_card_tid(rCardTID))) {
            *rResult = ERROR_RFID_FAIL;
        } else {
            *rResult = ERROR_RFID_OK;
        }
    } else {
        ret = ((ogchcusb_getCardRfidTID) shim[34])(rCardTID, rResult);
        dprintf(NAME ": Card TID from Printer:\n");
        dump(rCardTID, CARD_ID_LEN);
    }

    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_commCardRfidReader)(uint8_t *sendData, uint8_t *rRecvData, uint32_t sendSize,
                                           uint32_t *rRecvSize, uint16_t *rResult);

int chcusb_commCardRfidReader(uint8_t *sendData, uint8_t *rRecvData, uint32_t sendSize, uint32_t *rRecvSize,
                              uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %p, %d, %p, %p)\n", __func__, sendData[0], rRecvData, sendSize, rRecvSize, rResult);

    int ret = 1;
    if (config.rfid_port > 0) {
        if (FAILED(rfid_transact(((uint16_t *) sendData)[0], sendData, sendSize, rRecvData, rRecvSize))) {
            *rResult = ERROR_RFID_FAIL;
        } else {
            *rResult = 0;
        }

    } else {
        dprintf("SEND (%d):\n", sendSize);
        dump(sendData, sendSize);
        ret = ((ogchcusb_commCardRfidReader) shim[35])(sendData, rRecvData, sendSize, rRecvSize, rResult);
        dprintf("RECV (%d):\n", *rRecvSize);
        dump(rRecvData, *rRecvSize);
    }

    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_updateCardRfidReader)(uint8_t *data, uint32_t size, uint16_t *rResult);

int chcusb_updateCardRfidReader(uint8_t *data, uint32_t size, uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    if (config.allow_firmware_write != BRICK_PRINTER_MAGIC_ID) {
        *rResult = 0;
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        return 1;
    } else {
        ConfirmBricking();
        int ret = ((ogchcusb_updateCardRfidReader) shim[36])(data, size, rResult);
        SUPER_VERBOSE_RESULT_PRINT(*rResult);
        return ret;
    }
}


typedef int (*ogchcusb_getErrorLog)(uint16_t index, uint8_t *rData, uint16_t *rResult);

int chcusb_getErrorLog(uint16_t index, uint8_t *rData, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, index);
    int ret = ((ogchcusb_getErrorLog) shim[37])(index, rData, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_getErrorStatus)(uint16_t *rBuffer);

int chcusb_getErrorStatus(uint16_t *rBuffer) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_getErrorStatus) shim[38])(rBuffer);
    return ret;
}


typedef int (*ogchcusb_setCutList)(uint8_t *rData, uint16_t *rResult);

int chcusb_setCutList(uint8_t *rData, uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_setCutList) shim[39])(rData, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_setLaminatePattern)(uint16_t index, uint8_t *rData, uint16_t *rResult);

int chcusb_setLaminatePattern(uint16_t index, uint8_t *rData, uint16_t *rResult) {
    // TODO CHC320?
    dprintf_sv(NAME ": %s(%d)\n", __func__, index);
    int ret = ((ogchcusb_setLaminatePattern) shim[40])(index, rData, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_color_adjustment)(LPCSTR filename, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6,
                                         int64_t a7, uint16_t *rResult);

int chcusb_color_adjustment(LPCSTR filename, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7,
                            uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%s, %d, %d, %d, %d, %ld, %ld)\n", __func__, filename, a2, a3, a4, a5, a6, a7);
    int ret = ((ogchcusb_color_adjustment) shim[41])(filename, a2, a3, a4, a5, a6, a7, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_color_adjustmentEx)(int32_t a1, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6,
                                           int64_t a7, uint16_t *rResult);

int chcusb_color_adjustmentEx(int32_t a1, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7,
                              uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d, %d, %d, %d, %ld, %ld)\n", __func__, a1, a2, a3, a4, a5, a6, a7);
    int ret = ((ogchcusb_color_adjustmentEx) shim[42])(a1, a2, a3, a4, a5, a6, a7, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_getEEPROM)(uint8_t index, uint8_t *rData, uint16_t *rResult);

int chcusb_getEEPROM(uint8_t index, uint8_t *rData, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, index);
    int ret = ((ogchcusb_getEEPROM) shim[43])(index, rData, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_setParameter)(uint8_t a1, uint32_t a2, uint16_t *rResult);

int chcusb_setParameter(uint8_t a1, uint32_t a2, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d, %d)\n", __func__, a1, a2);
    int ret = ((ogchcusb_setParameter) shim[44])(a1, a2, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_getParameter)(uint8_t a1, uint8_t *a2, uint16_t *rResult);

int chcusb_getParameter(uint8_t a1, uint8_t *a2, uint16_t *rResult) {
    dprintf_sv(NAME ": %s(%d)\n", __func__, a1);
    int ret = ((ogchcusb_getParameter) shim[45])(a1, a2, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_universal_command)(int32_t a1, uint8_t a2, int32_t a3, uint8_t *a4, uint16_t *rResult);

int chcusb_universal_command(int32_t a1, uint8_t a2, int32_t a3, uint8_t *a4, uint16_t *rResult) {
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_universal_command) shim[46])(a1, a2, a3, a4, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


typedef int (*ogchcusb_writeIred)(uint8_t *a1, uint8_t *a2, uint16_t *rResult);

int chcusb_writeIred(uint8_t *a1, uint8_t *a2, uint16_t *rResult) {
    // TODO CHC320?
    dprintf_sv(NAME ": %s\n", __func__);
    int ret = ((ogchcusb_writeIred) shim[47])(a1, a2, rResult);
    SUPER_VERBOSE_RESULT_PRINT(*rResult);
    return ret;
}


void chcusb_shim_install(struct printerbot_config *cfg) {
    assert(cfg != NULL);

    memcpy(&config, cfg, sizeof(*cfg));

    char path[MAX_PATH];
    if (cfg->from == cfg->to) {
        sprintf(path, ".\\C%dAusb_orig.dll", cfg->to);
    } else {
        sprintf(path, ".\\C%dAusb.dll", cfg->to);
    }
    HINSTANCE ptr = LoadLibraryA(path);
    if (ptr == NULL) {
        dprintf("LoadLibrary(%s) FAILED: %ld\n", path, GetLastError());
        exit(1);
        return;
    }
    int count = 0;
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_MakeThread");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_open");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_close");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_ReleaseThread");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_listupPrinter");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_listupPrinterSN");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_selectPrinter");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_selectPrinterSN");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getPrinterInfo");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_imageformat");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_setmtf");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_makeGamma");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_setIcctable");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_copies");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_status");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_statusAll");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_startpage");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_endpage");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_write");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_writeLaminate");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_writeHolo");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_setPrinterInfo");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getGamma");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getMtf");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_cancelCopies");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_setPrinterToneCurve");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getPrinterToneCurve");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_blinkLED");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_resetPrinter");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_AttachThreadCount");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getPrintIDStatus");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_setPrintStandby");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_testCardFeed");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_exitCard");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getCardRfidTID");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_commCardRfidReader");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_updateCardRfidReader");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getErrorLog");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getErrorStatus");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_setCutList");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_setLaminatePattern");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_color_adjustment");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_color_adjustmentEx");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getEEPROM");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_setParameter");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_getParameter");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_universal_command");
    shim[count++] = GetProcAddressChecked(ptr, "chcusb_writeIred");
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

    dprintf(NAME ": CHC310 shim installed\n");
}

#pragma clang diagnostic pop