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



void app_main() {}