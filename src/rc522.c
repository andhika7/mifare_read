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