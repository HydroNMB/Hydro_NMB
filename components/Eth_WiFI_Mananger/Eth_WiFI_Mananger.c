#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_eth.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_crt_bundle.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "Eth_WiFI_Mananger.h"
#include "configuration.h"

static const char *TAG = "Net_Manager";
static char last_message[256] = "";  // Now global or static

extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_server_cert_pem_end");
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
bool eth_connected = false;

// Pin Assignment of W5500
#define RSTn           GPIO_NUM_9
#define INTn           GPIO_NUM_10
#define MOSI_ETH       GPIO_NUM_11
#define MISO_ETH       GPIO_NUM_12
#define SCLK_ETH       GPIO_NUM_13
#define CS             GPIO_NUM_14

void wifi_init(void);

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
   	uint8_t custom_mac[6];
     
    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_ERROR_CHECK( esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, custom_mac));
      
        ESP_LOGI(TAG, "Ethernet Connected");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 custom_mac[0], custom_mac[1], custom_mac[2], custom_mac[3], custom_mac[4], custom_mac[5]);         
        esp_wifi_stop();
        ESP_LOGI(TAG, "WiFi  Disconnected");
        eth_connected = true;
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Disconnected try to connect WiFi");
        esp_wifi_start();
        esp_err_t esp_wifi_connect(void);
        eth_connected = false;
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
			case WIFI_EVENT_STA_CONNECTED:
			     ESP_LOGI(TAG, "Connected to Wi-Fi .. STA IP :\n");
			     break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Connecting to Wi-Fi...");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                
                if(!eth_connected){
					ESP_LOGI(TAG, "Disconnected from Wi-Fi. Retrying...");
                	esp_wifi_connect();
                	}
                break;
            default:
                break;
        }
    } 
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
	ESP_LOGI(TAG, "Got IP event handler triggered!");
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "ESP Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ESPIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ESPMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ESPGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}


void wifi_init(void)
{
     // Create default Wi-Fi station interface
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    
     // Set up Wi-Fi event handler
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_event_handler, NULL));
    
     // Configure Wi-Fi in station mode
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20));
    
    ESP_ERROR_CHECK( esp_wifi_start());
    
    ESP_LOGI(TAG, "Wi-Fi initialization done.");
    
/*    ESP_ERROR_CHECK (esp_wifi_connect());*/

}
// OTA upgrade
void do_firmware_upgrade(const char *URL)
{
    esp_http_client_config_t config = {
        .url = URL,
        .timeout_ms = 5000,
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
        //.transport_type = HTTP_TRANSPORT_OVER_SSL,
        .cert_pem = NULL,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .partial_http_download = true,
    };

    ESP_LOGI(TAG, "Starting OTA from URL: %s", URL);

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK || https_ota_handle == NULL) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
    }
 

   do {
	    err = esp_https_ota_perform(https_ota_handle);
	    if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
	        break;
	    }
	} while (true);
	
	if (err != ESP_OK) {
	    ESP_LOGE(TAG, "OTA perform failed: %s", esp_err_to_name(err));
	    esp_https_ota_abort(https_ota_handle);
	    return;
	}

    esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if (ota_finish_err != ESP_OK) {
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(ota_finish_err));
    }

    ESP_LOGI(TAG, "OTA Successful. Restarting...");
    esp_restart();
}

void ethernet_init(void)
{
	 // Install GPIO interrupt service (as the SPI-Ethernet module is interrupt-driven)
	gpio_set_direction(INTn, GPIO_MODE_INPUT);
	gpio_set_intr_type(INTn, GPIO_INTR_POSEDGE);
	gpio_install_isr_service(0);
	
	
    ESP_ERROR_CHECK(esp_netif_init());
	
	// Create Ethernet interface
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&netif_config);
    
    spi_bus_config_t buscfg = {
	    .miso_io_num = MISO_ETH,
	    .mosi_io_num = MOSI_ETH,
	    .sclk_io_num = SCLK_ETH,
	    .quadwp_io_num = -1,
	    .quadhd_io_num = -1,
	    .max_transfer_sz = 4096, // Important for safe transactions
	};
	ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO ));
    
    static spi_device_interface_config_t spi_devcfg = {
	    .mode = 0,  // SPI mode 0
	    .clock_speed_hz = 40 * 1000 * 1000,  // 8MHz
	    .spics_io_num = CS,  // Chip select pin
	    .queue_size = 20
	    };
	
	
    eth_w5500_config_t w5500_cnfig = ETH_W5500_DEFAULT_CONFIG(SPI3_HOST, &spi_devcfg);
    
	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();      // apply default common MAC configuration
	esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_cnfig, &mac_config);
	
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();      // apply default PHY configuration
	phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
    phy_config.reset_gpio_num = -1; // ENC28J60 doesn't have a pin to reset internal PHY    	
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
    
    // Ethernet driver configuration
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle  = NULL;esp_err_t ret = esp_eth_driver_install(&eth_config, &eth_handle);
    if (ret != ESP_OK)
    {
		ESP_LOGE(TAG, "Failed to install Ethernet driver: %s", esp_err_to_name(ret));
    	return;
	}
	
	 mac->set_addr(mac, (uint8_t[]) {
        0x02, 0x00, 0x00, 0x12, 0x34, 0x56
    });

        // Enable DHCP on the Ethernet interface
	ESP_ERROR_CHECK(esp_netif_dhcpc_start(eth_netif));
	
     // Attach Ethernet interface
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    
    	// register Ethernet event handler (to deal with user-specific stuff when events like link up/down happened)
	esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL); 
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
	ESP_LOGI(TAG, "IP event handler registered successfully!");

	    // Start Ethernet
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    ESP_LOGI(TAG, "Ethernet initialized successfully");
}
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
           /* if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }*/

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
#if CONFIG_EXAMPLE_ENABLE_RESPONSE_BUFFER_DUMP
                ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
#endif
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}


void http_client_post_data(const char *url,const char *data)
{
	 esp_http_client_config_t config = {
	    .url = url,
	    .timeout_ms = 5000,
	    .transport_type = HTTP_TRANSPORT_OVER_SSL,
	    .event_handler = _http_event_handler,
	    .crt_bundle_attach = esp_crt_bundle_attach, // Enables built-in CA certificates
	    .disable_auto_redirect = true,
	};

    ESP_LOGI(TAG, "HTTPS async requests =>");
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    /*const char *post_data = " linkedloops iot devices  "
                            "ESP 32 HTTP CLIENT , "
                            "HTTP POST DATA  "
                            "HELLOW FROM ESP32 ";*/
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, data, strlen(data));
    while (1) {
        err = esp_http_client_perform(client);
        if (err != ESP_ERR_HTTP_EAGAIN) {
            break;
        }
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

// Function to perform HTTP GET
void http_get_string(const char *url)
{
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int content_length = esp_http_client_get_content_length(client);
        if (content_length > 0 && content_length < 255) {
            char *buffer = malloc(content_length + 1);
            if (buffer) {
                int read_len = esp_http_client_read_response(client, buffer, content_length);
                buffer[read_len] = '\0';

                // Only print if message is new and not empty
                if (strlen(buffer) > 0 && strcmp(buffer, last_message) != 0) {
                    ESP_LOGI(TAG, "Message from server: %s", buffer);
                    strcpy(last_message, buffer);
                }
                free(buffer);
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}



void check_and_handle_rollback()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGW(TAG, "App is pending verification. Marking as valid...");
            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
                ESP_LOGI(TAG, "App successfully marked as valid.");
            } else {
                ESP_LOGE(TAG, "Failed to mark app valid. Rolling back...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        } else {
            ESP_LOGI(TAG, "App is already valid. State: %d", ota_state);
        }
    } else {
        ESP_LOGE(TAG, "Failed to get OTA state.");
    }
}



//*******************************************************************************************************************************************************/
void upload_file_from_emmc(const char *url, const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", filepath);
        return;
    }
    
    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    // Set up HTTP client config
    esp_http_client_config_t config = {
        .url = url,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set HTTP method and body
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json"); // adjust if needed
    
    // If server does NOT support chunked encoding, use this:
    char content_length_str[32];
    snprintf(content_length_str, sizeof(content_length_str), "%ld", file_size);
    esp_http_client_set_header(client, "Content-Length", content_length_str);
    
    esp_err_t err = esp_http_client_open(client, file_size); // -1 means use chunked transfer encoding
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        fclose(file);
        esp_http_client_cleanup(client);
        return;
    }

    char buffer[512];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        int bytes_written = esp_http_client_write(client, buffer, bytes_read);
        if (bytes_written < 0) {
            ESP_LOGE(TAG, "Error writing data to server");
            break;
        }
    }

    fclose(file);

    // Finalize the request
    int status_code = -1;
    err = esp_http_client_fetch_headers(client);
    if (err == ESP_OK) {
        status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "File uploaded successfully, HTTP Status = %d", status_code);
    } else {
        ESP_LOGE(TAG, "Failed to fetch headers: %s", esp_err_to_name(err));
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

void log_file_from_url(const char *url) {
    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(TAG, "Failed to fetch HTTP headers");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    char buffer[512];
    int read_len;
    ESP_LOGI(TAG, "Reading content from URL: %s", url);
    while ((read_len = esp_http_client_read(client, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[read_len] = '\0';  // Null-terminate for safe logging
        //ESP_LOGI(TAG, "%s", buffer);
        printf("Received data : %s",buffer);
        if(strncmp((char *)buffer, "ota", 3) == 0)
            {
				do_firmware_upgrade(OTA_URL);	
			}
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI(TAG, "Finished reading from URL");
}

void download_file_to_emmc(const char *url, const char *filepath) {
    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(TAG, "Failed to fetch HTTP headers");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }
    // Optional: create directory if it doesn't exist
    mkdir("/emmc", 0775); // Change this based on your mount path if different

    // Remove existing file if any
    remove(filepath);
    ESP_LOGI(TAG, "Trying to write file: %s", filepath);

    FILE *file = fopen(filepath, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filepath);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    char buffer[512];
    int read_len;
    while ((read_len = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, read_len, file);
    }

    fclose(file);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "File downloaded and saved to eMMC at: %s", filepath);
}

void download_setup_data_emmc(const char *url, const char *filepath) {
    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(TAG, "Failed to fetch HTTP headers");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }
    // Optional: create directory if it doesn't exist
    mkdir("/emmc", 0775); // Change this based on your mount path if different

    // Remove existing file if any
    remove(filepath);
    ESP_LOGI(TAG, "Trying to write file: %s", filepath);

    FILE *file = fopen(filepath, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filepath);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    char buffer[512];
    int read_len;
    while ((read_len = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, read_len, file);
    }

    fclose(file);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "File downloaded and saved to eMMC at: %s", filepath);
    
}
