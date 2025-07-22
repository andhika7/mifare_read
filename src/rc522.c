#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdio.h"
#include "string.h"
#include "rc522.h"

static spi_device_handle_t spi;

// rc read register
uint8_t rc522_read_reg(uint8_t reg) {
    uint8_t tx[] = { ((reg << 1) & 0x7E) | 0x80, 0x00 };
    uint8_t rx[2] = {0};

    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    spi_device_transmit(spi, &t);
    return rx[1];
}

// rc write register
void rc522_write_reg(uint8_t reg, uint8_t val) {
    uint8_t tx[] = { (reg << 1) & 0x7E, val };

    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx,
    };

    spi_device_transmit(spi, &t);
}

// rc init
esp_err_t rc522_init(spi_host_device_t spi_host) {
    esp_err_t ret;

    gpio_set_direction(MFRC522_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(MFRC522_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    spi_bus_config_t buscfg = {
        .miso_io_num = 26,
        .mosi_io_num = 25,
        .sclk_io_num = 33,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = MFRC522_CS_GPIO,
        .queue_size = 1,
    };

    ret = spi_bus_initialize(spi_host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return ret;

    ret = spi_bus_add_device(spi_host, &devcfg, &spi);
    if (ret != ESP_OK) return ret;

    // Soft reset RC522
    rc522_write_reg(CommandReg, PCD_RESETPHASE);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Init recommended by datasheet
    rc522_write_reg(TModeReg, 0x8D);
    rc522_write_reg(TPrescalerReg, 0x3E);
    rc522_write_reg(TReloadRegL, 30);
    rc522_write_reg(TReloadRegH, 0);

    rc522_write_reg(TxASKReg, 0x40);
    rc522_write_reg(ModeReg, 0x3D);

    rc522_antenna_on();

    // Check version
    uint8_t version = rc522_read_reg(VersionReg);
    printf("RC522 VersionReg: 0x%02X\n", version); // mencetak versi firmware MFRC522 (0x92:versi 2.0)

    return ESP_OK;
}

// fungsi meng-enable antena 
void rc522_antenna_on() {
    uint8_t val = rc522_read_reg(TxControlReg);
    if (!(val & 0x03)) {
        rc522_write_reg(TxControlReg, val | 0x03);
    }
}

// kirim REQA (0x26) dan membaca ATQA (2-byte)
bool rc522_request(uint8_t *atqa) {
    // Clear all IRQs
    rc522_write_reg(CommIrqReg, 0x7F);

    // Flush FIFO
    rc522_write_reg(FIFOLevelReg, 0x80);

    // Set BitFramingReg to 7 bits
    rc522_write_reg(BitFramingReg, 0x07);

    // Write REQA
    rc522_write_reg(FIFODataReg, 0x26);

    // Start Transceive
    rc522_write_reg(CommandReg, PCD_TRANSCEIVE);

    // StartSend bit high (BitFramingReg bit 7)
    rc522_write_reg(BitFramingReg, 0x87);

    // Wait for the RxIRq bit (bit 4)
    int i = 0;
    uint8_t irq;
    do {
        irq = rc522_read_reg(CommIrqReg);
        i++;
    } while (!(irq & 0x30) && i < 200);

    uint8_t error = rc522_read_reg(ErrorReg);
    uint8_t fifo_level = rc522_read_reg(FIFOLevelReg);

    // printf("REQA -> IRQ: 0x%02X, ERR: 0x%02X, FIFO: %d\n", irq, error, fifo_level); // line untuk debug

    if ((irq & 0x30) && !(error & 0x1B) && fifo_level >= 2) {
        atqa[0] = rc522_read_reg(FIFODataReg);
        atqa[1] = rc522_read_reg(FIFODataReg);
        return true;
    }
    return false;
}

bool rc522_anticoll(uint8_t *uid) {
    // Clear all IRQs
    rc522_write_reg(CommIrqReg, 0x7F);

    // Flush FIFO
    rc522_write_reg(FIFOLevelReg, 0x80);

    // Send anticollision command
    rc522_write_reg(BitFramingReg, 0x00); // send full bytes

    rc522_write_reg(FIFODataReg, 0x93); // anticollision command
    rc522_write_reg(FIFODataReg, 0x20); // NVB

    rc522_write_reg(CommandReg, PCD_TRANSCEIVE);
    rc522_write_reg(BitFramingReg, 0x80); // StartSend

    int i = 0;
    uint8_t irq;
    do {
        irq = rc522_read_reg(CommIrqReg);
        i++;
    } while (!(irq & 0x30) && i < 200);

    uint8_t error = rc522_read_reg(ErrorReg);
    uint8_t fifo_level = rc522_read_reg(FIFOLevelReg);

    // printf("Anticoll -> IRQ: 0x%02X, ERR: 0x%02X, FIFO: %d\n", irq, error, fifo_level); // line untuk debug

    if ((irq & 0x30) && !(error & 0x1B) && fifo_level >= 5) {
        for (int i = 0; i < 5; i++) {
            uid[i] = rc522_read_reg(FIFODataReg);
        }
        return true;
    }
    return false;
}

// CRC Calculation
uint8_t rc522_calculate_crc(uint8_t *data, uint8_t length, uint8_t *result) {
    rc522_write_reg(CommandReg, PCD_IDLE);
    rc522_write_reg(DivIrqReg, 0x04); // Clear CRCIRq

    rc522_write_reg(FIFOLevelReg, 0x80); // Flush FIFO

    for (uint8_t i = 0; i < length; i++) {
        rc522_write_reg(FIFODataReg, data[i]);
    }

    rc522_write_reg(CommandReg, PCD_CALCCRC);

    uint8_t i = 0xFF;
    uint8_t n;
    do {
        n = rc522_read_reg(DivIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x04)); // CRCIRq

    result[0] = rc522_read_reg(CRCResultRegL);
    result[1] = rc522_read_reg(CRCResultRegM);

    return Status_OK;
}

// otentikasi dan read block
esp_err_t rc522_auth(uint8_t auth_mode, uint8_t block_addr, uint8_t *key, uint8_t *uid) {
    uint8_t buff[12];

    buff[0] = auth_mode;
    buff[1] = block_addr;
    for (uint8_t i = 0; i < 6; i++) buff[i + 2] = key[i];
    for (uint8_t i = 0; i < 4; i++) buff[i + 8] = uid[i];

    rc522_write_reg(BitFramingReg, 0x00);

    for (uint8_t i = 0; i < 12; i++) {
        rc522_write_reg(FIFODataReg, buff[i]);
    }

    rc522_write_reg(CommandReg, PCD_AUTHENT);

    uint8_t i = 200;
    uint8_t n;
    do {
        n = rc522_read_reg(Status2Reg);
        i--;
    } while ((i != 0) && !(n & 0x08)); // MFCrypto1On

    uint8_t error = rc522_read_reg(ErrorReg);
    
    /*
    n = rc522_read_reg(Status2Reg);
    if (n & 0x08) return Status_OK;

    return Status_ERROR;*/

    // Cek keberhasilan autentikasi
    if ((error & 0x1B) == 0) {
        printf("Auth OK\n");
        return Status_OK;
    } else {
        printf("Auth Gagal\n");
        return Status_ERROR;
    }

}

esp_err_t rc522_read_block(uint8_t block_addr, uint8_t *recv_data) {
    uint8_t buff[4];

    buff[0] = PICC_READ;
    buff[1] = block_addr;
    rc522_calculate_crc(buff, 2, &buff[2]);

    rc522_write_reg(BitFramingReg, 0x00);

    for (uint8_t i = 0; i < 4; i++) {
        rc522_write_reg(FIFODataReg, buff[i]);
    }
    rc522_write_reg(CommandReg, PCD_TRANSCEIVE);
    rc522_write_reg(BitFramingReg, 0x87);

    uint8_t i = 200;
    uint8_t n;
    do {
        n = rc522_read_reg(CommIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x30));

    uint8_t error = rc522_read_reg(ErrorReg);
    if (error & 0x1B) return Status_ERROR;

    uint8_t fifo_level = rc522_read_reg(FIFOLevelReg);
    for (uint8_t i = 0; i < fifo_level; i++) {
        recv_data[i] = rc522_read_reg(FIFODataReg);
    }

    return Status_OK;
}

esp_err_t rc522_select(uint8_t *uid) {
    uint8_t buffer[9];
    buffer[0] = PICC_SELECTTAG;
    buffer[1] = 0x70; // NVB = 0x70 = 7 UID bytes
    for (int i = 0; i < 5; i++) buffer[2 + i] = uid[i];
    rc522_calculate_crc(buffer, 7, &buffer[7]);

    rc522_write_reg(BitFramingReg, 0x00);

    for (uint8_t i = 0; i < 9; i++) {
        rc522_write_reg(FIFODataReg, buffer[i]);
    }

    rc522_write_reg(CommandReg, PCD_TRANSCEIVE);
    rc522_write_reg(BitFramingReg, 0x87);

    uint8_t i = 200;
    uint8_t n;
    do {
        n = rc522_read_reg(CommIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x30));

    uint8_t error = rc522_read_reg(ErrorReg);
    if (error & 0x1B) return Status_ERROR;

    return Status_OK;
}

// brute force
bool brute_force_key_finder(uint8_t *uid, uint8_t *found_key) {
    // printf("Mulai brute force key...\n"); // line for debug

    // Daftar key umum untuk Mifare Classic 1K
    uint8_t known_keys[][6] = {
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // default key
        {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}, // contoh key factory
        {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}, // transport key
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // kosong
        {0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD}, // sample
        {0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A}, // sample
        {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}, // sample
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}, // sample
    };

    uint8_t block_to_auth = 4; // Gunakan block data pertama

    for (int i = 0; i < sizeof(known_keys)/6; i++) {
        /*
        printf("Mencoba key bro: "); //debug
        for (int k = 0; k < 6; k++) {
            printf("%02X ", known_keys[i][k]); // menampilkan keys
        }        
        printf("\n");
        */

        esp_err_t auth_result = rc522_auth(PICC_AUTHENT1A, block_to_auth, known_keys[i], uid);
        if (auth_result == Status_OK) {
            // printf("Key ditemukan!\n");
            memcpy(found_key, known_keys[i], 6);
            return true;
        }
    }
    printf("Brute force key gagal, tidak menemukan key.\n");
    return false;
}