#pragma once

#include <stdbool.h>

typedef void (*azure_dps_result_cb_t)(bool success, const char *assigned_hub, const char *device_id);
void azure_dps_start_provisioning(const char *id_scope, const char *registration_id, azure_dps_result_cb_t cb);
void azure_dps_stop_provisioning(void);
