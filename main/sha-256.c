#include <stdio.h>


#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <string.h>
#include <mbedtls/sha256.h>

static const char *TAG = "WEBSOCKET";


#define MOUNT_POINT "/sdcard"



#define SPI_DMA_CHAN    1


static sdmmc_card_t *mount_card = NULL;
static char *mount_base_path = MOUNT_POINT;


#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13


esp_err_t start_file_server(const char *base_path);

void sdcard_mount(void) {
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = true,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");


    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
            .mosi_io_num = PIN_NUM_MOSI,
            .miso_io_num = PIN_NUM_MISO,
            .sclk_io_num = PIN_NUM_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        ESP_ERROR_CHECK(ret);
    }


    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    mount_card = card;

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        ESP_ERROR_CHECK(ret);
    }
    sdmmc_card_print_info(stdout, card);

}












#define SHA256_MAX_LEN 32

void sha256_sum(char * file_path,char * sha_result){
    FILE *f = fopen(file_path, "rb");
    if(f==NULL){
        ESP_LOGE("sdcard","file not found");
        return;
    }
    char buf[64];
    mbedtls_sha256_context ctx;
    unsigned char digest[SHA256_MAX_LEN];

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx,0);

    size_t read;

    do {
        read = fread((void*) buf, 1, sizeof(buf), f);
        if(read==0){
            break;
        }
        mbedtls_sha256_update_ret(&ctx, (unsigned const char*) buf, read);
    } while(1);

    fclose(f);

    mbedtls_sha256_finish_ret(&ctx, digest);


    char digest_str[SHA256_MAX_LEN * 2+1];

    for (int i = 0; i < SHA256_MAX_LEN; i++) {
        sprintf(&digest_str[i * 2], "%02x", (unsigned int)digest[i]);
    }
    digest_str[SHA256_MAX_LEN * 2]='\0';

    ESP_LOGI("sha256","%s",digest_str);
    if(sha_result){
        strcpy(sha_result, digest_str);
    }

}

void app_main(void)
{
    sdcard_mount();

    sha256_sum("/sdcard/bd1fbab7-2c81-43e3-a25d-75637048870f.bin",NULL);
}
