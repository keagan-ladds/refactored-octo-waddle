file(GLOB SOURCES *.c)
list (APPEND SOURCES "main.c")
list (APPEND SOURCES "lora_radio.c")
list (APPEND SOURCES "sx126x.c")
list (APPEND SOURCES "app_pwr_mgmnt.c")
list (APPEND SOURCES "loramac/loramac_serializer.c")
list (APPEND SOURCES "loramac/loramac_parser.c")
list (APPEND SOURCES "loramac/loramac_debug.c")
list (APPEND SOURCES "loramac/loramac_crypto.c")


idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS "")
