idf_component_register(
    SRCS "emmc.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS ""  
    REQUIRES configuration driver sdmmc vfs fatfs json
)


