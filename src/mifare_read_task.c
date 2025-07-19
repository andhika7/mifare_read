#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rc522.h"
#include "string.h"

typedef enum {
    RFID_STATE_IDLE,
    RFID_STATE_DETECTED,
    RFID_STATE_DONE
} rfid_state_t;

static const char *TAG = "rfid";

void mifare_read_task(void *pvParameters){
    rfid_state_t state = RFID_STATE_IDLE;
    uint8_t uid[4];
    uint8_t atqa[2];
    uint8_t found_key[6];
    uint8_t block_data[16];
    const uint8_t block_to_read = 4; // contoh block

    while (1){
        switch (state){
            case RFID_STATE_IDLE:
                if (rc522_request(atqa)){
                    ESP_LOGI(TAG, "Kartu Terdeteksi!");
                    state = RFID_STATE_DETECTED;
                }
                break;
        
            case RFID_STATE_DETECTED:
                if (rc522_anticoll(uid)){
                    ESP_LOGI(TAG, "UID: %02X %02X %02X %02X", uid[0], uid[1], uid[2], uid[3]);
                    if (rc522_select(uid) == Status_OK){
                        if (brute_force_key_finder(uid, found_key)){
                            if (rc522_auth(PICC_AUTHENT1A, block_to_read, found_key, uid) == Status_OK){
                                if (rc522_read_block(block_to_read, block_data) == Status_OK){
                                    ESP_LOG_BUFFER_HEX(TAG, block_data, 16);
                                } else {
                                    ESP_LOGW(TAG, "Gagal membaca block %d", block_to_read);
                                }
                            } else {
                                ESP_LOGW(TAG, "Auth gagal untuk block %d", block_to_read);
                            }                                                    
                        } else {
                            ESP_LOGW(TAG, "Key tidak ditemukan.");
                        }                                                
                    } else {
                        ESP_LOGW(TAG, "Select UID gagal.");
                    }
                    state = RFID_STATE_DONE;
                }
                break;
            
            case RFID_STATE_DONE:
            if (!rc522_request(atqa)){
                ESP_LOGI(TAG, "Kartu dilepas, kembali idle.");
                state = RFID_STATE_IDLE;
            }
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
    }    
}

