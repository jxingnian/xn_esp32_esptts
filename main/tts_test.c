/**
 * @file tts_test.c
 * @brief TTS组件测试代码
 */

#include "xn_tts.h"
#include "esp_log.h"
#include "audio_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TTS_TEST";

/**
 * @brief TTS音频数据回调 - 将数据送入音频管理器播放
 */
static bool tts_audio_callback(const int16_t *data, int len, void *user_ctx)
{
    if (data == NULL || len == 0) {
        return true;
    }

    // 将TTS生成的音频数据送入音频管理器播放
    esp_err_t ret = audio_manager_play_audio(data, len);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to play TTS audio: %s", esp_err_to_name(ret));
        return false;
    }

    return true;
}

/**
 * @brief 初始化并测试TTS
 */
void tts_test_init_and_play(void)
{
    ESP_LOGI(TAG, "=== TTS Test Start ===");

    // 1. 配置TTS
    xn_tts_config_t config = xn_tts_get_default_config();
    config.speed = 3;  // 最快语速 (0-5, 5最快)
    config.callback = tts_audio_callback;
    config.user_ctx = NULL;

    // 2. 初始化TTS
    xn_tts_handle_t tts = xn_tts_init(&config);
    if (tts == NULL) {
        ESP_LOGE(TAG, "TTS init failed!");
        return;
    }

    ESP_LOGI(TAG, "TTS initialized successfully");

    // 3. 播放测试语音
    ESP_LOGI(TAG, "Playing test speech...");
    // 使用简单文本测试
    int ret = xn_tts_speak_chinese(tts, "你好 我是小新");
    
    if (ret == 0) {
        ESP_LOGI(TAG, "TTS test completed successfully");
    } else {
        ESP_LOGE(TAG, "TTS speak failed");
    }

    // 4. 清理（可选，如果需要长期使用可以不清理）
    // xn_tts_deinit(tts);
    
    ESP_LOGI(TAG, "=== TTS Test End ===");
}
