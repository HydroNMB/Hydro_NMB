#include <stdio.h>
#include "coding.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "wolfTPM/tpm2.h"
#include "wolfTPM/tpm2_tis.h"
#include "wolfTPM/tpm2_types.h"
#include "wolfTPM/tpm2_wrap.h"
#include "mbedtls/base64.h"
#include "wolfssl/wolfcrypt/sha256.h"
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include "configuration.h"
#include "tpm_io_espressif.h"

TPM2_CTX tpm;

#define DEBUG_WOLFTPM
static const char *TAG =          "ESP_TPM";
// I2C Configuration
#define I2C_MASTER_SCL_IO           2
#define I2C_MASTER_SDA_IO           1
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TIMEOUT_MS       500 
#define I2C_TIMEOUT_TICKS           pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)

// TPM & GPIO Pins
#define TPM_I2C_ADDRESS             0x2E // 7-bit address
#define TPM_RST_IO                  20
#define TPM_INTR_IO                 19
#define LED_PIN                     8
char sas_token[512];




TPM2_AUTH_SESSION session[MAX_SESSION_NUM];

 union {
        Startup_In startup;
        Shutdown_In shutdown;
        SelfTest_In selfTest;
        GetRandom_In getRand;
        StirRandom_In stirRand;
        GetCapability_In cap;
        IncrementalSelfTest_In incSelfTest;
        PCR_Read_In pcrRead;
        PCR_Extend_In pcrExtend;
        PCR_Reset_In pcrReset;
        CreatePrimary_In createPri;
        Create_In create;
        CreateLoaded_In createLoaded;
        EvictControl_In evict;
        ReadPublic_In readPub;
        StartAuthSession_In authSes;
        Load_In load;
        LoadExternal_In loadExt;
        FlushContext_In flushCtx;
        Unseal_In unseal;
        PolicyGetDigest_In policyGetDigest;
        PolicyPCR_In policyPCR;
        PolicyRestart_In policyRestart;
        PolicyCommandCode_In policyCC;
        Clear_In clear;
        HashSequenceStart_In hashSeqStart;
        SequenceUpdate_In seqUpdate;
        SequenceComplete_In seqComp;
        MakeCredential_In makeCred;
        ObjectChangeAuth_In objChgAuth;
        NV_ReadPublic_In nvReadPub;
        NV_DefineSpace_In nvDefine;
        NV_UndefineSpace_In nvUndefine;
        RSA_Encrypt_In rsaEnc;
        RSA_Decrypt_In rsaDec;
        Sign_In sign;
        VerifySignature_In verifySign;
        ECC_Parameters_In eccParam;
        ECDH_KeyGen_In ecdh;
        ECDH_ZGen_In ecdhZ;
        EncryptDecrypt2_In encDec;
        CertifyCreation_In certifyCreation;
        HMAC_In hmac;
        HMAC_Start_In hmacStart;
#if defined(WOLFTPM_ST33) || defined(WOLFTPM_AUTODETECT)
        SetCommandSet_In setCmdSet;
#endif
        byte maxInput[MAX_COMMAND_SIZE];
    } cmdIn;
    union {
        GetCapability_Out cap;
        GetRandom_Out getRand;
    #if defined(WOLFTPM_ST33) || defined(WOLFTPM_AUTODETECT)
        GetRandom2_Out getRand2;
    #endif
        GetTestResult_Out tr;
        IncrementalSelfTest_Out incSelfTest;
        ReadClock_Out readClock;
        PCR_Read_Out pcrRead;
        CreatePrimary_Out createPri;
        Create_Out create;
        CreateLoaded_Out createLoaded;
        ReadPublic_Out readPub;
        StartAuthSession_Out authSes;
        Load_Out load;
        LoadExternal_Out loadExt;
        Unseal_Out unseal;
        PolicyGetDigest_Out policyGetDigest;
        HashSequenceStart_Out hashSeqStart;
        SequenceComplete_Out seqComp;
        MakeCredential_Out makeCred;
        ObjectChangeAuth_Out objChgAuth;
        NV_ReadPublic_Out nvReadPub;
        RSA_Encrypt_Out rsaEnc;
        RSA_Decrypt_Out rsaDec;
        Sign_Out sign;
        VerifySignature_Out verifySign;
        ECC_Parameters_Out eccParam;
        ECDH_KeyGen_Out ecdh;
        ECDH_ZGen_Out ecdhZ;
        EncryptDecrypt2_Out encDec;
        CertifyCreation_Out certifyCreation;
        HMAC_Out hmac;
        HMAC_Start_Out hmacStart;
        byte maxOutput[MAX_RESPONSE_SIZE];
    } cmdOut;
    
    int pcrCount, pcrIndex, i;
    TPML_PCR_SELECTION* pcrSel;
    TPML_TAGGED_TPM_PROPERTY* tpmProp;
    TPM_HANDLE handle = TPM_RH_NULL;
    TPM_HANDLE sessionHandle = TPM_RH_NULL;
    GetRandom_Out getRand2;

int TPM_ESP32_I2C_Init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    return 0;
}

int tpm_espressif_i2c_io_cb(struct TPM2_CTX* ctx, INT32 isRead, UINT32 addr,
                            BYTE* xferBuf, UINT16 xferSz, void* userCtx)
{
    esp_err_t ret;
    uint8_t reg = (uint8_t)addr;

    if (isRead) {
        ret = i2c_master_write_read_device(I2C_MASTER_NUM, TPM_I2C_ADDRESS, &reg, 1,
                                           xferBuf, xferSz, I2C_TIMEOUT_TICKS);
    } else {
        uint8_t writeBuf[1 + xferSz];
        writeBuf[0] = reg;
        memcpy(&writeBuf[1], xferBuf, xferSz);

        ret = i2c_master_write_to_device(I2C_MASTER_NUM, TPM_I2C_ADDRESS,
                                         writeBuf, sizeof(writeBuf), I2C_TIMEOUT_TICKS);
    }

    return (ret == ESP_OK) ? 0 : -1;
}

TPM_HANDLE tpm2_createprimary(void)
{
    CreatePrimary_In cpIn;
    CreatePrimary_Out cpOut;

    XMEMSET(&cpIn, 0, sizeof(cpIn));
    XMEMSET(&cpOut, 0, sizeof(cpOut));

    cpIn.primaryHandle = TPM_RH_ENDORSEMENT;

    // Set sensitive fields empty
    cpIn.inSensitive.sensitive.userAuth.size = 0;
    cpIn.inSensitive.sensitive.data.size = 0;

    // Set public parameters as required for EK
    cpIn.inPublic.publicArea.type = TPM_ALG_RSA;
    cpIn.inPublic.publicArea.nameAlg = TPM_ALG_SHA256;

    cpIn.inPublic.publicArea.objectAttributes = 
        TPMA_OBJECT_fixedTPM |
        TPMA_OBJECT_fixedParent |
        TPMA_OBJECT_adminWithPolicy |
        TPMA_OBJECT_restricted |
        TPMA_OBJECT_decrypt |
        TPMA_OBJECT_sensitiveDataOrigin;

    // EK template policy for RSA (standard from TCG)
    static const BYTE ek_policy[] = {
        0x83, 0x71, 0x97, 0x67, 0x44, 0x84, 0xB3, 0xF8,
        0x1A, 0x90, 0xCC, 0x8D, 0x46, 0xA5, 0xD7, 0x24,
        0xFD, 0x52, 0xD7, 0x6E, 0x06, 0x62, 0x0D, 0xA0,
        0xAF, 0xD2, 0x56, 0x0A, 0xB1, 0x55, 0x8D, 0xC0
    };

    cpIn.inPublic.publicArea.authPolicy.size = sizeof(ek_policy);
    XMEMCPY(cpIn.inPublic.publicArea.authPolicy.buffer, ek_policy, sizeof(ek_policy));

    cpIn.inPublic.publicArea.parameters.rsaDetail.symmetric.algorithm = TPM_ALG_AES;
    cpIn.inPublic.publicArea.parameters.rsaDetail.symmetric.keyBits.aes = 128;
    cpIn.inPublic.publicArea.parameters.rsaDetail.symmetric.mode.aes = TPM_ALG_CFB;
    cpIn.inPublic.publicArea.parameters.rsaDetail.scheme.scheme = TPM_ALG_NULL;
    cpIn.inPublic.publicArea.parameters.rsaDetail.keyBits = 2048;
    cpIn.inPublic.publicArea.parameters.rsaDetail.exponent = 0; // default exponent
    cpIn.inPublic.publicArea.unique.rsa.size = 0;

    int ret = TPM2_CreatePrimary(&cpIn, &cpOut);
    if (ret != TPM_RC_SUCCESS) {
        printf("TPM2_CreatePrimary failed: 0x%X\n", ret);
        return 0;
    }

    printf("TPM2_CreatePrimary succeeded!\n");
    printf("Handle: 0x%08" PRIX32 "\n", cpOut.objectHandle);

    // Print first 16 bytes of EK
    printf("Public key (first 16 bytes): ");
    for (int i = 0; i < 16 && i < cpOut.outPublic.publicArea.unique.rsa.size; i++) {
        printf("%02X ", cpOut.outPublic.publicArea.unique.rsa.buffer[i]);
    }
    printf("...\n");

    // Now persist to 0x81010001
    TPM_HANDLE objectHandle = cpOut.objectHandle;
    TPMI_DH_PERSISTENT persistentHandle = 0x81010001;

    EvictControl_In evictIn;
    XMEMSET(&evictIn, 0, sizeof(evictIn));
    evictIn.auth = TPM_RH_OWNER;
    evictIn.objectHandle = objectHandle;
    evictIn.persistentHandle = persistentHandle;

    int evictRet = TPM2_EvictControl(&evictIn);
    if (evictRet == TPM_RC_SUCCESS) {
        printf("EK persisted at handle 0x%08" PRIX32 "\n", persistentHandle);
    } else {
        printf("EvictControl failed: 0x%X\n", evictRet);
    }

    return persistentHandle;
}

/*void tpm2_read_persistent_key(TPMI_DH_OBJECT handle)
{
    TPM2B_NAME name;
    TPM2B_PUBLIC outPublic;
    XMEMSET(&name, 0, sizeof(name));
    XMEMSET(&outPublic, 0, sizeof(outPublic));

    ReadPublic_In in;
    ReadPublic_Out out;
    XMEMSET(&in, 0, sizeof(in));
    XMEMSET(&out, 0, sizeof(out));

    in.objectHandle = handle;

    int ret = TPM2_ReadPublic(&in, &out);
    if (ret == TPM_RC_SUCCESS) {
        printf("Successfully read persistent key at handle 0x%08" PRIX32 "\n", (uint32_t)handle);

        uint16_t size = out.outPublic.publicArea.unique.rsa.size;
        uint8_t* buf = out.outPublic.publicArea.unique.rsa.buffer;

        printf("TPM2_ReadPublic key size: %d\n", size);

        // Base64 encode the public key
        unsigned char base64_encoded[512];
        size_t base64_len;
        

        if (mbedtls_base64_encode(base64_encoded, sizeof(base64_encoded), &base64_len, buf, size) == 0) {
            base64_encoded[base64_len] = '\0';
            printf("TPM EK (Base64):\n%s\n", base64_encoded);
        } else {
            printf("Base64 encoding failed\n");
        }

    } else {
        printf("TPM2_ReadPublic failed: 0x%X\n", ret);
    }
}*/


int marshal_tpmt_public(const TPMT_PUBLIC* pub, uint8_t* buffer, uint16_t buffer_size, uint16_t* written)
{
    if (!pub || !buffer || !written) return -1;

    uint16_t offset = 0;

    // publicArea.type (2 bytes)
    buffer[offset++] = (pub->type >> 8) & 0xFF;
    buffer[offset++] = (pub->type >> 0) & 0xFF;

    // publicArea.nameAlg (2 bytes)
    buffer[offset++] = (pub->nameAlg >> 8) & 0xFF;
    buffer[offset++] = (pub->nameAlg >> 0) & 0xFF;

    // objectAttributes (4 bytes)
    UINT32 attr = pub->objectAttributes;
    buffer[offset++] = (attr >> 24) & 0xFF;
    buffer[offset++] = (attr >> 16) & 0xFF;
    buffer[offset++] = (attr >> 8) & 0xFF;
    buffer[offset++] = (attr >> 0) & 0xFF;

    // authPolicy (2 + N)
    buffer[offset++] = (pub->authPolicy.size >> 8) & 0xFF;
    buffer[offset++] = (pub->authPolicy.size >> 0) & 0xFF;
    if (pub->authPolicy.size > 0) {
        if ((offset + pub->authPolicy.size) > buffer_size) return -2;
        XMEMCPY(&buffer[offset], pub->authPolicy.buffer, pub->authPolicy.size);
        offset += pub->authPolicy.size;
    }

    // RSA Parameters
    const TPMS_RSA_PARMS* rsaParms = &pub->parameters.rsaDetail;

    buffer[offset++] = (rsaParms->symmetric.algorithm >> 8) & 0xFF;
    buffer[offset++] = (rsaParms->symmetric.algorithm >> 0) & 0xFF;

    buffer[offset++] = (rsaParms->symmetric.keyBits.aes >> 8) & 0xFF;
    buffer[offset++] = (rsaParms->symmetric.keyBits.aes >> 0) & 0xFF;

    buffer[offset++] = (rsaParms->symmetric.mode.aes >> 8) & 0xFF;
    buffer[offset++] = (rsaParms->symmetric.mode.aes >> 0) & 0xFF;

    buffer[offset++] = (rsaParms->scheme.scheme >> 8) & 0xFF;
    buffer[offset++] = (rsaParms->scheme.scheme >> 0) & 0xFF;

    buffer[offset++] = (rsaParms->keyBits >> 8) & 0xFF;
    buffer[offset++] = (rsaParms->keyBits >> 0) & 0xFF;

    UINT32 exp = rsaParms->exponent;
    buffer[offset++] = (exp >> 24) & 0xFF;
    buffer[offset++] = (exp >> 16) & 0xFF;
    buffer[offset++] = (exp >> 8) & 0xFF;
    buffer[offset++] = (exp >> 0) & 0xFF;

    // unique.rsa.size (2 bytes) + modulus
    uint16_t mod_size = pub->unique.rsa.size;
    buffer[offset++] = (mod_size >> 8) & 0xFF;
    buffer[offset++] = (mod_size >> 0) & 0xFF;

    if ((offset + mod_size) > buffer_size) return -2;
    XMEMCPY(&buffer[offset], pub->unique.rsa.buffer, mod_size);
    offset += mod_size;

    *written = offset;
    return 0;
}

void tpm2_read_persistent_key(TPMI_DH_OBJECT handle)
{
    TPM2B_NAME name;
    TPM2B_PUBLIC outPublic;
    XMEMSET(&name, 0, sizeof(name));
    XMEMSET(&outPublic, 0, sizeof(outPublic));

    ReadPublic_In in;
    ReadPublic_Out out;
    XMEMSET(&in, 0, sizeof(in));
    XMEMSET(&out, 0, sizeof(out));

    in.objectHandle = handle;

    int ret = TPM2_ReadPublic(&in, &out);
    if (ret == TPM_RC_SUCCESS) {
        printf("Successfully read persistent key at handle 0x%08" PRIX32 "\n", (uint32_t)handle);

        // Marshal the full TPM2B_PUBLIC
        uint8_t* pub_buf = (uint8_t*)malloc(1024);
        if (!pub_buf) {
            printf("Memory allocation failed\n");
            return;
        }

        uint16_t offset = 0;
        int marshal_ret = marshal_tpmt_public(&out.outPublic.publicArea, pub_buf + 2, 1024 - 2, &offset);
        if (marshal_ret != 0) {
            printf("Failed to marshal TPM2B_PUBLIC\n");
            free(pub_buf);
            return;
        }
        
        // Add 2-byte size prefix (for TPM2B_PUBLIC)
		pub_buf[0] = (offset >> 8) & 0xFF;
		pub_buf[1] = offset & 0xFF;
		offset += 2;
		
		for (int i = 0; i < offset; i++) {
		    printf("%02X", pub_buf[i]);
		}
		printf("\n");

        // Base64 encode the marshaled buffer
        size_t base64_len = 4 * ((offset + 2 + 2) / 3) + 1; // rounded up
        unsigned char* base64_encoded = (unsigned char*)malloc(base64_len);
        if (!base64_encoded) {
            printf("Memory allocation failed for base64\n");
            free(pub_buf);
            return;
        }

        if (mbedtls_base64_encode(base64_encoded, base64_len, &base64_len, pub_buf, offset) == 0) {
            base64_encoded[base64_len] = '\0';
            printf("TPM EK (Base64):\n%s\n", base64_encoded);
        } else {
            printf("Base64 encoding failed\n");
        }

        free(pub_buf);
        free(base64_encoded);

    } else {
        printf("TPM2_ReadPublic failed: 0x%X\n", ret);
    }
}

void get_cabailities(void)
{
    int rc;

    /************ Full Self Test ************/
    XMEMSET(&cmdIn.selfTest, 0, sizeof(cmdIn.selfTest));
    cmdIn.selfTest.fullTest = YES;
    rc = TPM2_SelfTest(&cmdIn.selfTest);
    if (rc != TPM_RC_SUCCESS) {
        printf("TPM2_SelfTest failed 0x%x: %s\n", rc, TPM2_GetRCString(rc));
        return;
    }
    printf("TPM2_SelfTest passed.\n");

    /************ Get Test Result ************/
    rc = TPM2_GetTestResult(&cmdOut.tr);
    if (rc != TPM_RC_SUCCESS) {
        printf("TPM2_GetTestResult failed 0x%x: %s\n", rc, TPM2_GetRCString(rc));
        return;
    }
    printf("TPM2_GetTestResult: Size %d, Rc 0x%x\n", cmdOut.tr.outData.size, cmdOut.tr.testResult);
    TPM2_PrintBin(cmdOut.tr.outData.buffer, cmdOut.tr.outData.size);

    /************ Incremental Self Test (RSA) ************/
    XMEMSET(&cmdIn.incSelfTest, 0, sizeof(cmdIn.incSelfTest));
    cmdIn.incSelfTest.toTest.count = 1;
    cmdIn.incSelfTest.toTest.algorithms[0] = TPM_ALG_RSA;
    rc = TPM2_IncrementalSelfTest(&cmdIn.incSelfTest, &cmdOut.incSelfTest);
    printf("TPM2_IncrementalSelfTest: Rc 0x%x, Alg 0x%x (ToDo count %d)\n",
           rc, cmdIn.incSelfTest.toTest.algorithms[0],
           (int)cmdOut.incSelfTest.toDoList.count);

    /************ TPM Properties ************/
    TPM_PT properties[] = {
        TPM_PT_FAMILY_INDICATOR,
        TPM_PT_PCR_COUNT,
        TPM_PT_FIRMWARE_VERSION_1,
        TPM_PT_FIRMWARE_VERSION_2
    };
    const char* propNames[] = {
        "FAMILY_INDICATOR",
        "PCR_COUNT",
        "FIRMWARE_VERSION_1",
        "FIRMWARE_VERSION_2"
    };

    for (int i = 0; i < sizeof(properties)/sizeof(properties[0]); ++i) {
        XMEMSET(&cmdIn.cap, 0, sizeof(cmdIn.cap));
        cmdIn.cap.capability = TPM_CAP_TPM_PROPERTIES;
        cmdIn.cap.property = properties[i];
        cmdIn.cap.propertyCount = 1;

        rc = TPM2_GetCapability(&cmdIn.cap, &cmdOut.cap);
        if (rc != TPM_RC_SUCCESS) {
            printf("TPM2_GetCapability for %s failed 0x%x: %s\n", propNames[i],
                   rc, TPM2_GetRCString(rc));
            continue;
        }

        tpmProp = &cmdOut.cap.capabilityData.data.tpmProperties;
        printf("TPM2_GetCapability: Property %s = 0x%08x\n",
               propNames[i], (unsigned int)tpmProp->tpmProperty[0].value);
    }

    /************ PCR Capabilities ************/
    XMEMSET(&cmdIn.cap, 0, sizeof(cmdIn.cap));
    cmdIn.cap.capability = TPM_CAP_PCRS;
    cmdIn.cap.property = 0;
    cmdIn.cap.propertyCount = 1;
    rc = TPM2_GetCapability(&cmdIn.cap, &cmdOut.cap);
    if (rc != TPM_RC_SUCCESS) {
        printf("TPM2_GetCapability for PCRs failed 0x%x: %s\n", rc, TPM2_GetRCString(rc));
    } else {
        pcrSel = &cmdOut.cap.capabilityData.data.assignedPCR;
        printf("Assigned PCRs:\n");
        for (int i = 0; i < pcrSel->count; ++i) {
            printf("\t%s:", TPM2_GetAlgName(pcrSel->pcrSelections[i].hash));
            for (int j = 0; j < pcrSel->pcrSelections[i].sizeofSelect * 8; ++j) {
                if (pcrSel->pcrSelections[i].pcrSelect[j / 8] & (1 << (j % 8))) {
                    printf(" %d", j);
                }
            }
            printf("\n");
        }
    }

    /************ Persistent Handle Discovery ************/
    XMEMSET(&cmdIn.cap, 0, sizeof(cmdIn.cap));
    cmdIn.cap.capability = TPM_CAP_HANDLES;
    cmdIn.cap.property = TPM_HT_PERSISTENT << 24;  // 0x81000000
    cmdIn.cap.propertyCount = 20;  // Query up to 20 persistent handles

    rc = TPM2_GetCapability(&cmdIn.cap, &cmdOut.cap);
    if (rc != TPM_RC_SUCCESS) {
        printf("TPM2_GetCapability for persistent handles failed 0x%x: %s\n",
               rc, TPM2_GetRCString(rc));
    } else {
        TPML_HANDLE* handles = &cmdOut.cap.capabilityData.data.handles;
        printf("Persistent Handles Found (%lu):\n", (unsigned long)handles->count);
        for (int i = 0; i < handles->count; ++i) {
            printf(" - Handle: 0x%08" PRIX32 "\n", handles->handle[i]);
        }
    }
}

int tpm_sign_with_persistent_key(const uint8_t* data, size_t data_len, uint8_t* out_sig, uint16_t* out_sig_len)
{
    int ret = 0;
    TPM2B_DIGEST digest;
    //TPMT_SIGNATURE signature;

    // Step 1: Compute SHA-256 hash of the message to sign
    Sha256 sha;  // ✅ Correct structure for wolfSSL SHA-256
    wc_InitSha256(&sha);
    wc_Sha256Update(&sha, data, data_len);
    wc_Sha256Final(&sha, digest.buffer);
    digest.size = SHA256_DIGEST_SIZE;

    // Step 2: TPM Sign
    Sign_In in;
    Sign_Out out;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.keyHandle = TPM_PERSISTENT_HANDLE;
    memcpy(&in.digest, &digest, sizeof(digest));

    in.inScheme.scheme = TPM_ALG_RSASSA;
    in.inScheme.details.rsassa.hashAlg = TPM_ALG_SHA256;

    in.validation.tag = TPM_ST_HASHCHECK;
    in.validation.hierarchy = TPM_RH_NULL;
    in.validation.digest.size = 0;

    ret = TPM2_Sign(&in, &out);
    if (ret != TPM_RC_SUCCESS) {
        printf("TPM2_Sign failed: 0x%X\n", ret);
        return ret;
    }

    if (out.signature.sigAlg != TPM_ALG_RSASSA) {
        printf("Unexpected signature algorithm: 0x%X\n", out.signature.sigAlg);
        return -1;
    }

    *out_sig_len = out.signature.signature.rsassa.sig.size;
    memcpy(out_sig, out.signature.signature.rsassa.sig.buffer, *out_sig_len);

    printf("TPM2_Sign success. Signature length: %d\n", *out_sig_len);
    return 0;
}

void tpm_init(void)
{
	
	TPM_ESP32_I2C_Init();
	
	ESP_LOGI(TAG,  "Initializing TPM over I2C...\n");
	
	XMEMSET(&tpm, 0, sizeof(tpm));
	
	int ret = TPM2_Init(&tpm,tpm_espressif_i2c_io_cb, NULL);
    if (ret != 0) {
        printf("TPM2_Init failed: %d\n", ret);
        return;
    }
    printf("TPM2: Caps 0x%08x, Did 0x%04x, Vid 0x%04x, Rid 0x%2x \n",
        tpm.caps,
        tpm.did_vid >> 16,
        tpm.did_vid & 0xFFFF,
        tpm.rid);
    // Set the active context globally
	TPM2_SetActiveCtx(&tpm);
	
	/* define the default session auth */
    XMEMSET(session, 0, sizeof(session));
    session[0].sessionHandle = TPM_RS_PW;
    TPM2_SetSessionAuth(session);
    
    XMEMSET(&cmdIn.startup, 0, sizeof(cmdIn.startup));
    cmdIn.startup.startupType = TPM_SU_CLEAR;
    TPM2_Startup(&cmdIn.startup);
    vTaskDelay(50);                              // delay for initialize TPM chip
    ret = TPM2_Startup(&cmdIn.startup);
    if (ret != TPM_RC_SUCCESS &&
        ret != TPM_RC_INITIALIZE /* TPM_RC_INITIALIZE = Already started */ ) {
        printf("TPM2_Startup failed 0x%x: %s\n", ret, TPM2_GetRCString(ret));
        return;
        //goto exit;
    }
    
    //printf("TPM2_Startup pass\n");
	
	printf("TPM startup succeeded.\n");

	//get_cabailities();
}