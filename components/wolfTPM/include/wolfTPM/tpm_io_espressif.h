#ifndef TPM_IO_ESPRESSIF_H
#define TPM_IO_ESPRESSIF_H

#include "tpm2.h"
void tpm_init(void);
void tpm2_read_persistent_key(TPMI_DH_OBJECT handle);
TPM_HANDLE  tpm2_createprimary(void);
const char* generate_sas_token_tpm(const char *resource_uri, uint32_t expiry);
int tpm_sign_with_persistent_key(const uint8_t* data, size_t data_len, uint8_t* out_sig, uint16_t* out_sig_len);
#endif // BLE_H