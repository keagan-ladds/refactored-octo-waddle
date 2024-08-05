file(GLOB SOURCES *.c)
list (APPEND SOURCES "main.c")
list (APPEND SOURCES "lora_radio.c")
list (APPEND SOURCES "sx126x.c")


idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "")
