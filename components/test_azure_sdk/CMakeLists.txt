idf_component_register(
    SRCS "test_azure_sdk.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES mqtt wolfTPM
    PRIV_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../azure-sdk-for-c/sdk/inc"
    				  "${CMAKE_CURRENT_LIST_DIR}/../azure-sdk-for-c/sdk/inc/azure/iot"
)
