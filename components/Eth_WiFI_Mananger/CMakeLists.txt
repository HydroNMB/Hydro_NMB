idf_component_register(SRCS "Eth_WiFi_Mananger.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp-tls
                       REQUIRES  configuration esp_event esp_wifi esp_http_client esp_eth esp_netif nvs_flash driver esp_https_ota app_update mbedtls configuration)
