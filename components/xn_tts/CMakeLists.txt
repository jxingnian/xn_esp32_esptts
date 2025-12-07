idf_component_register(
    SRCS 
        "src/xn_tts.c"
    INCLUDE_DIRS 
        "include"
        "esp_tts"
    PRIV_REQUIRES
        freertos
)

# 添加ESP-TTS库路径
target_link_directories(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/esp_tts")

# 链接ESP-TTS库
target_link_libraries(${COMPONENT_LIB} INTERFACE
    "-Wl,--start-group"
    esp_tts_chinese
    voice_set_xiaole
    "-Wl,--end-group"
)
