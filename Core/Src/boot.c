#include <string.h>
#include "boot.h"
#include "gpio.h"

#define DEBUG
#include "debug.h"

void bootPinInit() {
    // A4 is the boot pin
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

bool bootPinDeInit() {
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);
    __HAL_RCC_GPIOA_CLK_DISABLE();
    return true;
}

bool startFirmware() {
    // If the boot pin is low, start the firmware
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET;
}

static const uint32_t sector_offset[] = {
    [0]  = 0x00000000,
    [1]  = 0x00004000,
    [2]  = 0x00008000,
    [3]  = 0x0000C000,
    [4]  = 0x00010000,
    [5]  = 0x00020000,
    [6]  = 0x00040000,
    [7]  = 0x00060000,
    [8]  = 0x00080000,
    [9]  = 0x000A0000,
    [10] = 0x000C0000,
    [11] = 0x000E0000,
};

uint8_t boot_buffer[LOCAL_BUFFER_PAGE * FIRMWARE_PAGE_SIZE];

typedef struct {
    uint16_t bufferPage;
    uint16_t flashPage;
    uint16_t numPages;
} __attribute__((__packed__)) WriteFlash;

typedef struct {
    uint16_t bufferPage;
    uint16_t offset;
} __attribute__((__packed__)) LoadBuffer;

int count = 0;
bool bootLoad(PodtpPacket *packet) {
    uint8_t len = packet->length - 1;
    packet->length = 1;
    LoadBuffer *lb = (LoadBuffer *)packet->data;
    uint8_t data_size = len - sizeof(LoadBuffer);
    if (lb->bufferPage >= LOCAL_BUFFER_PAGE || lb->offset >= FIRMWARE_PAGE_SIZE
        || lb->bufferPage * FIRMWARE_PAGE_SIZE + lb->offset + data_size
            >= LOCAL_BUFFER_PAGE * FIRMWARE_PAGE_SIZE) {
        packet->type = PODTP_TYPE_ERROR;
        return true;
    }
    // DEBUG_PRINT("Lb [%d]:bp=%d,off=%d,len=%d\n", count, lb->bufferPage, lb->offset, data_size);
    memcpy(boot_buffer + (lb->bufferPage * FIRMWARE_PAGE_SIZE) + lb->offset, packet->data + sizeof(LoadBuffer), data_size);
    count++;
    packet->type = PODTP_TYPE_ACK;
    return true;
}

bool bootWrite(PodtpPacket *packet) {
    WriteFlash *wf = (WriteFlash *)packet->data;

    packet->length = 1;
    if (wf->bufferPage >= LOCAL_BUFFER_PAGE || wf->flashPage < FIRMWARE_START_PAGE
        || wf->flashPage + wf->numPages > FIRMWARE_END_PAGE) {
        packet->type = PODTP_TYPE_ERROR;
        return true;
    }

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |FLASH_FLAG_WRPERR |
            FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    __disable_irq();

    // Erase the page(s) by sector
    bool error = false;
    for (uint16_t i = 0; i < wf->numPages; i++) {
        for (int j = 0; j < 12; j++) {
            if (((uint32_t)(wf->flashPage + i)) * FIRMWARE_PAGE_SIZE == sector_offset[j]) {
                FLASH_EraseInitTypeDef EraseInitStruct;
                EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
                EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
                EraseInitStruct.NbSectors = 1;
                EraseInitStruct.Sector = j;
                uint32_t SectorError = 0;
                // SPL: 
                // FLASH_EraseSector(j << 3, VoltageRange_3);
                // HAL:
                if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
                    error = true;
                    break;
                }
            }
        }
        if (error) {
            break;
        }
    }

    if (!error) {
        // Write the data, long per long
        uint32_t flashAddress = FLASH_BASE + (wf->flashPage * FIRMWARE_PAGE_SIZE);
        uint32_t *buffer = (uint32_t *)(boot_buffer + (wf->bufferPage * FIRMWARE_PAGE_SIZE));

        // for (int i = 0; i < 7*1024/16; i++) {
        //     for (int j = 0; j < 16; j++) {
        //         DEBUG_PRINT("%02x ", boot_buffer[i*16+j]);
        //     }
        //     DEBUG_PRINT("\n");
        // }

        int word_size = sizeof(uint32_t);
        for (int i = 0; i < (wf->numPages * FIRMWARE_PAGE_SIZE) / word_size; i++, flashAddress += word_size) {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flashAddress, buffer[i]) != HAL_OK) {
                error = true;
                break;
            }
        }
    }
    
    HAL_FLASH_Lock();
    __enable_irq();

    packet->type = error ? PODTP_TYPE_ERROR : PODTP_TYPE_ACK;
    return true;
}