#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rc522.h"
#include "string.h"
#include "esp_log.h"
#include "mifare_read_task.h"

typedef enum {
    RFID_STATE_IDLE,
    RFID_STATE_DETECTED,
    RFID_STATE_DONE
} rfid_state_t;

#define TAG "rfid_task"


void mifare_read_task(void *pvParameters){
    rfid_state_t state = RFID_STATE_IDLE;
    bool card_present = false;
    
    uint8_t uid[4];
    uint8_t atqa[2];
    uint8_t found_key[6];
    uint8_t block_data[16];
    const uint8_t block_to_read = 4; // contoh block

    while (1){
        switch (state){
            case RFID_STATE_IDLE:
                if (rc522_request(atqa)){
                    if (!card_present){
                        card_present = true;
                        ESP_LOGI(TAG, "Kartu Terdeteksi!");
                        state = RFID_STATE_DETECTED;
                    }                     
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                break;
        
            case RFID_STATE_DETECTED:
                if (rc522_anticoll(uid)){
                    ESP_LOGI(TAG, "UID: %02X %02X %02X %02X", uid[0], uid[1], uid[2], uid[3]);
                    if (rc522_select(uid) == Status_OK){
                        if (brute_force_key_finder(uid, found_key)){
                            ESP_LOGI(TAG, "Brute force key found: %02X %02X %02X %02X %02X %02X", found_key[0], found_key[1], found_key[2],
                            found_key[3], found_key[4], found_key[5]);
                            ESP_LOGI(TAG, "Attempting auth for block %d with found key...", block_to_read);
                            rc522_select(uid);

                            if (rc522_auth(PICC_AUTHENT1A, block_to_read, found_key, uid) == Status_OK){
                                if (rc522_read_block(block_to_read, block_data) == Status_OK){
                                    ESP_LOG_BUFFER_HEX(TAG, block_data, 16);
                                } else {
                                    ESP_LOGI(TAG, "Gagal membaca block %d", block_to_read);
                                }
                            } else {
                                ESP_LOGI(TAG, "Auth gagal untuk block %d", block_to_read);
                            }                                                    
                        } else {
                            ESP_LOGI(TAG, "Key tidak ditemukan.");
                        }                                                
                    } else {
                        ESP_LOGI(TAG, "Select UID gagal.");
                    }
                    state = RFID_STATE_DONE;
                } else {
                    ESP_LOGW(TAG, "Gagal membaca UID.");
                    state = RFID_STATE_IDLE;
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                break;
            
            case RFID_STATE_DONE:
            if (!rc522_request(atqa)){
                ESP_LOGI(TAG, "Kartu dilepas, kembali idle.");
                card_present = false;
                state = RFID_STATE_IDLE;
            }
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
    }    
}

