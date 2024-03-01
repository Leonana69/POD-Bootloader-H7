#include "link.h"
#include "usart.h"
#include <boot.h>

#define DEBUG
#include "debug.h"

LinkBuffer linkBufferRx = { 0 };

void linkBufferPutChar(uint8_t c) {
    uint16_t next = LINK_BUFFER_INC_INDEX(linkBufferRx.head);
    if (next != linkBufferRx.tail) {
        linkBufferRx.data[linkBufferRx.head] = c;
        linkBufferRx.head = next;
    }
}
uint8_t linkBufferGetChar() {
    if (linkBufferIsEmpty()) {
        return 0;
    }
    uint8_t c = linkBufferRx.data[linkBufferRx.tail];
    linkBufferRx.tail = LINK_BUFFER_INC_INDEX(linkBufferRx.tail);
    return c;
}

bool linkBufferIsEmpty() {
    return linkBufferRx.head == linkBufferRx.tail;
}

typedef enum {
    PODTP_STATE_START_1,
    PODTP_STATE_START_2,
    PODTP_STATE_LENGTH,
    PODTP_STATE_RAW_DATA,
    PODTP_STATE_CRC_1,
    PODTP_STATE_CRC_2
} LinkState;

bool linkGetPacket(PodtpPacket *packet) {
    static LinkState state = PODTP_STATE_START_1;
    static uint8_t length = 0;
    static uint8_t check_sum[2] = { 0 };

    while (!linkBufferIsEmpty()) {
        uint8_t c = linkBufferGetChar();
        switch (state) {
            case PODTP_STATE_START_1:
                if (c == PODTP_START_BYTE_1) {
                    state = PODTP_STATE_START_2;
                }
                break;
            case PODTP_STATE_START_2:
                state = (c == PODTP_START_BYTE_2) ? PODTP_STATE_LENGTH : PODTP_STATE_START_1;
                break;
            case PODTP_STATE_LENGTH:
                length = c;
                if (length > PODTP_MAX_DATA_LEN || length == 0) {
                    state = PODTP_STATE_START_1;
                } else {
                    packet->length = 0;
                    check_sum[0] = check_sum[1] = c;
                    state = PODTP_STATE_RAW_DATA;
                }
                break;
            case PODTP_STATE_RAW_DATA:
                packet->raw[packet->length++] = c;
                check_sum[0] += c;
                check_sum[1] += check_sum[0];
                if (packet->length >= length) {
                    state = PODTP_STATE_CRC_1;
                }
                break;
            case PODTP_STATE_CRC_1:
                state = (c == check_sum[0]) ? PODTP_STATE_CRC_2 : PODTP_STATE_START_1;
                break;
            case PODTP_STATE_CRC_2:
                state = PODTP_STATE_START_1;
                if (c == check_sum[1]) {
                    return true;
                }
                break;
        }
    }
    return false;
}

void linkSendPacket(PodtpPacket *packet) {
    uint8_t check_sum[2] = { 0 };
    check_sum[0] = check_sum[1] = packet->length;
    esp32UartPutChar(PODTP_START_BYTE_1);
    esp32UartPutChar(PODTP_START_BYTE_2);
    esp32UartPutChar(packet->length);
    for (uint8_t i = 0; i < packet->length; i++) {
        check_sum[0] += packet->raw[i];
        check_sum[1] += check_sum[0];
        esp32UartPutChar(packet->raw[i]);
    }
    esp32UartPutChar(check_sum[0]);
    esp32UartPutChar(check_sum[1]);
}

bool linkProcessPacket(PodtpPacket *packet) {
    uint8_t len = packet->length;

    if (len <= 1 || packet->type != PODTP_TYPE_BOOT_LOADER) {
        return false;
    }

    // Bootloader command, currently only support write flash
    if (packet->port == PORT_LOAD_BUFFER) {
        return bootLoad(packet);
    } else if (packet->port == PORT_WRITE_FLASH) {
        return bootWrite(packet);
    }
    return false;
}