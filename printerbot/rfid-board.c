#include <windows.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
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

#define CHECKSUM_IS_ERROR 0
#define TRY_SALVAGE_COMM_SYNC 1

static HANDLE hSerial = INVALID_HANDLE_VALUE;
static int com_port;
static int com_baud;

HRESULT rfid_connect(int port, int baud) {
    dprintf(RFIDNAME ": Connect COM%d@%d\n", port, baud);

    com_port = port;
    com_baud = baud;

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
    dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;
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

    return rfid_reset();
}

HRESULT rfid_encode(const uint8_t *in, uint32_t inlen, uint8_t *out, uint32_t *outlen){
    if (in == NULL || out == NULL || outlen == NULL){
        return E_HANDLE;
    }
    if (inlen < 2){
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

HRESULT serial_read_single_byte(HANDLE handle, uint8_t* ptr){
    DWORD read = 0;
    if (!ReadFile(handle, ptr, 1, &read, NULL)){
        uint32_t er = GetLastError();
        dprintf(RFIDNAME ": Failed to read from serial port: %u\n", er);
        return HRESULT_FROM_WIN32(er);
    }
    if (read == 0){
        dprintf(RFIDNAME ": Stream was empty\n");
        return HRESULT_FROM_WIN32(E_FAIL);
    }
    return S_OK;
}

HRESULT rfid_decoding_read(HANDLE handle, uint8_t *out, uint32_t *outlen){
    if (handle == NULL || out == NULL || outlen == NULL){
        return E_HANDLE;
    }

    const uint32_t len_byte_offset = 3;
    uint8_t checksum = 0;
    uint32_t offset = 0;
    int bytes_left = COMM_BUF_SIZE;
    HRESULT hr;
    bool escape_flag = false;

    do {
        hr = serial_read_single_byte(handle, out + offset);
        if (FAILED(hr)){
            return hr;
        }

        uint8_t byte = *(out + offset);

        if (offset == len_byte_offset){
            bytes_left = byte;
        }

        if (offset == 0){
            if (byte != 0xE0){
#if TRY_SALVAGE_COMM_SYNC
                dprintf(RFIDNAME ": WARNING! Garbage on line: %x\n", byte);
                continue;
#else
                dprintf(RFIDNAME ": Failed to read from serial port: RFID decode failed: Sync failure: %x\n", byte);
                return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
#endif
            }
            offset++;
        } else if (byte == 0xD0){
            escape_flag = true;
        } else {
            if (escape_flag) {
                byte += 1;
                escape_flag = false;
                bytes_left++;
            } else if (byte == 0xE0){
                dprintf(RFIDNAME ": Failed to read from serial port: RFID decode failed: Found unexpected sync byte in stream at pos %d\n", offset);
                return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
            }
            checksum += byte;
            offset++;
        }
    } while (bytes_left-- > 0);

    hr = serial_read_single_byte(handle, out + offset);
    if (FAILED(hr)){
        return hr;
    }

    uint8_t schecksum = *(out + offset);

    if (checksum != schecksum){
#if CHECKSUM_IS_ERROR
        dprintf(RFIDNAME ": Failed to read from serial port: RFID decode failed: Checksum failed: expected %d, got %d\n", checksum, schecksum);
        return HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR);
#else
        dprintf(RFIDNAME ": Decode: WARNING! Checksum mismatch: expected %d, got %d\n", checksum, schecksum);
#endif
    }

#if SUPER_VERBOSE
    dprintf(RFIDNAME": Data received from serial (%d):\n", offset);
    dump(out, offset);
#endif

    // strip sync and checksum byte from response
    *outlen = offset - 2;
    memcpy(out, out + 1, *outlen);

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
    int attempt = 1;

    do {
        if (attempt > 1){
            dprintf(RFIDNAME ": RETRY! #%d\n", attempt);
        }
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

        hr = rfid_decoding_read(hSerial, response, response_len);
    } while (!SUCCEEDED(hr) && attempt++ < 5);

#if SUPER_VERBOSE
        dprintf("RECV (%d):\n", *response_len);
        dump(response, *response_len);
#endif

    return hr;
}

HRESULT rfid_reset(){
    dprintf(RFIDNAME ": Reset\n");

    if (hSerial == INVALID_HANDLE_VALUE){
        return E_HANDLE;
    }

    HRESULT hr;

    struct rfid_req_any reset_cmd;
    reset_cmd.cmd = RFID_CMD_RESET;
    reset_cmd.len = 0;

    struct rfid_resp_any resp_cmd;
    uint32_t resp_len = sizeof(resp_cmd);

    hr = rfid_transact(0, (uint8_t*) &reset_cmd, 2, (uint8_t *) &resp_cmd, &resp_len);

    if (FAILED(hr)){
        dprintf(RFIDNAME ": Reset failed, transact failed: %ld\n", hr);
        return hr;
    }

    dprintf(RFIDNAME ": Reset: %d\n", resp_cmd.subcmd);

    return S_OK;
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
        resp_len = sizeof(resp_cmd);
        hr = rfid_transact(0, NULL, 0, (uint8_t *) &resp_cmd, &resp_len);

        if (FAILED(hr)){
            dprintf(RFIDNAME ": GetCardTID failed, transact failed: %ld\n", hr);
            return hr;
        }

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