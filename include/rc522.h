#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "stdint.h"

// RC522 Command set
#define PCD_IDLE            0x00
#define PCD_MEM             0x01
#define PCD_GEN_RANDOM_ID   0x02
#define PCD_CALC_CRC        0x03
#define PCD_TRANSMIT        0x04
#define PCD_NO_CMD_CHANGE   0x07
#define PCD_RECEIVE         0x08
#define PCD_TRANSCEIVE      0x0C
#define PCD_MFAuthent       0x0E
#define PCD_SOFTRESET       0x0F
#define PCD_RESETPHASE      0x0F // nilai untuk reset

// RC522 PICC commands
#define PICC_REQIDL         0x26
#define PICC_REQALL         0x52
#define PICC_ANTICOLL       0x93
#define PICC_SELECTTAG      0x93
#define PICC_AUTHENT1A      0x60
#define PICC_AUTHENT1B      0x61
#define PICC_READ           0x30
#define PICC_WRITE          0xA0
#define PICC_DECREMENT      0xC0
#define PICC_INCREMENT      0xC1
#define PICC_RESTORE        0xC2
#define PICC_TRANSFER       0xB0
#define PICC_HALT           0x50

// RC522 registers
#define CommandReg          0x01
#define CommIEnReg          0x02
#define DivIEnReg           0x03
#define CommIrqReg          0x04
#define DivIrqReg           0x05
#define ErrorReg            0x06
#define Status1Reg          0x07
#define Status2Reg          0x08
#define FIFODataReg         0x09
#define FIFOLevelReg        0x0A
#define WaterLevelReg       0x0B
#define ControlReg          0x0C
#define BitFramingReg       0x0D
#define CollReg             0x0E
#define ModeReg             0x11
#define TxModeReg           0x12
#define RxModeReg           0x13
#define TxControlReg        0x14
#define TxASKReg            0x15
#define TxSelReg            0x16
#define RxSelReg            0x17
#define RxThresholdReg      0x18
#define DemodReg            0x19
#define MfTxReg             0x1C
#define MfRxReg             0x1D
#define SerialSpeedReg      0x1F
#define CRCResultRegH       0x21
#define CRCResultRegL       0x22
#define ModWidthReg         0x24
#define RFCfgReg            0x26
#define GsNReg              0x27
#define CWGsPReg            0x28
#define ModGsPReg           0x29
#define TModeReg            0x2A
#define TPrescalerReg       0x2B
#define TReloadRegH         0x2C
#define TReloadRegL         0x2D
#define TCounterValueRegH   0x2E
#define TCounterValueRegL   0x2F
#define VersionReg          0x37

// crc calculate
#define DivIrqReg      0x05
#define CRCResultRegL  0x22
#define CRCResultRegM  0x21
#define PCD_CALCCRC    0x03

// auth
#define Status2Reg     0x08
#define PCD_AUTHENT    0x0E

// Status codes
#define Status_OK              0
#define Status_ERROR           1
#define Status_COLLISION       2
#define Status_TIMEOUT         3
#define Status_NO_ROOM         4
#define Status_INTERNAL_ERROR  5
#define Status_INVALID         6
#define Status_CRC_WRONG       7
#define Status_MIFARE_NACK     8
#define MAX_LEN                16

#define MFRC522_CS_GPIO      32 //gpio untuk esp32
#define MFRC522_RST_GPIO     13

esp_err_t rc522_init(spi_host_device_t spi_host);
uint8_t rc522_read_reg(uint8_t reg);
void rc522_write_reg(uint8_t reg, uint8_t val);

void rc522_antenna_on(); //enable antena

bool rc522_request(uint8_t *atqa); // REQA dan ATQA
bool rc522_anticoll(uint8_t *uid);

// otentikasi
esp_err_t rc522_auth(uint8_t auth_mode, uint8_t block_addr, uint8_t *key, uint8_t *uid);
esp_err_t rc522_read_block(uint8_t block_addr, uint8_t *block_data);

// select
esp_err_t rc522_select(uint8_t *uid);
// brute force finder
bool brute_force_key_finder(uint8_t *uid, uint8_t *found_key);