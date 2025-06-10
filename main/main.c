#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "driver/gpio.h"
#include "cJSON.h"

#include "test_azure_sdk.h"

#include "configuration.h"
#include "driver/uart.h"
#include "emmc.h"
#include "uart.h"
#include "BLE.h"
#include "Eth_WiFI_Mananger.h"                  // Both Wifi And Ethernet
#include "esp_event.h"
#include "esp_netif.h"
#include "wolfTPM/tpm_io_espressif.h"
#include "data_structure.h"
#define VERSION " version : 3.1.0"

static const char *TAG =          "ESP_Hydroconnect";

#define LED_PIN 8



static const int RX_BUF_SIZE = 1024*4;

void download_json_table(void)
{
	download_file_to_emmc(SYSTEM_table_URL,MOUNT_POINT"/SYSTEM_table.json");
	download_file_to_emmc(setup_table_URL, MOUNT_POINT"/setup.json");
	download_file_to_emmc(realtime_table_URL, MOUNT_POINT"/realtime.json");
	download_file_to_emmc(record_able_URL, MOUNT_POINT"/record.json");
	download_file_to_emmc(activitylog_table_URL, MOUNT_POINT"/activitylog.json");
	download_file_to_emmc(firmware_table_URL,MOUNT_POINT"/firmware.json");
	download_file_to_emmc(timezone_table_URL, MOUNT_POINT"/timezone.json");
	download_file_to_emmc(xdd_hyc_setup_table_URL,MOUNT_POINT"/xdd_hyc_setup.json");
	download_file_to_emmc(xdd_tec_setup_table_URL, MOUNT_POINT"/xdd_tecc_setup.json");
}

static void rx_task(void *arg)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if ( rxBytes> 0) {
            data[rxBytes] = 0;
            ESP_LOGI(TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(TAG, data, rxBytes, ESP_LOG_INFO);
        }
        // Yield to avoid watchdog reset
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    free(data);
}

void creat_file(void)
{
   const char *file_path = MOUNT_POINT"/test_file.txt";
    
   FILE *f;
   f = fopen(file_path, "w");
   if (f == NULL) {
	perror("fopen");
    ESP_LOGE(TAG, "Failed to open file for writing");
    return;
	}
	fprintf(f, "Hello, this is a test write to eMMC card file no 2!\n");
	fclose(f);
	ESP_LOGI(TAG, "File written successfully: %s", file_path);
	FILE *file = fopen(file_path, "r");
	if (file_path == NULL) {
	    ESP_LOGE(TAG, "Failed to open file for reading");
	    return;
	}
	fseek(file, 0, SEEK_END);
	long len = ftell(file);
	rewind(file);
	
	char *buffer = malloc(len + 1);
	fread(buffer, 1, len, file);
	buffer[len] = '\0';
	fclose(file);
	
	printf("Data:\n%s\n", buffer);
	free(buffer);
}	

void app_main(void)
{
	ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    check_and_handle_rollback();
    /*Ble_init();
    wifi_init();
  	ethernet_init(); */
    gpio_reset_pin(LED_PIN);
  	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
  	gpio_set_level(LED_PIN, false);
  	//vTaskDelay(2000);
  	tpm_init();
  	//tpm2_createprimary();
  	tpm2_read_persistent_key(TPM_PERSISTENT_HANDLE);
    //emmc_init(); 
    uart1_init();
    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    /***************************** Initilisation Done **********************************/ 
/* //Application Start testing code */
	  
	// download_json_table();              //Function to Download Json format table 
	// Crea_csv_table();                    //Function to create the CSV file 
}
