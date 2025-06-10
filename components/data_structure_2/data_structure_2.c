#include "data_structure_2.h"
#include <complex.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "driver/uart.h"
#include "uart.h"
#include "esp_log.h"

static const char *TAG = "data_structure_2";

#define BUF_SIZE 1024
uint8_t data[BUF_SIZE] = {0};

uint8_t calculate_checksum(const uint8_t *data, size_t length) {
    if (length == 0) return 0;

    uint8_t checksum = 0;
    
    // XOR all bytes except the last one (which is the received checksum)
    for (size_t i = 0; i < length - 1; ++i) {
        checksum ^= data[i];
    }

    return checksum;
}
uint8_t* getData() {
    static uint8_t data[257];

    // Fill with example data (you can customize this)
    for (int i = 0; i < 257; i++) {
        data[i] = i & 0xFF;  // wraps values between 0 to 255
    }

    return data;
}
uint8_t get_dynamic_value() {
    // Example dynamic function — replace with your logic
    return 0x09;
}
void WRITE_C2D_TIMESTAMP(const char *json_filepath) {
    FILE *file = fopen(json_filepath, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", json_filepath);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long json_size = ftell(file);
    rewind(file);

    // Allocate memory for JSON data
    uint8_t *json_data = malloc(json_size);
    if (!json_data) {
        ESP_LOGE(TAG, "Memory allocation failed");
        fclose(file);
        return;
    }

    // Read file content into json_data
    fread(json_data, 1, json_size, file);
    fclose(file);

    // Each byte becomes 2 hex characters
    size_t hex_data_len = json_size * 2;
    char *hex_string = malloc(hex_data_len + 1);
    if (!hex_string) {
        ESP_LOGE(TAG, "Memory allocation for hex string failed");
        free(json_data);
        return;
    }

    // Convert to hex string (e.g., 0x7B → "7B")
    for (size_t i = 0; i < json_size; i++) {
        sprintf(&hex_string[i * 2], "%02X", json_data[i]);
    }

    // UART message: [Header 3B] + [Length 2B] + [Hex String N B] + [Checksum 1B]
    size_t total_size = 5 + hex_data_len + 1;
    uint8_t *message = malloc(total_size);
    if (!message) {
        ESP_LOGE(TAG, "Memory allocation for message failed");
        free(json_data);
        free(hex_string);
        return;
    }

    // Fill header
    message[0] = 0x48;
    message[1] = 0x53;
    message[2] = 0x47;  // Command: 

    // Fill length (hex string length) - 2 bytes (high, low)
    message[3] = (hex_data_len >> 8) & 0xFF; // High byte
    message[4] = hex_data_len & 0xFF;        // Low byte

    // Copy hex string as ASCII chars (not actual bytes)
    memcpy(&message[5], hex_string, hex_data_len);

    // Calculate checksum (sum of all bytes before it)
    uint8_t checksum = 0;
    for (size_t i = 0; i < 5 + hex_data_len; i++) {
        checksum += message[i];
    }

    message[5 + hex_data_len] = checksum;

    // Send message
    uart_write_bytes(UART_NUM_1, (const char *)message, total_size);
    ESP_LOGI(TAG, "Hex-encoded JSON sent over UART");
    // Cleanup
    free(json_data);
    free(hex_string);
    free(message);
    
    int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }
    uint8_t Header1 = data[0];
    uint8_t Header2 = data[1];
    uint8_t washer_id = data[2];
    uint8_t command_code = data[3];
    uint8_t Message_high_byte = data[4];
    uint8_t Message_low_byte = data[5];
    uint8_t Response = data[6];
    uint8_t checksum_byte = data[7];  
    printf("Header1: %d\n", Header1);
    printf("Header2: %d\n", Header2);
    printf("Washer ID: %d\n", washer_id);
    printf("Command: %d\n", command_code);
    printf("Message_high_byte: %d\n", Message_high_byte);
    printf("Message_low_byte: %d\n", Message_low_byte);
    printf("Message Length: %d\n", len);
    printf("Response : %d\n", Response);
    printf("checksum_byte : %d\n", checksum_byte);
}

void WRITE_C2D_MACHINE_NAME(const char *json_filepath) {
    FILE *file = fopen(json_filepath, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", json_filepath);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long json_size = ftell(file);
    rewind(file);

    // Allocate memory for JSON data
    uint8_t *json_data = malloc(json_size);
    if (!json_data) {
        ESP_LOGE(TAG, "Memory allocation failed");
        fclose(file);
        return;
    }

    // Read file content into json_data
    fread(json_data, 1, json_size, file);
    fclose(file);

    // Each byte becomes 2 hex characters
    size_t hex_data_len = json_size * 2;
    char *hex_string = malloc(hex_data_len + 1);
    if (!hex_string) {
        ESP_LOGE(TAG, "Memory allocation for hex string failed");
        free(json_data);
        return;
    }

    // Convert to hex string (e.g., 0x7B → "7B")
    for (size_t i = 0; i < json_size; i++) {
        sprintf(&hex_string[i * 2], "%02X", json_data[i]);
    }

    // UART message: [Header 3B] + [Length 2B] + [Hex String N B] + [Checksum 1B]
    size_t total_size = 5 + hex_data_len + 1;
    uint8_t *message = malloc(total_size);
    if (!message) {
        ESP_LOGE(TAG, "Memory allocation for message failed");
        free(json_data);
        free(hex_string);
        return;
    }

    // Fill header
    message[0] = 0x48;
    message[1] = 0x53;
    message[2] = 0x48;  // Command: 

    // Fill length (hex string length) - 2 bytes (high, low)
    message[3] = (hex_data_len >> 8) & 0xFF; // High byte
    message[4] = hex_data_len & 0xFF;        // Low byte

    // Copy hex string as ASCII chars (not actual bytes)
    memcpy(&message[5], hex_string, hex_data_len);

    // Calculate checksum (sum of all bytes before it)
    uint8_t checksum = 0;
    for (size_t i = 0; i < 5 + hex_data_len; i++) {
        checksum += message[i];
    }

    message[5 + hex_data_len] = checksum;

    // Send message
    uart_write_bytes(UART_NUM_1, (const char *)message, total_size);
    ESP_LOGI(TAG, "Hex-encoded JSON sent over UART");
    // Cleanup
    free(json_data);
    free(hex_string);
    free(message);
    
    int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }
      
    printf("Header1: %d\n", data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("Message Length: %d\n", len);
    printf("Response : %d\n", data[6]);
    printf("checksum_byte : %d\n", data[7]);
}

void WRITE_C2D_COMPANY_NAME(const char *json_filepath) {
    FILE *file = fopen(json_filepath, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", json_filepath);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long json_size = ftell(file);
    rewind(file);

    // Allocate memory for JSON data
    uint8_t *json_data = malloc(json_size);
    if (!json_data) {
        ESP_LOGE(TAG, "Memory allocation failed");
        fclose(file);
        return;
    }

    // Read file content into json_data
    fread(json_data, 1, json_size, file);
    fclose(file);

    // Each byte becomes 2 hex characters
    size_t hex_data_len = json_size * 2;
    char *hex_string = malloc(hex_data_len + 1);
    if (!hex_string) {
        ESP_LOGE(TAG, "Memory allocation for hex string failed");
        free(json_data);
        return;
    }

    // Convert to hex string (e.g., 0x7B → "7B")
    for (size_t i = 0; i < json_size; i++) {
        sprintf(&hex_string[i * 2], "%02X", json_data[i]);
    }

    // UART message: [Header 3B] + [Length 2B] + [Hex String N B] + [Checksum 1B]
    size_t total_size = 5 + hex_data_len + 1;
    uint8_t *message = malloc(total_size);
    if (!message) {
        ESP_LOGE(TAG, "Memory allocation for message failed");
        free(json_data);
        free(hex_string);
        return;
    }

    // Fill header
    message[0] = 0x48;
    message[1] = 0x53;
    message[2] = 0x49;  // Command: 

    // Fill length (hex string length) - 2 bytes (high, low)
    message[3] = (hex_data_len >> 8) & 0xFF; // High byte
    message[4] = hex_data_len & 0xFF;        // Low byte

    // Copy hex string as ASCII chars (not actual bytes)
    memcpy(&message[5], hex_string, hex_data_len);

    // Calculate checksum (sum of all bytes before it)
    uint8_t checksum = 0;
    for (size_t i = 0; i < 5 + hex_data_len; i++) {
        checksum += message[i];
    }

    message[5 + hex_data_len] = checksum;

    // Send message
    uart_write_bytes(UART_NUM_1, (const char *)message, total_size);
    ESP_LOGI(TAG, "Hex-encoded JSON sent over UART");
    // Cleanup
    free(json_data);
    free(hex_string);
    free(message);
    
    int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }
      
    printf("Header1: %d\n", data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("Message Length: %d\n", len);
    printf("Response : %d\n", data[6]);
    printf("checksum_byte : %d\n", data[7]);
}
void WRITE_C2D_ACCOUNT_NAME(const char *json_filepath) {
    FILE *file = fopen(json_filepath, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", json_filepath);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long json_size = ftell(file);
    rewind(file);

    // Allocate memory for JSON data
    uint8_t *json_data = malloc(json_size);
    if (!json_data) {
        ESP_LOGE(TAG, "Memory allocation failed");
        fclose(file);
        return;
    }

    // Read file content into json_data
    fread(json_data, 1, json_size, file);
    fclose(file);

    // Each byte becomes 2 hex characters
    size_t hex_data_len = json_size * 2;
    char *hex_string = malloc(hex_data_len + 1);
    if (!hex_string) {
        ESP_LOGE(TAG, "Memory allocation for hex string failed");
        free(json_data);
        return;
    }

    // Convert to hex string (e.g., 0x7B → "7B")
    for (size_t i = 0; i < json_size; i++) {
        sprintf(&hex_string[i * 2], "%02X", json_data[i]);
    }

    // UART message: [Header 3B] + [Length 2B] + [Hex String N B] + [Checksum 1B]
    size_t total_size = 5 + hex_data_len + 1;
    uint8_t *message = malloc(total_size);
    if (!message) {
        ESP_LOGE(TAG, "Memory allocation for message failed");
        free(json_data);
        free(hex_string);
        return;
    }

    // Fill header
    message[0] = 0x48;
    message[1] = 0x53;
    message[2] = 0x4A;  // Command: 

    // Fill length (hex string length) - 2 bytes (high, low)
    message[3] = (hex_data_len >> 8) & 0xFF; // High byte
    message[4] = hex_data_len & 0xFF;        // Low byte

    // Copy hex string as ASCII chars (not actual bytes)
    memcpy(&message[5], hex_string, hex_data_len);

    // Calculate checksum (sum of all bytes before it)
    uint8_t checksum = 0;
    for (size_t i = 0; i < 5 + hex_data_len; i++) {
        checksum += message[i];
    }

    message[5 + hex_data_len] = checksum;

    // Send message
    uart_write_bytes(UART_NUM_1, (const char *)message, total_size);
    ESP_LOGI(TAG, "Hex-encoded JSON sent over UART");
    // Cleanup
    free(json_data);
    free(hex_string);
    free(message);
    
    int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }
      
    printf("Header1: %d\n", data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("Message Length: %d\n", len);
    printf("Response : %d\n", data[6]);
    printf("checksum_byte : %d\n", data[7]);
}
void WRITE_C2D_INSTALLERS_PASSWORD(const char *json_filepath) {
    FILE *file = fopen(json_filepath, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", json_filepath);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long json_size = ftell(file);
    rewind(file);

    // Allocate memory for JSON data
    uint8_t *json_data = malloc(json_size);
    if (!json_data) {
        ESP_LOGE(TAG, "Memory allocation failed");
        fclose(file);
        return;
    }

    // Read file content into json_data
    fread(json_data, 1, json_size, file);
    fclose(file);

    // Each byte becomes 2 hex characters
    size_t hex_data_len = json_size * 2;
    char *hex_string = malloc(hex_data_len + 1);
    if (!hex_string) {
        ESP_LOGE(TAG, "Memory allocation for hex string failed");
        free(json_data);
        return;
    }

    // Convert to hex string (e.g., 0x7B → "7B")
    for (size_t i = 0; i < json_size; i++) {
        sprintf(&hex_string[i * 2], "%02X", json_data[i]);
    }

    // UART message: [Header 3B] + [Length 2B] + [Hex String N B] + [Checksum 1B]
    size_t total_size = 5 + hex_data_len + 1;
    uint8_t *message = malloc(total_size);
    if (!message) {
        ESP_LOGE(TAG, "Memory allocation for message failed");
        free(json_data);
        free(hex_string);
        return;
    }

    // Fill header
    message[0] = 0x48;
    message[1] = 0x53;
    message[2] = 0x4B;  // Command: 

    // Fill length (hex string length) - 2 bytes (high, low)
    message[3] = (hex_data_len >> 8) & 0xFF; // High byte
    message[4] = hex_data_len & 0xFF;        // Low byte

    // Copy hex string as ASCII chars (not actual bytes)
    memcpy(&message[5], hex_string, hex_data_len);

    // Calculate checksum (sum of all bytes before it)
    uint8_t checksum = 0;
    for (size_t i = 0; i < 5 + hex_data_len; i++) {
        checksum += message[i];
    }

    message[5 + hex_data_len] = checksum;

    // Send message
    uart_write_bytes(UART_NUM_1, (const char *)message, total_size);
    ESP_LOGI(TAG, "Hex-encoded JSON sent over UART");
    // Cleanup
    free(json_data);
    free(hex_string);
    free(message);
    
    int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }
      
    printf("Header1: %d\n", data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("Message Length: %d\n", len);
    printf("Response : %d\n", data[6]);
    printf("checksum_byte : %d\n", data[7]);
}
//WRITE_C2D_TUBE_ALARM
void WRITE_C2D_TUBE_ALARM(const char *json_filepath) {
    FILE *file = fopen(json_filepath, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", json_filepath);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long json_size = ftell(file);
    rewind(file);

    // Allocate memory for JSON data
    uint8_t *json_data = malloc(json_size);
    if (!json_data) {
        ESP_LOGE(TAG, "Memory allocation failed");
        fclose(file);
        return;
    }

    // Read file content into json_data
    fread(json_data, 1, json_size, file);
    fclose(file);

    // Each byte becomes 2 hex characters
    size_t hex_data_len = json_size * 2;
    char *hex_string = malloc(hex_data_len + 1);
    if (!hex_string) {
        ESP_LOGE(TAG, "Memory allocation for hex string failed");
        free(json_data);
        return;
    }

    // Convert to hex string (e.g., 0x7B → "7B")
    for (size_t i = 0; i < json_size; i++) {
        sprintf(&hex_string[i * 2], "%02X", json_data[i]);
    }

    // UART message: [Header 3B] + [Length 2B] + [Hex String N B] + [Checksum 1B]
    size_t total_size = 5 + hex_data_len + 1;
    uint8_t *message = malloc(total_size);
    if (!message) {
        ESP_LOGE(TAG, "Memory allocation for message failed");
        free(json_data);
        free(hex_string);
        return;
    }

    // Fill header
    message[0] = 0x48;
    message[1] = 0x53;
    message[2] = 0x4C;  // Command: 

    // Fill length (hex string length) - 2 bytes (high, low)
    message[3] = (hex_data_len >> 8) & 0xFF; // High byte
    message[4] = hex_data_len & 0xFF;        // Low byte

    // Copy hex string as ASCII chars (not actual bytes)
    memcpy(&message[5], hex_string, hex_data_len);

    // Calculate checksum (sum of all bytes before it)
    uint8_t checksum = 0;
    for (size_t i = 0; i < 5 + hex_data_len; i++) {
        checksum += message[i];
    }

    message[5 + hex_data_len] = checksum;

    // Send message
    uart_write_bytes(UART_NUM_1, (const char *)message, total_size);
    ESP_LOGI(TAG, "Hex-encoded JSON sent over UART");
    // Cleanup
    free(json_data);
    free(hex_string);
    free(message);
    
    int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }      
    printf("Header1: %d\n", data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("Message Length: %d\n", len);
    printf("Response : %d\n", data[6]);
    printf("checksum_byte : %d\n", data[7]);
}

void WRITE_C2D_COMPLETE(void)
{
	uint8_t WRITE_C2D_COMPLETE[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x4D, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    WRITE_C2D_COMPLETE[7] = calculate_checksum(WRITE_C2D_COMPLETE, sizeof(WRITE_C2D_COMPLETE));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, WRITE_C2D_COMPLETE, sizeof(WRITE_C2D_COMPLETE));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}

void READ_C2D_SETUP_TRANSFERRING_STATUS(void)
{
	uint8_t READ_C2D_SETUP_TRANSFERRING_STATUS[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x4E, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_C2D_SETUP_TRANSFERRING_STATUS[7] = calculate_checksum(READ_C2D_SETUP_TRANSFERRING_STATUS, sizeof(READ_C2D_SETUP_TRANSFERRING_STATUS));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_C2D_SETUP_TRANSFERRING_STATUS, sizeof(READ_C2D_SETUP_TRANSFERRING_STATUS));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}
void WRITE_PAGE_TO_RAM(void)
{
	uint8_t WRITE_PAGE_TO_RAM[265] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x5E, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	uint8_t* data257 = getData();  // pointer to 257-byte data
		// Insert the 257 bytes into WRITE_UPLOAD_SECTOR_NUMBER starting at index 6
	for (int i = 0; i < 257; i++) {
	    WRITE_PAGE_TO_RAM[6 + i] = data257[i];
	}
	// ✅ Calculate checksum and assign to Byte 6
    WRITE_PAGE_TO_RAM[264] = calculate_checksum(WRITE_PAGE_TO_RAM, sizeof(WRITE_PAGE_TO_RAM));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, WRITE_PAGE_TO_RAM, sizeof(WRITE_PAGE_TO_RAM));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}
void WRITE_UPLOAD_SECTOR_NUMBER(void)
{
	uint8_t WRITE_UPLOAD_SECTOR_NUMBER[9] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x5F, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00,  // [7] data to be
    0x00  // [8] Checksum (to be calculated)
}; 
	WRITE_UPLOAD_SECTOR_NUMBER[6]=get_dynamic_value();
	WRITE_UPLOAD_SECTOR_NUMBER[7]=get_dynamic_value();
	// ✅ Calculate checksum and assign to Byte 6
    WRITE_UPLOAD_SECTOR_NUMBER[8] = calculate_checksum(WRITE_UPLOAD_SECTOR_NUMBER, sizeof(WRITE_UPLOAD_SECTOR_NUMBER));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, WRITE_C2D_COMPLETE, sizeof(WRITE_C2D_COMPLETE));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}

void WRITE_UPLOAD_SECTOR_TO_FLASH(void)
{
	uint8_t WRITE_UPLOAD_SECTOR_TO_FLASH[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x60, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    WRITE_UPLOAD_SECTOR_TO_FLASH[7] = calculate_checksum(WRITE_UPLOAD_SECTOR_TO_FLASH, sizeof(WRITE_UPLOAD_SECTOR_TO_FLASH));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, WRITE_C2D_COMPLETE, sizeof(WRITE_C2D_COMPLETE));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}
#include <stdio.h>
#include <stdint.h>

uint32_t get_file_length(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", path);
        return 0;
    }
    fseek(file, 0, SEEK_END);
    uint32_t length = ftell(file);
    fclose(file);
    return length;
}

void COMPLETE_TRANSFER_STEP1(const char* firmware_path) {
    uint8_t COMPLETE_TRANSFER_STEP1[12] = {
        0x48, // [0] Header1
        0x53, // [1] Header2
        0x01, // [2] Washer ID
        0x63, // [3] Command
        0x00, // [4] Message Length High Byte
        0x01, // [5] Message Length Low Byte
        0x00, // [6] data (firmware length byte 0)
        0x00, // [7] data (firmware length byte 1)
        0x00, // [8] data (firmware length byte 2)
        0x00, // [9] data (firmware length byte 3)
        0x00, // [10] Reserved
        0x00  // [11] Checksum (to be calculated)
    };

    // 🔄 Read file length
    uint32_t firmware_length = get_file_length(firmware_path);

    // 🔢 Insert file length in little-endian format
    COMPLETE_TRANSFER_STEP1[6] = (firmware_length >> 0) & 0xFF;
    COMPLETE_TRANSFER_STEP1[7] = (firmware_length >> 8) & 0xFF;
    COMPLETE_TRANSFER_STEP1[8] = (firmware_length >> 16) & 0xFF;
    COMPLETE_TRANSFER_STEP1[9] = (firmware_length >> 24) & 0xFF;

    // ✅ Calculate checksum
    COMPLETE_TRANSFER_STEP1[11] = calculate_checksum(COMPLETE_TRANSFER_STEP1, sizeof(COMPLETE_TRANSFER_STEP1));

    // 🚀 Send command 
    uart_write_bytes(UART_NUM_1, COMPLETE_TRANSFER_STEP1, sizeof(COMPLETE_TRANSFER_STEP1));

    // 📥 Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));
    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}

void COMPLETE_TRANSFER_FINAL_STEP(void)
{
	uint8_t COMPLETE_TRANSFER_FINAL_STEP[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x7D, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    COMPLETE_TRANSFER_FINAL_STEP[7] = calculate_checksum(COMPLETE_TRANSFER_FINAL_STEP, sizeof(COMPLETE_TRANSFER_FINAL_STEP));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, COMPLETE_TRANSFER_FINAL_STEP, sizeof(COMPLETE_TRANSFER_FINAL_STEP));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}

void READ_C2D_FIRMWARE_TRANSFERRING_STATUS(void)
{
	uint8_t READ_C2D_FIRMWARE_TRANSFERRING_STATUS[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x80, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_C2D_FIRMWARE_TRANSFERRING_STATUS[7] = calculate_checksum(READ_C2D_FIRMWARE_TRANSFERRING_STATUS, sizeof(READ_C2D_FIRMWARE_TRANSFERRING_STATUS));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_C2D_FIRMWARE_TRANSFERRING_STATUS, sizeof(READ_C2D_FIRMWARE_TRANSFERRING_STATUS));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}
void WRITE_C2D_FW_UPLOAD_TOTAL_SECTORS(void)
{
	uint8_t WRITE_C2D_FW_UPLOAD_TOTAL_SECTORS[9] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x81, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00,  // [7] data to be 
    0x00  // [8] Checksum (to be calculated)
}; 
	WRITE_C2D_FW_UPLOAD_TOTAL_SECTORS[6]=get_dynamic_value();
	WRITE_C2D_FW_UPLOAD_TOTAL_SECTORS[7]=get_dynamic_value();
	// ✅ Calculate checksum and assign to Byte 6
    WRITE_C2D_FW_UPLOAD_TOTAL_SECTORS[8] = calculate_checksum(WRITE_C2D_FW_UPLOAD_TOTAL_SECTORS, sizeof(WRITE_C2D_FW_UPLOAD_TOTAL_SECTORS));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, COMPLETE_TRANSFER_FINAL_STEP, sizeof(COMPLETE_TRANSFER_FINAL_STEP));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}

void COMPLETE_TRANSFER_STEP2(void)
{
	uint8_t COMPLETE_TRANSFER_STEP2[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x84, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    COMPLETE_TRANSFER_STEP2[7] = calculate_checksum(COMPLETE_TRANSFER_STEP2, sizeof(COMPLETE_TRANSFER_STEP2));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, COMPLETE_TRANSFER_STEP2, sizeof(COMPLETE_TRANSFER_STEP2));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}

void WATCHDOG_REBOOT(void)
{
	uint8_t WATCHDOG_REBOOT[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x85, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    WATCHDOG_REBOOT[7] = calculate_checksum(WATCHDOG_REBOOT, sizeof(WATCHDOG_REBOOT));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, WATCHDOG_REBOOT, sizeof(WATCHDOG_REBOOT));

    // Read response
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
        return;
    }

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
}