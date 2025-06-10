#include <complex.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "data_structure.h"
#include "driver/uart.h"
#include "uart.h"
#include "esp_log.h"

static const char *TAG = "data_structure";

#define BUF_SIZE 1024
uint8_t data[BUF_SIZE] = {0};

uint8_t get_dynamic_value() {
    // Example dynamic function — replace with your logic
    return 0x09;
}

uint8_t calculate_checksum(const uint8_t *data, size_t length) {
    if (length == 0) return 0;

    uint8_t checksum = 0;
    
    // XOR all bytes except the last one (which is the received checksum)
    for (size_t i = 0; i < length - 1; ++i) {
        checksum ^= data[i];
    }

    return checksum;
}

void send_poll_command (void)
{
	uint8_t poll_command[8] =        {
		0x48,              // Header 1
		0x53,              // Header 2
		0x00,              // Washer index
		0x01,              // Command
		0x01,              // Data Length high byte
		0x00,              // Data Length low byte
		0x00,              // Data byte
		0x00               // Checksum byte
		};
	poll_command[7] = calculate_checksum(poll_command, sizeof(poll_command));
	
	uart_write_bytes(UART_NUM_1,poll_command, sizeof(poll_command));
	
	// Wait for response (adjust timeout as needed)
    int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, pdMS_TO_TICKS(1000));
    
    if (len > 0) {
        ESP_LOGI("UART", "Received %d bytes:", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW("UART", "No data received");
    }
    if (len < 18) {
        printf("Invalid data length\n");
        return;
    }

    uint8_t Header1 = data[0];
    uint8_t Header2 = data[1];
    uint8_t washer_id = data[2];         // Washer ID
    uint32_t timestamp = (data[9]) | (data[8] << 8) | (data[7] << 16) | (data[6] << 24);
    uint8_t programmed_pumps = data[10];
    uint8_t pump_interface_status = data[11];
    uint8_t pump_run_status = data[12];
    uint8_t alarm_raw = data[13];
    uint8_t status = data[14];
    uint8_t formula_number = data[15];

    uint8_t setup_fw_status = data[16];
    uint8_t checksum = data[17];
    uint8_t c2d_setup_status = setup_fw_status & 0x03;
    uint8_t c2d_fw_status = setup_fw_status & 0x18;
    bool dst_active = setup_fw_status & 0x04;

    bool activity_available = (status & 0x10) > 0;
    bool power_up = (status & 0x20) > 0;
    bool records_available = (status & 0x40) > 0;
    bool setups_available = (status & 0x80) > 0;
    uint8_t op_state = status & 0x07;

    // Print parsed data
    printf("Header1 : %d\n", Header1);
    printf("Header2: %d\n", Header2);
    printf("Washer ID: %d\n", washer_id);
    printf("Timestamp: 0x%08lX\n", (unsigned long)timestamp);
    printf("programmed_pumps Status: %d\n", programmed_pumps);
    printf("Pump Interface Status: %d\n", pump_interface_status);
    printf("Pump Run Status: %d\n", pump_run_status);
    printf("Alarm: %d\n", alarm_raw & 0x7F);
    printf("Status Byte: 0x%02X\n", status);
    printf("checksum Byte: 0x%02X\n", checksum);
    printf("  - Activity Available: %s\n", activity_available ? "true" : "false");
    printf("  - Power Up: %s\n", power_up ? "true" : "false");
    printf("  - Records Available: %s\n", records_available ? "true" : "false");
    printf("  - Setups Available: %s\n", setups_available ? "true" : "false");
    printf("  - OpState: %d\n", op_state);
    printf("Active Formula: %d\n", formula_number);
    printf("c2dTESetupTransferringStatus: %d\n", c2d_setup_status);
    printf("c2dTEFirmwareTransferringStatus: %d\n", c2d_fw_status >> 3);
    printf("DST Active: %s\n", dst_active ? "true" : "false");
}

void send_READ_SOFTWARE_ID_command(void)
{
	uint8_t  READ_SOFTWARE_ID_command[] = {0x48, 0x53, 0x02, 0x01, 0x00, 0x00, 0x84};
    // Send command
    uart_write_bytes(UART_NUM_1, READ_SOFTWARE_ID_command, sizeof(READ_SOFTWARE_ID_command));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    // Extract fields
    uint8_t header1 = data[0];
    uint8_t header2 = data[1];
    uint8_t washer_id = data[2];
    uint8_t command_code = data[3];
    uint8_t Message_high_byte = data[4];
    uint8_t Message_low_byte = data[5];

    printf("Header1: %d\n", header1);
    printf("Header2: %d\n", header2);
    printf("Washer ID: %d\n", washer_id);
    printf("Command: %d\n", command_code);
    printf("Message_high_byte: %d\n", Message_high_byte);
    printf("Message_low_byte: %d\n", Message_low_byte);
    printf("Message Length: %d\n", len);

    // Extract Software ID String
    printf("Software ID String: ");
    for (int i = 6; i < 6 + len-1; i++) {
        if (data[i] >= 0x20 && data[i] <= 0x7E) {
            printf("%c", data[i]);
        } else {
            printf("\\x%02X", data[i]);
        }
    }
    printf("\n");

   uint8_t calculated_checksum = calculate_checksum(data,len);

   printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}

void send_C2D_config_command(const char *json_filepath) {
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
    message[2] = 0x04;  // Command: WRITE_C2D_SYS_CONFIG

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

void READ_MACINE_NAME(void)
{
	uint8_t  READ_MACINE_NAME_command[] = {0x48, 0x53, 0x0d, 0x01, 0x00, 0x00, 0xA9};
    // Send command
    uart_write_bytes(UART_NUM_1, READ_MACINE_NAME_command, sizeof(READ_MACINE_NAME_command));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    // Extract fields
    uint8_t header1 = data[0];
    uint8_t header2 = data[1];
    uint8_t washer_id = data[2];
    uint8_t command_code = data[3];
    uint8_t Message_high_byte = data[4];
    uint8_t Message_low_byte = data[5];

    printf("Header1: %d\n", header1);
    printf("Header2: %d\n", header2);
    printf("Washer ID: %d\n", washer_id);
    printf("Command: %d\n", command_code);
    printf("Message_high_byte: %d\n", Message_high_byte);
    printf("Message_low_byte: %d\n", Message_low_byte);
    printf("Message Length: %d\n", len);

    // Calculate message length using high and low byte
	int message_len = (Message_high_byte << 8) | Message_low_byte;
	
	// Ensure it doesn't exceed buffer
	if (message_len > BUF_SIZE - 6) message_len = BUF_SIZE - 6;
	
	printf("Machine Name String: ");
	
	// Create a temporary buffer to hold the string
	char name_str[message_len + 1]; // +1 for null terminator
	memcpy(name_str, &data[6], message_len);
	name_str[message_len] = '\0';  // Null-terminate
	
	// Print as a string
	printf("%s\n", name_str);
    printf("\n");

   uint8_t calculated_checksum = calculate_checksum(data,len);

   printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}


void READ_SETUP_TIMESTAMP_BY_INDEX(void)
{
	uint8_t READ_SETUP_TIMESTAMP_BY_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x0F, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00  // [7] Checksum (to be calculated)
};
	READ_SETUP_TIMESTAMP_BY_INDEX[6] = get_dynamic_value();  
	// ✅ Calculate checksum and assign to Byte 6
    READ_SETUP_TIMESTAMP_BY_INDEX[7] = calculate_checksum(READ_SETUP_TIMESTAMP_BY_INDEX, sizeof(READ_SETUP_TIMESTAMP_BY_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_SETUP_TIMESTAMP_BY_INDEX, sizeof(READ_SETUP_TIMESTAMP_BY_INDEX));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    // Extract fields
    uint8_t header1 = data[0];
    uint8_t header2 = data[1];
    uint8_t washer_id = data[2];
    uint8_t command_code = data[3];
    uint8_t Message_high_byte = data[4];
    uint8_t Message_low_byte = data[5];
    uint8_t received_checksum = data[len-1];

    printf("Header1: %d\n", header1);
    printf("Header2: %d\n", header2);
    printf("Washer ID: %d\n", washer_id);
    printf("Command: %d\n", command_code);
    printf("Message_high_byte: %d\n", Message_high_byte);
    printf("Message_low_byte: %d\n", Message_low_byte);
    printf("Message Length: %d\n", len);
    printf("received_checksum : %d\n", received_checksum);
 
	uint32_t timestamp = ((uint32_t)data[9] << 24) |
                         ((uint32_t)data[8] << 16) |
                         ((uint32_t)data[7] << 8)  |
                         ((uint32_t)data[6]);

    printf("Timestamp (hex): 0x%08" PRIX32 "\n", timestamp);
    printf("Timestamp (decimal): %" PRIu32 "\n", timestamp);


	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}

void READ_SETUP_INDEX(void)
{
	uint8_t READ_SETUP_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x11, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Index value (to be updated)
    0x00  // [6] Checksum (to be calculated)
};
	// ✅ Calculate checksum and assign to Byte 6
    READ_SETUP_INDEX[7] = calculate_checksum(READ_SETUP_INDEX, sizeof(READ_SETUP_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_SETUP_INDEX, sizeof(READ_SETUP_INDEX));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }
    printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("response : 0x%02X\n", data[6]);
    printf("received_checksum : %d\n", data[7]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}

void WRITE_SETUP_TRANSFERRED(void)
{
	uint8_t WRITE_SETUP_TRANSFERRED[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x13, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00  // [7] Checksum (to be calculated)
};
	WRITE_SETUP_TRANSFERRED[6] = get_dynamic_value();  
	// ✅ Calculate checksum and assign to Byte 6
    WRITE_SETUP_TRANSFERRED[7] = calculate_checksum(WRITE_SETUP_TRANSFERRED, sizeof(WRITE_SETUP_TRANSFERRED));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, WRITE_SETUP_TRANSFERRED, sizeof(WRITE_SETUP_TRANSFERRED));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("response : 0x%02X\n", data[6]);
    printf("received_checksum : %d\n", data[7]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);;
}

void READ_SYS_CONFIG_BY_INDEX(void)
{
	uint8_t READ_SYS_CONFIG_BY_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x13, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00  // [7] Checksum (to be calculated)
};
	READ_SYS_CONFIG_BY_INDEX[6] = get_dynamic_value();  
	// ✅ Calculate checksum and assign to Byte 6
    READ_SYS_CONFIG_BY_INDEX[7] = calculate_checksum(READ_SYS_CONFIG_BY_INDEX, sizeof(READ_SYS_CONFIG_BY_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_SYS_CONFIG_BY_INDEX, sizeof(READ_SYS_CONFIG_BY_INDEX));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    
    printf("  opMode: %d\n", data[6]);
	printf("  piType:  %d\n", data[7]);
	printf("  userPrime:  %d\n", data[8]);
	printf("  unitSystem:  %d\n", data[9]);
	printf("  flushTime:  %d\n", data[10]);
	printf("  cycleTime:  %d\n", data[11]);
	
	printf("  eventPumps[16]: ");
	for (int i = 12; i < len-3; i++) {
	    printf("%d\n", data[i]);
	}
	printf("\n");

	printf("  autoFormTrig:  %d\n", data[28]);
	printf("  filterOption:  %d\n", data[29]);
    
    
    printf("received_checksum :  0x%02X\n\n", data[30]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);;
}


void READ_PUMPS_BY_INDEX(void)
{
	uint8_t READ_PUMPS_BY_INDEX[9] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x13, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00,  // [7] data to be changed
    0x00  // [8] Checksum (to be calculated)
};
	READ_PUMPS_BY_INDEX[6] = get_dynamic_value();
	READ_PUMPS_BY_INDEX[7] = get_dynamic_value();    
	// ✅ Calculate checksum and assign to Byte 6
    READ_PUMPS_BY_INDEX[8] = calculate_checksum(READ_PUMPS_BY_INDEX, sizeof(READ_PUMPS_BY_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_PUMPS_BY_INDEX, sizeof(READ_PUMPS_BY_INDEX));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);	
	
	// Extract pumpName (16 characters)
	char pumpName[17];  // 16 chars + null terminator
	memcpy(pumpName, &data[6], 16);
	pumpName[16] = '\0';
	printf("pumpName: \"%s\"\n", pumpName);
	

    uint32_t pumpCostInt;
	float pumpCost;
	memcpy(&pumpCost, &data[22], sizeof(float));
	pumpCostInt = (uint32_t)pumpCost;
	
	printf("pumpCost (int): %" PRIu32 "\n", pumpCostInt);
	
    printf("received_checksum :  0x%02X\n\n", data[26]);
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}


//READ_FORMULAS_BY_INDEX
void READ_FORMULAS_BY_INDEX(void)
{
	uint8_t READ_FORMULAS_BY_INDEX[9] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x16, // [3] Command
    0x00, // [4] Message Length High Byte
    0x02, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00,  // [7] data to be changed
    0x00  // [8] Checksum (to be calculated)
};
	READ_FORMULAS_BY_INDEX[6] = get_dynamic_value();
	READ_FORMULAS_BY_INDEX[7] = get_dynamic_value();    
	// ✅ Calculate checksum and assign to Byte 6
    READ_FORMULAS_BY_INDEX[8] = calculate_checksum(READ_FORMULAS_BY_INDEX, sizeof(READ_FORMULAS_BY_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_FORMULAS_BY_INDEX, sizeof(READ_FORMULAS_BY_INDEX));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    printf("Header1: 0x%02X\n", data[0]);
	printf("Header2: 0x%02X\n", data[1]);
	printf("Washer ID: %d\n", data[2]);
	printf("Command: 0x%02X\n", data[3]);
	
	
    printf("Received Checksum: 0x%02X\n", data[91]);
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}

void READ_OLDEST_UNTXFD_SETUP_INDEX(void)
{
	uint8_t READ_OLDEST_UNTXFD_SETUP_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x17, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_OLDEST_UNTXFD_SETUP_INDEX[7] = calculate_checksum(READ_OLDEST_UNTXFD_SETUP_INDEX, sizeof(READ_OLDEST_UNTXFD_SETUP_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_OLDEST_UNTXFD_SETUP_INDEX, sizeof(READ_OLDEST_UNTXFD_SETUP_INDEX));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("  washer id: %d\n", data[6]);
    printf("received_checksum :  0x%02X\n\n", data[7]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
//READ_RECORD_INDEX
void READ_RECORD_INDEX(void)
{
	uint8_t READ_RECORD_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x18, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_RECORD_INDEX[7] = calculate_checksum(READ_RECORD_INDEX, sizeof(READ_RECORD_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_RECORD_INDEX, sizeof(READ_RECORD_INDEX));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("  RecordIndex[washer] : %d\n", data[6]);
    printf("  RecordIndex[washer] : %d\n", data[7]);
    printf("received_checksum :  0x%02X\n\n", data[8]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}

void READ_RECORD_BY_INDEX(void)
{
	uint8_t READ_RECORD_BY_INDEX[9] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x19, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00,  // [7] data to be changed
    0x00  // [8] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_RECORD_BY_INDEX[6] = get_dynamic_value();
	READ_RECORD_BY_INDEX[7] = get_dynamic_value();  
	READ_RECORD_BY_INDEX[8]= calculate_checksum(READ_RECORD_BY_INDEX, sizeof(READ_RECORD_BY_INDEX));
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_RECORD_BY_INDEX, sizeof(READ_RECORD_BY_INDEX));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

    printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("Record[washer]:");
    for (int i=6;i<len -1;i++)
    {
		printf(" 0x%02X", data[6]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n\n", data[8]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
void READ_OLDEST_UNTXFD_RECORD_INDEX(void)
{
	uint8_t READ_OLDEST_UNTXFD_RECORD_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x1A, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_OLDEST_UNTXFD_RECORD_INDEX[7] = calculate_checksum(READ_OLDEST_UNTXFD_RECORD_INDEX, sizeof(READ_OLDEST_UNTXFD_RECORD_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_OLDEST_UNTXFD_RECORD_INDEX, sizeof(READ_OLDEST_UNTXFD_RECORD_INDEX));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("  index of oldest unstransferred record : %d\n", data[6]);
    printf("  index of oldest unstransferred record : %d\n", data[7]);
    printf("received_checksum :  0x%02X\n\n", data[8]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
//WRITE_RECORD_TRANSFERRED
void WRITE_RECORD_TRANSFERRED(void)
{
	uint8_t WRITE_RECORD_TRANSFERRED[9] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x1C, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be changed
    0x00,  // [7] data to be changed
    0x00  // [8] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
	WRITE_RECORD_TRANSFERRED[6] = get_dynamic_value();
	WRITE_RECORD_TRANSFERRED[7] = get_dynamic_value();
    WRITE_RECORD_TRANSFERRED[8] = calculate_checksum(WRITE_RECORD_TRANSFERRED, sizeof(WRITE_RECORD_TRANSFERRED));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, WRITE_RECORD_TRANSFERRED, sizeof(WRITE_RECORD_TRANSFERRED));

    // Read response
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

    if (len < 7) {
        printf("Invalid data length (too short)\n");
        return;
    }

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("  max record index : %d\n", data[6]);
    printf("received_checksum :  0x%02X\n\n", data[7]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}	
//READ_SERIAL_NUMBER
void READ_SERIAL_NUMBER_command(void)
{
	uint8_t READ_SERIAL_NUMBER[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x20, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_SERIAL_NUMBER[7] = calculate_checksum(READ_SERIAL_NUMBER, sizeof(READ_SERIAL_NUMBER));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_SERIAL_NUMBER, sizeof(READ_SERIAL_NUMBER));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("serial number  : ");
    for(int i= 6; i<len-1;i++)
    {
		 printf("0x%02X ", data[i]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}

void READ_CURRENT_LOAD_START_TIMESTAMP(void)
{
	uint8_t READ_CURRENT_LOAD_START_TIMESTAMP[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x22, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_CURRENT_LOAD_START_TIMESTAMP[7] = calculate_checksum(READ_CURRENT_LOAD_START_TIMESTAMP, sizeof(READ_CURRENT_LOAD_START_TIMESTAMP));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_CURRENT_LOAD_START_TIMESTAMP, sizeof(READ_CURRENT_LOAD_START_TIMESTAMP));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("serial number  : ");
    for(int i= 6; i<len-1;i++)
    {
		 printf("0x%02X ", data[i]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
//READ_CURRENT_LOAD_FORMULA_NAME
void READ_CURRENT_LOAD_FORMULA_NAME(void)
{
	uint8_t READ_CURRENT_LOAD_FORMULA_NAME[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x23, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_CURRENT_LOAD_FORMULA_NAME[7] = calculate_checksum(READ_CURRENT_LOAD_FORMULA_NAME, sizeof(READ_CURRENT_LOAD_FORMULA_NAME));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_CURRENT_LOAD_FORMULA_NAME, sizeof(READ_CURRENT_LOAD_FORMULA_NAME));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("current load formula name   : ");
    for(int i= 6; i<len-1;i++)
    {
		 printf("%c", data[i]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
void READ_CURRENT_LOAD_WEIGHT(void)
{
	uint8_t READ_CURRENT_LOAD_WEIGHT[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x24, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_CURRENT_LOAD_WEIGHT[7] = calculate_checksum(READ_CURRENT_LOAD_WEIGHT, sizeof(READ_CURRENT_LOAD_WEIGHT));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_CURRENT_LOAD_WEIGHT, sizeof(READ_CURRENT_LOAD_WEIGHT));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("current load weight   : ");
    for(int i= 6; i<len-1;i++)
    {
		  printf("0x%02X ", data[i]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
void GET_OP_STATE(void)
{
	uint8_t GET_OP_STATE[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x25, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    GET_OP_STATE[7] = calculate_checksum(GET_OP_STATE, sizeof(GET_OP_STATE));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, GET_OP_STATE, sizeof(GET_OP_STATE));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    uint8_t opState = data[6];
	printf("OpState = 0x%02X & 0x07 = %d\n", opState, opState & 0x07);
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
void READ_LAST_CYCLE_START_TIMESTAMP(void)
{
	uint8_t READ_LAST_CYCLE_START_TIMESTAMP[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x26, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_LAST_CYCLE_START_TIMESTAMP[7] = calculate_checksum(READ_LAST_CYCLE_START_TIMESTAMP, sizeof(READ_LAST_CYCLE_START_TIMESTAMP));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_LAST_CYCLE_START_TIMESTAMP, sizeof(READ_LAST_CYCLE_START_TIMESTAMP));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    //last cycle start timestamp (cycle = load)
    printf("last cycle start timestamp (cycle = load)");
    for(int i=6;i<len-1;i++)
    {
		printf("0x%02X ", data[i]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
//READ_LAST_CYCLE_END_TIMESTAMP
void READ_LAST_CYCLE_END_TIMESTAMP(void)
{
	uint8_t READ_LAST_CYCLE_END_TIMESTAMP[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x27, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_LAST_CYCLE_END_TIMESTAMP[7] = calculate_checksum(READ_LAST_CYCLE_END_TIMESTAMP, sizeof(READ_LAST_CYCLE_END_TIMESTAMP));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_LAST_CYCLE_END_TIMESTAMP, sizeof(READ_LAST_CYCLE_END_TIMESTAMP));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    //last cycle start timestamp (cycle = load)
    printf("last cycle end timestamp :");
    for(int i=6;i<len-1;i++)
    {
		printf("0x%02X ", data[i]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
//READ_LAST_CYCLE_FORMULA_NAME
void READ_LAST_CYCLE_FORMULA_NAME(void)
{
	uint8_t READ_LAST_CYCLE_FORMULA_NAME[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x28, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_LAST_CYCLE_FORMULA_NAME[7] = calculate_checksum(READ_LAST_CYCLE_FORMULA_NAME, sizeof(READ_LAST_CYCLE_FORMULA_NAME));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_LAST_CYCLE_FORMULA_NAME, sizeof(READ_LAST_CYCLE_FORMULA_NAME));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    //last cycle start timestamp (cycle = load)
    printf("last cycle end timestamp :");
    for(int i=6;i<len-1;i++)
    {
		printf("%c", data[i]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
//READ_LAST_CYCLE_WEIGHT
void READ_LAST_CYCLE_WEIGHT(void)
{
	uint8_t READ_LAST_CYCLE_WEIGHT[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x29, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_LAST_CYCLE_WEIGHT[7] = calculate_checksum(READ_LAST_CYCLE_WEIGHT, sizeof(READ_LAST_CYCLE_WEIGHT));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_LAST_CYCLE_WEIGHT, sizeof(READ_LAST_CYCLE_WEIGHT));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    //last cycle start timestamp (cycle = load)
    printf("last cycle weight :");
    for(int i=6;i<len-1;i++)
    {
		printf("0x%02X ", data[i]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
void READ_LAST_CYCLE_FORMULA_NUMBER(void)
{
	uint8_t READ_LAST_CYCLE_WEIGHT[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x2A, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_LAST_CYCLE_WEIGHT[7] = calculate_checksum(READ_LAST_CYCLE_WEIGHT, sizeof(READ_LAST_CYCLE_WEIGHT));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_LAST_CYCLE_WEIGHT, sizeof(READ_LAST_CYCLE_WEIGHT));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    printf("last cycle formula number :%d\n",data[6]);
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
void READ_CURRENT_LOAD_FORMULA_NUMBER(void)
{
	uint8_t READ_LAST_CYCLE_WEIGHT[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x2B, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_LAST_CYCLE_WEIGHT[7] = calculate_checksum(READ_LAST_CYCLE_WEIGHT, sizeof(READ_LAST_CYCLE_WEIGHT));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_LAST_CYCLE_WEIGHT, sizeof(READ_LAST_CYCLE_WEIGHT));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    //last cycle start timestamp (cycle = load)
    printf("current load formula number :%d\n",data[6]);
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
void SET_DATETIME(void)
{
	const char *datetime = "2025-05-31 22:45:00"; // ⏰ Format: yyyy-MM-dd HH:mm:ss (length = 19)

    int payload_len = strlen(datetime); // 19
    int total_len = 6 + payload_len + 1; // header + payload + checksum

    uint8_t SET_DATETIME[64] = {
        0x48, // Header1 ('H')
        0x53, // Header2 ('S')
        0x01, // Washer ID
        0x32, // Command = SET_DATETIME
        (uint8_t)(payload_len >> 8),     // Message Length High Byte (0x00)
        (uint8_t)(payload_len & 0xFF)    // Message Length Low Byte (0x13)
    };

    // Copy ASCII bytes of datetime string
    memcpy(&SET_DATETIME[6], datetime, payload_len);

    // Calculate checksum (sum of all previous bytes)
    SET_DATETIME[6 + payload_len] = calculate_checksum(SET_DATETIME, 6 + payload_len);

    // Send UART data
    uart_write_bytes(UART_NUM_1, (const char *)SET_DATETIME, total_len);
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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    //last cycle start timestamp (cycle = load)
    printf("response :%d\n",data[6]);
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}

//READ_COMPANY_NAME
void READ_COMPANY_NAME(void)
{
	uint8_t READ_COMPANY_NAME[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x35, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_COMPANY_NAME[7] = calculate_checksum(READ_COMPANY_NAME, sizeof(READ_COMPANY_NAME));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_COMPANY_NAME, sizeof(READ_COMPANY_NAME));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    //last cycle start timestamp (cycle = load)
    printf("Company Name :");
    for(int i=6;i<len-1;i++)
    {
		printf("%c", data[i]);
	}
	printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}

void READ_ALTERNATE_BUILD_NUMBER(void)
{
	uint8_t READ_ALTERNATE_BUILD_NUMBER[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x39, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_ALTERNATE_BUILD_NUMBER[7] = calculate_checksum(READ_ALTERNATE_BUILD_NUMBER, sizeof(READ_ALTERNATE_BUILD_NUMBER));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_ALTERNATE_BUILD_NUMBER, sizeof(READ_ALTERNATE_BUILD_NUMBER));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    //last cycle start timestamp (cycle = load)
    printf("alternate build number : %d\n", data[6]);
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}

void READ_ACCOUNT_NAME(void)
{
	uint8_t READ_ACCOUNT_NAME[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x3B, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_ACCOUNT_NAME[7] = calculate_checksum(READ_ACCOUNT_NAME, sizeof(READ_ACCOUNT_NAME));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_ACCOUNT_NAME, sizeof(READ_ACCOUNT_NAME));

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

   	printf("Header1: %d\n",data[0]);
    printf("Header2: %d\n", data[1]);
    printf("Washer ID: %d\n", data[2]);
    printf("Command: %d\n", data[3]);
    printf("Message_high_byte: %d\n", data[4]);
    printf("Message_low_byte: %d\n", data[5]);
    //last cycle start timestamp (cycle = load)
    printf("Account Name :");
    for(int i=6;i<len-1;i++)
    {
		 printf("%c", data[i]);
	}
    printf("\n");
    printf("received_checksum :  0x%02X\n", data[len-1]);
    
	// Calculate checksum over the received data
	uint8_t calculated_checksum = calculate_checksum(data, len);
	printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
}
//WRITE_CLEAR_POWER_UP
void WRITE_CLEAR_POWER_UP(void)
{
	uint8_t WRITE_CLEAR_POWER_UP[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x3D, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    WRITE_CLEAR_POWER_UP[7] = calculate_checksum(WRITE_CLEAR_POWER_UP, sizeof(WRITE_CLEAR_POWER_UP));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, WRITE_CLEAR_POWER_UP, sizeof(WRITE_CLEAR_POWER_UP));

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

void READ_CYCLE_COUNTER(void)
{
	uint8_t READ_CYCLE_COUNTER[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x3E, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_CYCLE_COUNTER[7] = calculate_checksum(READ_CYCLE_COUNTER, sizeof(READ_CYCLE_COUNTER));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_CYCLE_COUNTER, sizeof(READ_CYCLE_COUNTER));

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
void READ_MACHINE_NAME_BY_INDEX(void)
{
	uint8_t READ_MACHINE_NAME_BY_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x3F, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_MACHINE_NAME_BY_INDEX[7] = calculate_checksum(READ_MACHINE_NAME_BY_INDEX, sizeof(READ_MACHINE_NAME_BY_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_MACHINE_NAME_BY_INDEX, sizeof(READ_MACHINE_NAME_BY_INDEX));

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
void READ_COMPANY_NAME_BY_INDEX(void)
{
	uint8_t READ_COMPANY_NAME_BY_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x40, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_COMPANY_NAME_BY_INDEX[7] = calculate_checksum(READ_COMPANY_NAME_BY_INDEX, sizeof(READ_COMPANY_NAME_BY_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_COMPANY_NAME_BY_INDEX, sizeof(READ_COMPANY_NAME_BY_INDEX));

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
void READ_ACCOUNT_NAME_BY_INDEX(void)
{
	uint8_t READ_ACCOUNT_NAME_BY_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x41, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_ACCOUNT_NAME_BY_INDEX[7] = calculate_checksum(READ_ACCOUNT_NAME_BY_INDEX, sizeof(READ_ACCOUNT_NAME_BY_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_ACCOUNT_NAME_BY_INDEX, sizeof(READ_ACCOUNT_NAME_BY_INDEX));

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
void READ_INSTALLERS_PASSWORD_BY_INDEX(void)
{
	uint8_t READ_INSTALLERS_PASSWORD_BY_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x42, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_INSTALLERS_PASSWORD_BY_INDEX[7] = calculate_checksum(READ_INSTALLERS_PASSWORD_BY_INDEX, sizeof(READ_INSTALLERS_PASSWORD_BY_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_INSTALLERS_PASSWORD_BY_INDEX, sizeof(READ_INSTALLERS_PASSWORD_BY_INDEX));

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
void READ_TUBE_ALARM_BY_INDEX(void)
{
	uint8_t READ_TUBE_ALARM_BY_INDEX[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x43, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_TUBE_ALARM_BY_INDEX[7] = calculate_checksum(READ_TUBE_ALARM_BY_INDEX, sizeof(READ_TUBE_ALARM_BY_INDEX));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, READ_TUBE_ALARM_BY_INDEX, sizeof(READ_TUBE_ALARM_BY_INDEX));

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
void SET_DST(void)
{
	uint8_t SET_DST[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x45, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    SET_DST[7] = calculate_checksum(SET_DST, sizeof(SET_DST));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, SET_DST, sizeof(SET_DST));

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
void READ_ACTIVITY(void)
{
	uint8_t READ_ACTIVITY[8] = {
    0x48, // [0] Header1
    0x53, // [1] Header2
    0x01, // [2] Washer ID
    0x46, // [3] Command
    0x00, // [4] Message Length High Byte
    0x01, // [5] Message Length Low Byte
    0x00,  // [6] data to be 
    0x00  // [7] Checksum (to be calculated)
}; 
	// ✅ Calculate checksum and assign to Byte 6
    READ_ACTIVITY[7] = calculate_checksum(READ_ACTIVITY, sizeof(READ_ACTIVITY));
	
    // Send command 
    uart_write_bytes(UART_NUM_1, SET_DST, sizeof(SET_DST));

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