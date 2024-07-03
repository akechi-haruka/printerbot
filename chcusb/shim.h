#pragma once

#define NAME "Printerbot[chcusb]"

#include "printerbot/config.h"

enum {
    kPINFTAG_PAPER = 0,
    kPINFTAG_USBINQ = 2,
    kPINFTAG_ENGID = 3,
    kPINFTAG_PRINTCNT = 4,
    kPINFTAG_PRINTCNT2 = 5,
    kPINFTAG_SVCINFO = 7,
    kPINFTAG_PRINTSTANDBY = 8,
    kPINFTAG_MEMORY = 16,
    kPINFTAG_PRINTMODE = 20,
    kPINFTAG_SERIALINFO = 26,
    kPINFTAG_TEMPERATURE = 40,
    kPINFTAG_ERRHISTORY = 50,
    kPINFTAG_TONETABLE = 60,
};

enum {
    ERROR_PRINTER_PARAMETER_ERROR = 1006,
    ERROR_RFID_OK = 2405,
    ERROR_RFID_FAIL = 4051,
};

void chcusb_shim_install(struct printerbot_config* cfg);

int chcusb_MakeThread(uint16_t maxCount);
int chcusb_open(uint16_t * rResult);
void chcusb_close();
int chcusb_ReleaseThread(uint16_t * rResult);
int chcusb_listupPrinter(uint8_t * rIdArray);
int chcusb_listupPrinterSN(uint64_t * rSerialArray);
int chcusb_selectPrinter(uint8_t printerId, uint16_t * rResult);
int chcusb_selectPrinterSN(uint64_t printerSN, uint16_t * rResult);
int chcusb_getPrinterInfo(uint16_t tagNumber, uint8_t * rBuffer, uint32_t * rLen);
int chcusb_imageformat_310(
        uint16_t format,
        uint16_t ncomp,
        uint16_t depth,
        uint16_t width,
        uint16_t height,
        uint8_t * inputImage,
        uint16_t * rResult
);
int chcusb_imageformat_330(
        uint16_t format,
        uint16_t ncomp,
        uint16_t depth,
        uint16_t width,
        uint16_t height,
        uint16_t * rResult
);
int chcusb_setmtf(int32_t * mtf);
int chcusb_makeGamma(uint16_t k, uint8_t * intoneR, uint8_t * intoneG, uint8_t * intoneB);
int chcusb_setIcctable(
        LPCSTR icc1,
        LPCSTR icc2,
        uint16_t intents,
        uint8_t * intoneR,
        uint8_t * intoneG,
        uint8_t * intoneB,
        uint8_t * outtoneR,
        uint8_t * outtoneG,
        uint8_t * outtoneB,
        uint16_t * rResult
);
int chcusb_copies(uint16_t copies, uint16_t * rResult);
int chcusb_status(uint16_t * rResult);
int chcusb_statusAll(uint8_t * idArray, uint16_t * rResultArray);
int chcusb_startpage(uint16_t postCardState, uint16_t * pageId, uint16_t * rResult);
int chcusb_endpage(uint16_t * rResult);
int chcusb_write(uint8_t * data, uint32_t * writeSize, uint16_t * rResult);
int chcusb_writeLaminate(uint8_t * data, uint32_t * writeSize, uint16_t * rResult);
int chcusb_writeHolo(uint8_t * data, uint32_t * writeSize, uint16_t * rResult);
int chcusb_setPrinterInfo(uint16_t tagNumber, uint8_t * rBuffer, uint32_t * rLen, uint16_t * rResult);
int chcusb_getGamma(LPCSTR filename, uint8_t * r, uint8_t * g, uint8_t * b, uint16_t * rResult);
int chcusb_getMtf(LPCSTR filename, int32_t * mtf, uint16_t * rResult);
int chcusb_cancelCopies(uint16_t pageId, uint16_t * rResult);
int chcusb_setPrinterToneCurve(uint16_t type, uint16_t number, uint16_t * data, uint16_t * rResult);
int chcusb_getPrinterToneCurve(uint16_t type, uint16_t number, uint16_t * data, uint16_t * rResult);
int chcusb_blinkLED(uint16_t * rResult);
int chcusb_resetPrinter(uint16_t * rResult);
int chcusb_AttachThreadCount(uint16_t * rCount, uint16_t * rMaxCount);
int chcusb_getPrintIDStatus(uint16_t pageId, uint8_t * rBuffer, uint16_t * rResult);
int chcusb_setPrintStandby(uint16_t position, uint16_t * rResult);
int chcusb_testCardFeed(uint16_t mode, uint16_t times, uint16_t * rResult);
int chcusb_exitCard(uint16_t * rResult);
int chcusb_getCardRfidTID(uint8_t * rCardTID, uint16_t * rResult);
int chcusb_commCardRfidReader(uint8_t * sendData, uint8_t * rRecvData, uint32_t sendSize, uint32_t * rRecvSize, uint16_t * rResult);
int chcusb_updateCardRfidReader(uint8_t * data, uint32_t size, uint16_t * rResult);
int chcusb_getErrorLog(uint16_t index, uint8_t * rData, uint16_t * rResult);
int chcusb_getErrorStatus(uint16_t * rBuffer);
int chcusb_setCutList(uint8_t * rData, uint16_t * rResult);
int chcusb_setLaminatePattern(uint16_t index, uint8_t * rData, uint16_t * rResult);
int chcusb_color_adjustment(LPCSTR filename, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7, uint16_t * rResult);
int chcusb_color_adjustmentEx(int32_t a1, int32_t a2, int32_t a3, int16_t a4, int16_t a5, int64_t a6, int64_t a7, uint16_t * rResult);
int chcusb_getEEPROM(uint8_t index, uint8_t * rData, uint16_t * rResult);
int chcusb_setParameter(uint8_t a1, uint32_t a2, uint16_t * rResult);
int chcusb_getParameter(uint8_t a1, uint8_t * a2, uint16_t * rResult);
int chcusb_universal_command(int32_t a1, uint8_t a2, int32_t a3, uint8_t * a4, uint16_t * rResult);
int chcusb_writeIred(uint8_t* a1, uint8_t* a2, uint16_t* rResult);
