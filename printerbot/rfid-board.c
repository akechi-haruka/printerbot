#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "printerbot/config.h"

#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define SUPER_VERBOSE 1
#include "util/dump.h"
#include "util/dprintf.h"
#include "rfid-board.h"

#define RFIDNAME "Printerbot[RFID]"

#define CHECK_OFFSET_BOUNDARY(pos, max) \
if (pos >= max){ \
    return E_NOT_SUFFICIENT_BUFFER; \
}
#define COMM_BUF_SIZE 255

static HANDLE hSerial = INVALID_HANDLE_VALUE;

HRESULT rfid_connect(int port, int baud) {
    dprintf(RFIDNAME ": Connect COM%d@%d\n", port, baud);

    char portname[16];
    sprintf(portname, "COM%d", port);

    hSerial = CreateFile(
            portname,
            GENERIC_READ | GENERIC_WRITE,
            0,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        uint32_t er = GetLastError();
        dprintf(RFIDNAME ": Failed to connect: %u\n", er);
        return HRESULT_FROM_WIN32(er);
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        uint32_t er = GetLastError();
        dprintf(RFIDNAME ": Failed to get port parameters: %u\n", er);
        hSerial = INVALID_HANDLE_VALUE;
        return HRESULT_FROM_WIN32(er);
    }
    dcbSerialParams.BaudRate = baud;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    /*dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;*/
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        uint32_t er = GetLastError();
        dprintf(RFIDNAME ": Failed to set port parameters: %u\n", er);
        hSerial = INVALID_HANDLE_VALUE;
        return HRESULT_FROM_WIN32(er);
    }

    COMMTIMEOUTS cto;
    GetCommTimeouts(hSerial,&cto);
    cto.ReadIntervalTimeout = 0;
    cto.ReadTotalTimeoutConstant = 1000;
    cto.ReadTotalTimeoutMultiplier = 0;
    cto.WriteTotalTimeoutConstant = 1000;
    cto.WriteTotalTimeoutMultiplier = 0;
    SetCommTimeouts(hSerial,&cto);

    dprintf(RFIDNAME ": Successfully opened port\n");
    return S_OK;
}

HRESULT rfid_encode(const uint8_t *in, uint32_t inlen, uint8_t *out, uint32_t *outlen){
    if (in == NULL || out == NULL || outlen == NULL){
        return E_HANDLE;
    }
    if (inlen < 4){
        return E_INVALIDARG;
    }
    if (*outlen < inlen + 2){
        return E_NOT_SUFFICIENT_BUFFER;
    }

    uint8_t checksum = 0;
    uint32_t offset = 0;

    out[offset++] = 0xE0;
    for (int i = 0; i < inlen; i++){

        uint8_t byte = in[i];

        if (byte == 0xE0 || byte == 0xD0) {
            CHECK_OFFSET_BOUNDARY(offset+2, *outlen)
            out[offset++] = 0xD0;
            out[offset++] = byte - 1;
        } else {
            CHECK_OFFSET_BOUNDARY(offset+1, *outlen)
            out[offset++] = byte;
        }

        checksum += byte;
    }
    CHECK_OFFSET_BOUNDARY(offset+1, *outlen)
    out[offset++] = checksum;
    *outlen = offset;

    return S_OK;
}

HRESULT rfid_decode(const uint8_t *in, uint32_t inlen, uint8_t *out, uint32_t *outlen){
    if (out == NULL || outlen == NULL){
        return E_HANDLE;
    }
    if (inlen < 6){
        return E_INVALIDARG;
    }
    if (*outlen < inlen - 2){
        return E_NOT_SUFFICIENT_BUFFER;
    }

    uint8_t checksum = 0;
    uint32_t offset = 0;

    if (in[offset++] != 0xE0){
        dprintf(RFIDNAME ": Failed to read from serial port: RFID decode failed: Sync failure: %x\n", in[0]);
        return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
    }

    for (int i = 0; i < inlen - 1; i++){
        uint8_t byte = in[i];

        if (byte == 0xE0){
            dprintf(RFIDNAME ": Failed to read from serial port: RFID decode failed: Found unexpected sync byte in stream at pos %d\n", i);
            return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
        } else if (byte == 0xD0){
            CHECK_OFFSET_BOUNDARY(offset+1, *outlen)
            uint8_t ebyte = in[++i];
            if (ebyte == 0xD0){
                dprintf(RFIDNAME ": Failed to read from serial port: RFID decode failed: Found unexpected escape byte in stream at pos %d\n", i);
                return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
            }
            out[offset++] = ebyte + 1;
            checksum += ebyte + 1;
        } else {
            CHECK_OFFSET_BOUNDARY(offset+1, *outlen)
            out[offset++] = byte;
            checksum += byte;
        }
    }

    uint8_t schecksum = in[inlen - 1];
    if (checksum != schecksum){
        dprintf(RFIDNAME ": Failed to read from serial port: RFID decode failed: Checksum failed: expected %d, got %d\n", checksum, schecksum);
        return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
    }

    *outlen = offset;

    return S_OK;
}

HRESULT rfid_transact(uint16_t packet, const uint8_t *payload, uint32_t payload_len, uint8_t *response, uint32_t *response_len) {

    if (response == NULL || response_len == NULL){
        return E_HANDLE;
    }

    dprintf(RFIDNAME ": Transact: %d (len=%d, buf=%d)\n", packet, payload_len, *response_len);

    if (hSerial == INVALID_HANDLE_VALUE){
        return E_HANDLE;
    }

    uint32_t comm_buf_size;
    HRESULT hr;

    if (payload != NULL && payload_len > 0) {
#if SUPER_VERBOSE
        dprintf("SEND (%d):\n", payload_len);
        dump(payload, payload_len);
#endif

        comm_buf_size = COMM_BUF_SIZE;
        uint8_t comm_out[COMM_BUF_SIZE];

        hr = rfid_encode(payload, payload_len, comm_out, &comm_buf_size);
        if (FAILED(hr)) {
            dprintf(RFIDNAME ": Failed to encode packet: %ld\n", hr);
            return hr;
        }

        DWORD send_written = 0;
        if (!WriteFile(hSerial, comm_out, comm_buf_size, &send_written, NULL)) {
            uint32_t er = GetLastError();
            dprintf(RFIDNAME ": Failed to write to serial port: %u\n", er);
            return HRESULT_FROM_WIN32(er);
        }

        dprintf_sv(RFIDNAME ": Send OK (%ld/%d)\n", send_written, comm_buf_size);
    }

    comm_buf_size = COMM_BUF_SIZE;
    uint8_t comm_in[COMM_BUF_SIZE];

    DWORD frame_len = 0;
    if (!ReadFile(hSerial, comm_in, 4, &frame_len, NULL)){
        uint32_t er = GetLastError();
        dprintf(RFIDNAME ": Failed to read from serial port: %u\n", er);
        return HRESULT_FROM_WIN32(er);
    }

    if (frame_len < 4){
        dprintf(RFIDNAME ": Failed to read from serial port: timeout (%lu bytes)\n", frame_len);
        return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
    }

    dprintf_sv(RFIDNAME ": Header Read OK (%ld/4)\n", frame_len);

    DWORD frame_payload_len = comm_in[3] + 1;
    if (!ReadFile(hSerial, (uint8_t*)(&comm_in) + 4, frame_payload_len, &frame_payload_len, NULL)){
        uint32_t er = GetLastError();
        dprintf(RFIDNAME ": Failed to read from serial port: %u\n", er);
        return HRESULT_FROM_WIN32(er);
    }

    hr = rfid_decode(comm_in, frame_len + frame_payload_len, response, response_len);
    if (FAILED(hr)){
        dprintf(RFIDNAME ": Failed to decode packet: %ld\n", hr);
        return hr;
    }

#if SUPER_VERBOSE
        dprintf("RECV (%d):\n", *response_len);
        dump(response, *response_len);
#endif

    return hr;
}

HRESULT rfid_get_card_tid(uint8_t *rCardTID){
    dprintf(RFIDNAME ": Get Card TID\n");

    if (hSerial == INVALID_HANDLE_VALUE){
        return E_HANDLE;
    }

    HRESULT hr;

    struct rfid_req_any scan_cmd;
    scan_cmd.cmd = RFID_CMD_CARD_SCAN;
    scan_cmd.len = 0;

    struct rfid_resp_any resp_cmd;
    uint32_t resp_len = sizeof(resp_cmd);

    hr = rfid_transact(RFID_CMD_CARD_SCAN, (uint8_t*) &scan_cmd, 2, (uint8_t *) &resp_cmd, &resp_len);

    if (FAILED(hr)){
        dprintf(RFIDNAME ": GetCardTID failed, scan start transact failed: %ld\n", hr);
        return hr;
    }

    if (resp_cmd.subcmd != RFID_SUBCMD_SCAN_DATA_START){
        dprintf(RFIDNAME ": GetCardTID failed, unexpected subcommand: %d\n", resp_cmd.subcmd);
        return E_FAIL;
    }

    int cards_read = 0;
    bool end_read = false;
    do {
        hr = rfid_transact(0, NULL, 0, (uint8_t *) &resp_cmd, &resp_len);

        if (resp_cmd.subcmd == RFID_SUBCMD_SCAN_DATA_CARD){
            memcpy(rCardTID, resp_cmd.data, resp_cmd.len);
            cards_read++;
        } else if (resp_cmd.subcmd == RFID_SUBCMD_SCAN_DATA_END){
            end_read = true;
        } else {
            dprintf(RFIDNAME ": GetCardTID failed, unexpected subcommand: %d\n", resp_cmd.subcmd);
            return E_FAIL;
        }
    } while (!end_read);


    dprintf(RFIDNAME ": GetCardTID: Card Count: %d\n", cards_read);
    dump(rCardTID, CARD_ID_LEN);

    return S_OK;
}

HRESULT rfid_close() {
    dprintf(RFIDNAME ": Close\n");
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
    return S_OK;
}