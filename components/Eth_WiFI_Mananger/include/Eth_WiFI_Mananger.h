#pragma once

void wifi_init(void);
void ethernet_init(void);
void http_client_post_data(const char *url,const char *data);  
void download_file_to_emmc(const char *url, const char *filepath);
void upload_file_from_emmc(const char *url, const char *filepath);
void do_firmware_upgrade(const char *URL);
void check_and_handle_rollback();
void download_setup_data_emmc(const char *url, const char *filepath);
void http_get_string(const char *url);
void log_file_from_url(const char *url);
