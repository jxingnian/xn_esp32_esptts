#include "xn_tts.h"
#include "esp_tts.h"
#include "esp_tts_voice_xiaole.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "XN_TTS";

/**
 * @brief TTS内部上下文结构
 */
typedef struct {
    esp_tts_handle_t tts_handle;
    esp_tts_voice_t *voice;
    xn_tts_config_t config;
    bool is_playing;
} xn_tts_context_t;

xn_tts_config_t xn_tts_get_default_config(void)
{
    xn_tts_config_t config = {
        .speed = 0,
        .sample_rate = 16000,
        .callback = NULL,
        .user_ctx = NULL,
    };
    return config;
}

xn_tts_handle_t xn_tts_init(const xn_tts_config_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return NULL;
    }

    // 分配上下文
    xn_tts_context_t *ctx = (xn_tts_context_t *)calloc(1, sizeof(xn_tts_context_t));
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate context");
        return NULL;
    }

    // 保存配置
    memcpy(&ctx->config, config, sizeof(xn_tts_config_t));

    // 初始化语音集
    ctx->voice = esp_tts_voice_set_init(&esp_tts_voice_xiaole, NULL);
    if (ctx->voice == NULL) {
        ESP_LOGE(TAG, "Failed to init voice set");
        free(ctx);
        return NULL;
    }

    // 创建TTS实例
    ctx->tts_handle = esp_tts_create(ctx->voice);
    if (ctx->tts_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create TTS");
        esp_tts_voice_set_free(ctx->voice);
        free(ctx);
        return NULL;
    }

    ctx->is_playing = false;
    ESP_LOGI(TAG, "TTS initialized successfully");
    return (xn_tts_handle_t)ctx;
}

int xn_tts_speak_chinese(xn_tts_handle_t handle, const char *text)
{
    if (handle == NULL || text == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 解析中文文本
    if (esp_tts_parse_chinese(ctx->tts_handle, text) != 1) {
        ESP_LOGE(TAG, "Failed to parse chinese text");
        return -1;
    }

    ctx->is_playing = true;
    ESP_LOGI(TAG, "Start speaking: %s", text);

    // 流式播放
    int len = 0;
    while (ctx->is_playing) {
        short *data = esp_tts_stream_play(ctx->tts_handle, &len, ctx->config.speed);
        
        if (len == 0) {
            // 播放完成
            ESP_LOGI(TAG, "Speaking completed");
            break;
        }

        if (data != NULL && len > 0) {
            // 调用回调函数
            if (ctx->config.callback != NULL) {
                bool continue_play = ctx->config.callback(data, len, ctx->config.user_ctx);
                if (!continue_play) {
                    ESP_LOGI(TAG, "Speaking stopped by callback");
                    break;
                }
            }
        }

        // 让出CPU
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    ctx->is_playing = false;
    esp_tts_stream_reset(ctx->tts_handle);
    return 0;
}

int xn_tts_speak_pinyin(xn_tts_handle_t handle, const char *pinyin)
{
    if (handle == NULL || pinyin == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 解析拼音文本
    if (esp_tts_parse_pinyin(ctx->tts_handle, pinyin) != 1) {
        ESP_LOGE(TAG, "Failed to parse pinyin text");
        return -1;
    }

    ctx->is_playing = true;
    ESP_LOGI(TAG, "Start speaking pinyin: %s", pinyin);

    // 流式播放
    int len = 0;
    while (ctx->is_playing) {
        short *data = esp_tts_stream_play(ctx->tts_handle, &len, ctx->config.speed);
        
        if (len == 0) {
            ESP_LOGI(TAG, "Speaking completed");
            break;
        }

        if (data != NULL && len > 0) {
            if (ctx->config.callback != NULL) {
                bool continue_play = ctx->config.callback(data, len, ctx->config.user_ctx);
                if (!continue_play) {
                    ESP_LOGI(TAG, "Speaking stopped by callback");
                    break;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    ctx->is_playing = false;
    esp_tts_stream_reset(ctx->tts_handle);
    return 0;
}

int xn_tts_start_chinese(xn_tts_handle_t handle, const char *text)
{
    if (handle == NULL || text == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 停止当前播放
    if (ctx->is_playing) {
        xn_tts_stop(handle);
    }

    // 解析中文文本
    if (esp_tts_parse_chinese(ctx->tts_handle, text) != 1) {
        ESP_LOGE(TAG, "Failed to parse chinese text");
        return -1;
    }

    ctx->is_playing = true;
    ESP_LOGI(TAG, "TTS started for: %s", text);
    return 0;
}

int xn_tts_start_pinyin(xn_tts_handle_t handle, const char *pinyin)
{
    if (handle == NULL || pinyin == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 停止当前播放
    if (ctx->is_playing) {
        xn_tts_stop(handle);
    }

    // 解析拼音文本
    if (esp_tts_parse_pinyin(ctx->tts_handle, pinyin) != 1) {
        ESP_LOGE(TAG, "Failed to parse pinyin text");
        return -1;
    }

    ctx->is_playing = true;
    ESP_LOGI(TAG, "TTS started for pinyin: %s", pinyin);
    return 0;
}

int xn_tts_get_audio_stream(xn_tts_handle_t handle, int16_t **data, int *len)
{
    if (handle == NULL || data == NULL || len == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    if (!ctx->is_playing) {
        *data = NULL;
        *len = 0;
        return 1; // 已完成
    }

    // 获取音频流
    short *audio_data = esp_tts_stream_play(ctx->tts_handle, len, ctx->config.speed);
    
    if (*len == 0) {
        // 播放完成
        ctx->is_playing = false;
        *data = NULL;
        return 1;
    }

    *data = audio_data;
    return 0; // 成功获取数据
}

void xn_tts_stop(xn_tts_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;
    ctx->is_playing = false;
    esp_tts_stream_reset(ctx->tts_handle);
    ESP_LOGI(TAG, "TTS stopped");
}

void xn_tts_set_speed(xn_tts_handle_t handle, uint8_t speed)
{
    if (handle == NULL) {
        return;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;
    
    if (speed > 5) {
        speed = 5;
    }
    
    ctx->config.speed = speed;
    ESP_LOGI(TAG, "Speed set to %d", speed);
}

uint8_t xn_tts_get_speed(xn_tts_handle_t handle)
{
    if (handle == NULL) {
        return 0;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;
    return ctx->config.speed;
}

void xn_tts_deinit(xn_tts_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 停止播放
    xn_tts_stop(handle);

    // 销毁TTS实例
    if (ctx->tts_handle != NULL) {
        esp_tts_destroy(ctx->tts_handle);
    }

    // 释放语音集
    if (ctx->voice != NULL) {
        esp_tts_voice_set_free(ctx->voice);
    }

    // 释放上下文
    free(ctx);
    ESP_LOGI(TAG, "TTS deinitialized");
}
