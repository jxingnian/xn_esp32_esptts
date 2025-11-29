idf_component_register(SRCS "main.c" "coze_chat_app/coze_chat_app.c"
                       PRIV_REQUIRES xn_web_wifi_manger xn_coze_chat esp_timer
                       INCLUDE_DIRS "." "coze_chat_app")
