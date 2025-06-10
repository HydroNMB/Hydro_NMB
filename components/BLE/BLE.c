#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"
#include "BLE.h"
#include "Eth_WiFI_Mananger.h"
#include "configuration.h"

// Define the GATT service UUID and characteristic UUID
#define GATTS_SERVICE_UUID_TEST_A 0x00FF
#define GATTS_CHAR_UUID_TEST_A 0xFF00
#define TAG "BLE_SERVER"
#define BLE_name "ESP32_BLE_Device"

#define GATTS_NUM_HANDLE_TEST_A 4
#define CHAR_PROP_READ_WRITE (ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE_NR)


static uint8_t adv_service_uuid128[32] = {
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010, 
    .appearance = 0x00,
    .manufacturer_len = 0, 
    .p_manufacturer_data =  NULL, 
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// Advertising parameters
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};;

// Global flag to track advertisement data setup completion
static uint8_t adv_config_done = 0;
#define adv_config_flag 0x01

// GAP event handler (handling advertising events)
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~adv_config_flag);
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising stop failed");
            } else {
                ESP_LOGI(TAG, "Stop adv successfully");
            }
            break;
        default:
            break;
    }
}

// GATTS event handler (handling GATT events)
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(TAG, "GATT Registration Event");

            // Setup the service UUID and add the service
            esp_ble_gap_set_device_name(BLE_name);
            esp_ble_gap_config_adv_data(&adv_data);

            // When advertising data is set, the advertisement will start
            adv_config_done |= adv_config_flag;
            
            esp_gatt_srvc_id_t service_id;
			service_id.is_primary = true;
			service_id.id.inst_id = 0x00;
			service_id.id.uuid.len = ESP_UUID_LEN_16;
			service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;
			
			esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLE_TEST_A);

            break;

        case ESP_GATTS_READ_EVT:
            ESP_LOGI(TAG, "GATT Read Event");

            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = 4;
            rsp.attr_value.value[0] = 0xdd;
            rsp.attr_value.value[1] = 0xed;
            rsp.attr_value.value[2] = 0xbe;
            rsp.attr_value.value[3] = 0xdf;
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            break;

        case ESP_GATTS_WRITE_EVT:
		    if (!param->write.is_prep) {
		        ESP_LOGI(TAG, "Write Value Length: %d", param->write.len);
		        esp_log_buffer_hex(TAG, param->write.value, param->write.len);
		
		         // Compare received value to "OTA"
			    if (param->write.len == 3 &&
			        memcmp(param->write.value, "OTA", 3) == 0) {
			        ESP_LOGI(TAG, "OTA command received.");
			        do_firmware_upgrade(OTA_URL); // OTA  function
			    }
		        // Send a write response
		        esp_ble_gatts_send_response(gatts_if,
		                                    param->write.conn_id,
		                                    param->write.trans_id,
		                                    ESP_GATT_OK, NULL);
		    }
		    break;
		    
        case ESP_GATTS_EXEC_WRITE_EVT:
            ESP_LOGI(TAG, "Execute Write Event");
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            break;

        case ESP_GATTS_CREATE_EVT:
            ESP_LOGI(TAG, "Service Create Event");

            uint16_t service_handle = param->create.service_handle;

		    esp_gatt_char_prop_t property = CHAR_PROP_READ_WRITE;
		    esp_attr_control_t control = {.auto_rsp = ESP_GATT_RSP_BY_APP};
		    
		    esp_attr_value_t gatts_demo_char_val = {
		        .attr_max_len = 0x40,
		        .attr_len     = sizeof(uint8_t),
		        .attr_value   = (uint8_t *)"\x00",
		    };
		
		    esp_bt_uuid_t char_uuid = {
		        .len = ESP_UUID_LEN_16,
		        .uuid = {.uuid16 = GATTS_CHAR_UUID_TEST_A},
		    };
		
		    esp_err_t add_char_ret = esp_ble_gatts_add_char(service_handle, &char_uuid,
		                                                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
		                                                    property, &gatts_demo_char_val, &control);
		    if (add_char_ret) {
		        ESP_LOGE(TAG, "Failed to add characteristic: %d", add_char_ret);
		    }
		
		    esp_ble_gatts_start_service(service_handle);
		    break;

        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "GATT Connect Event");
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "GATT Disconnect Event");
            esp_ble_gap_config_adv_data(&adv_data);
            break;

        default:
            break;
    }
}

// Initialize and configure BLE advertising
void Ble_init()
{
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();
	
    esp_err_t ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GAP Register Callback Failed");
        return;
    }
    
     ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(TAG, "GATTS Register Callback Failed");
        return;
    }

    // Register the GATTS application
    ret = esp_ble_gatts_app_register(0);
    if (ret) {
        ESP_LOGE(TAG, "GATTS App Register Failed");
        return;
    }
}

