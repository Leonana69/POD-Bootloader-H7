#ifndef __PODTP_H__
#define __PODTP_H__

#include <stdint.h>

#define PODTP_MAX_DATA_LEN 126

#define PODTP_START_BYTE_1 0xAD
#define PODTP_START_BYTE_2 0x6E

enum {
    PODTP_TYPE_ACK = 0x1,
    PODTP_TYPE_COMMAND = 0x2,
    PODTP_TYPE_ESP32 = 0xE,
    PODTP_TYPE_BOOT_LOADER = 0xF,
};

enum {
    // PODTP_TYPE_ACK
    PODTP_PORT_ERROR = 0,
    PODTP_PORT_OK = 1,
    // PODTP_TYPE_COMMAND
    
    // PODTP_TYPE_ESP32
    PORT_ECHO = 0,
    PORT_START_STM32_BOOTLOADER = 1,
    PORT_START_STM32_FIRMWARE = 2,
    PORT_DISABLE_STM32 = 3,
    PORT_ENABLE_STM32 = 4,
    // PODTP_TYPE_BOOT_LOADER
    PORT_LOAD_BUFFER = 1,
    PORT_WRITE_FLASH = 2,
};

typedef struct {
    uint8_t length;
    union {
        struct {
            union {
                uint8_t header;
                struct {
                    uint8_t port:4;
                    uint8_t type:4;
                };
            };
            uint8_t data[PODTP_MAX_DATA_LEN];
        } __attribute__((packed));
        uint8_t raw[PODTP_MAX_DATA_LEN + 1];
    };
} PodtpPacket;

#endif // __PODTP_H__