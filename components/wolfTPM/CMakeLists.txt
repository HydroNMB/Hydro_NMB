idf_component_register(
    SRCS
        "src/tpm2.c"
        "src/tpm2_wrap.c"
        "src/tpm2_packet.c"
        "src/tpm2_param_enc.c"
        "src/tpm2_tis.c"
        "src/tpm2_swtpm.c"
        "src/tpm2_cryptocb.c"
        "src/tpm2_asn.c"
        "src/tpm_io_espressif.c"   
    INCLUDE_DIRS
        "include" "include/wolfTPM" 
        PRIV_REQUIRES
        REQUIRES configuration wolfssl driver mbedtls
)
