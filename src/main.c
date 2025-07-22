/*
alur baca rfid:
    1. rfid state idle:
        - rc_request -> cek ada kartu
        - jika ada lanjut ke state detected
    2. rfid state detected:
        - rc_anticoll : baca uid
        - rc_select
        - brute_force_finder -> dapat key
        - rc_auth -> untuk block tertentu
        - rc_read_block -> baca data block
        - tampilkan data
        - lanjut rfid state done
    3. rfid state done
        - cek kartu dilepas
        - jika dilepas, kembali ke rfid state idle
*/

#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rc522.h"
#include "esp_log.h"
#include "mifare_read_task.h"

void app_main(void){

    if (rc522_init(VSPI_HOST) == ESP_OK){
        ESP_LOGI("MAIN", "Starting RFID reader task...");
        rc522_antenna_on();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    xTaskCreate(mifare_read_task, "rfid read task", 4096, NULL, 5, NULL);
}