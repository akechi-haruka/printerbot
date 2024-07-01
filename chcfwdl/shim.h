#pragma once

#define NAME "Printerbot[chcfwdl]"

#include "printerbot/config.h"

#define kPINFTAG_PAPER 0
#define kPINFTAG_USBINQ 2
#define kPINFTAG_ENGID 3
#define kPINFTAG_PRINTCNT 4
#define kPINFTAG_PRINTCNT2 5
#define kPINFTAG_SVCINFO 7
#define kPINFTAG_PRINTSTANDBY 8
#define kPINFTAG_MEMORY 16
#define kPINFTAG_PRINTMODE 20
#define kPINFTAG_SERIALINFO 26
#define kPINFTAG_TEMPERATURE 40
#define kPINFTAG_ERRHISTORY 50
#define kPINFTAG_TONETABLE 60

void chcfwdl_shim_install(struct printerbot_config* cfg);

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