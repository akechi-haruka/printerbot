#pragma once

#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CARD_ID_LEN 12

enum {
    RFID_CMD_CARD_SCAN = 0x06,
    RFID_SUBCMD_SCAN_DATA_START = 0x81,
    RFID_SUBCMD_SCAN_DATA_CARD = 0x82,
    RFID_SUBCMD_SCAN_DATA_END = 0x83,
};

struct __attribute__((__packed__)) rfid_req_any {
    uint8_t cmd;
    uint8_t len;
    uint8_t payload[252];
};

struct __attribute__((__packed__)) rfid_resp_any {
    uint8_t cmd;
    uint8_t subcmd;
    uint8_t len;
    uint8_t data[251];
};

HRESULT rfid_connect(int port, int baud);
HRESULT rfid_transact(uint16_t packet, const uint8_t* payload, uint32_t payload_len, uint8_t* response, uint32_t* response_len);
HRESULT rfid_get_card_tid(uint8_t *rCardTID);
HRESULT rfid_close();