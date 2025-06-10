idf_component_register(SRCS "BLE.c"
                    INCLUDE_DIRS "include" ""
                    REQUIRES configuration bt Eth_WiFI_Mananger 
                    )
