#include "test_azure_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "azure/az_iot.h"
#include <az_iot_provisioning_client.h>
#include "tpm_io_espressif.h"
#include "mbedtls/base64.h"

#define REGISTRATION_ID        "your-device-id"
#define DPS_ID_SCOPE           "your-scope-id"

static const char *TAG =          "ESP_azure_sdk";
static az_iot_provisioning_client prov_client;

void azure_dps(void)
{
    az_result rc;

    rc = az_iot_provisioning_client_init(
        &prov_client,
        az_span_create_from_str("global.azure-devices-provisioning.net"),
        az_span_create_from_str(DPS_ID_SCOPE),
        az_span_create_from_str(REGISTRATION_ID),
        NULL);

    if (az_result_failed(rc)) {
        printf("Provisioning client init failed: 0x%08x\n", (unsigned int)rc);
        return;
    }

    printf("Provisioning client initialized successfully\n");
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
	    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
	
	    // 1. Subscribe to DPS response topic
	    esp_mqtt_client_subscribe(client, "$dps/registrations/res/#", 1);
	
	    // 2. Publish register request
	    const char* register_topic = "$dps/registrations/PUT/iotdps-register/?$rid=1";
	    const char* register_payload = "{\"registrationId\":\"" REGISTRATION_ID "\"}";
	
	    esp_mqtt_client_publish(client, register_topic, register_payload, 0, 1, 0);
    	break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA: {
	    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
	    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
	    printf("DATA=%.*s\r\n", event->data_len, event->data);
	
	    char topic[event->topic_len + 1];
	    char data[event->data_len + 1];
	    memcpy(topic, event->topic, event->topic_len);
	    topic[event->topic_len] = '\0';
	    memcpy(data, event->data, event->data_len);
	    data[event->data_len] = '\0';
	
	    // 1. Look for 202 response -> polling needed
	    if (strstr(topic, "res/202")) {
	        char* op_id_ptr = strstr(data, "\"operationId\":\"");
	        if (op_id_ptr) {
	            op_id_ptr += strlen("\"operationId\":\"");
	            char* end = strchr(op_id_ptr, '"');
	            if (end) {
	                *end = '\0';
	
	                char operation_topic[256];
	                snprintf(operation_topic, sizeof(operation_topic),
	                    "$dps/registrations/GET/iotdps-get-operationstatus/?$rid=2&operationId=%s",
	                    op_id_ptr);
	
	                ESP_LOGI(TAG, "Polling DPS status with operationId: %s", op_id_ptr);
	                // Small delay before polling
	                vTaskDelay(3000 / portTICK_PERIOD_MS);
	                esp_mqtt_client_publish(client, operation_topic, NULL, 0, 1, 0);
	            }
	        }
	    }
	
	    // 2. Look for 200 response -> provisioning complete
	    else if (strstr(topic, "res/200")) {
	        char* hub_ptr = strstr(data, "\"assignedHub\":\"");
	        char* id_ptr = strstr(data, "\"deviceId\":\"");
	        if (hub_ptr && id_ptr) {
	            hub_ptr += strlen("\"assignedHub\":\"");
	            id_ptr += strlen("\"deviceId\":\"");
	
	            char* hub_end = strchr(hub_ptr, '"');
	            char* id_end = strchr(id_ptr, '"');
	
	            if (hub_end && id_end) {
	                *hub_end = '\0';
	                *id_end = '\0';
	
	                ESP_LOGI(TAG, "Provisioned to hub: %s with deviceId: %s", hub_ptr, id_ptr);
	
	                // ✅ Device successfully provisioned
	                // 👉 Save hub_ptr and id_ptr and connect to IoT Hub
	                // (You’ll create new MQTT connection here)
	            }
	        }
	    }
	
	    break;
	}
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

// Define the function
char* url_encode(const char* str) {
    size_t len = strlen(str);
    char* encoded = malloc(len * 3 + 1);  // worst case
    char* p = encoded;

    for (size_t i = 0; i < len; ++i) {
        switch (str[i]) {
            case '+': strcpy(p, "%2B"); p += 3; break;
            case '/': strcpy(p, "%2F"); p += 3; break;
            case '=': strcpy(p, "%3D"); p += 3; break;
            default: *p++ = str[i]; break;
        }
    }
    *p = '\0';
    return encoded;
}

// TPM SAS Token Generation
int generate_sas_token(const char* resource_uri, uint32_t expiry_time, char* sas_token_buf, size_t buf_len)
{
    char to_sign[256];
    snprintf(to_sign, sizeof(to_sign), "%s\n%lu", resource_uri, (unsigned long)expiry_time);

    uint8_t signature[256];
    uint16_t sig_len = 0;

    if (tpm_sign_with_persistent_key((const uint8_t*)to_sign, strlen(to_sign), signature, &sig_len) != 0) {
        printf("TPM signing failed\n");
        return -1;
    }

    char base64_sig[512];
    size_t b64_len;
    if (mbedtls_base64_encode((unsigned char*)base64_sig, sizeof(base64_sig), &b64_len, signature, sig_len) != 0) {
        printf("Base64 encoding failed\n");
        return -1;
    }
    base64_sig[b64_len] = '\0';

    // URL encode base64_sig — assuming url_encode returns a malloc'd pointer, remember to free later.
    char* encoded_sig = url_encode(base64_sig);
    if (!encoded_sig) {
        printf("URL encoding failed\n");
        return -1;
    }

    int written = snprintf(sas_token_buf, buf_len,
            "SharedAccessSignature sr=%s&sig=%s&se=%lu",
            resource_uri, encoded_sig, (unsigned long)expiry_time);

    free(encoded_sig);

    if (written < 0 || (size_t)written >= buf_len) {
        printf("SAS token buffer too small\n");
        return -1;
    }

    return 0; // success
}


void azure_mqtt(const char* sas_token)
{
    // Prepare username and client ID buffers
    char  username[256];
    uint32_t username_len;
    char  client_id[128];
    uint32_t client_id_len;

    if (az_iot_provisioning_client_get_user_name(&prov_client, username, sizeof(username), &username_len) != AZ_OK ||
        az_iot_provisioning_client_get_client_id(&prov_client, client_id, sizeof(client_id), &client_id_len) != AZ_OK) {
        printf("Failed to get MQTT username or client ID\n");
        return;
    }

    // Null-terminate username and client_id strings
    username[username_len] = '\0';
    client_id[client_id_len] = '\0';

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = "mqtts://global.azure-devices-provisioning.net:8883",
            .verification = {
                .crt_bundle_attach = esp_crt_bundle_attach
            }
        },
        .credentials = {
            .username = (const char*)username,
            .client_id = (const char*)client_id,
            .authentication = {
                .password = sas_token,
            },
        },
        .session.keepalive = 120,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void device_provisioning_start(void)
{
	char resource_uri[128];
    snprintf(resource_uri, sizeof(resource_uri),
         "global.azure-devices-provisioning.net/%s/registrations/%s",
           DPS_ID_SCOPE, REGISTRATION_ID);

    uint32_t expiry = time(NULL) + 3600;
    char sas_token[1024];
    
    azure_dps();
    
    if (generate_sas_token(resource_uri, expiry, sas_token, sizeof(sas_token)) == 0) {
        printf("Generated SAS token:\n%s\n", sas_token);
        // Now pass sas_token to MQTT config as password
    } else {
        printf("Failed to generate SAS token\n");
    }
    
    azure_mqtt(sas_token);
}
