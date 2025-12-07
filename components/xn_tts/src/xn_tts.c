#include "xn_tts.h"
#include "esp_tts.h"
#include "esp_tts_voice_xiaoxin.h"
#include "esp_tts_voice_template.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// 声明嵌入的语音数据文件
// 可选音色: xiaole (小乐), xiaoxin (小新), xiaoxin_small (小新精简版)
extern const uint8_t voice_data_xiaoxin_start[] asm("_binary_esp_tts_voice_data_xiaoxin_dat_start");
extern const uint8_t voice_data_xiaoxin_end[] asm("_binary_esp_tts_voice_data_xiaoxin_dat_end");

// 日志标签
static const char *TAG = "XN_TTS";

/**
 * @brief TTS内部上下文结构体
 * 
 * 包含TTS实例的所有运行时信息
 */
typedef struct {
    esp_tts_handle_t tts_handle;    // ESP-TTS句柄
    esp_tts_voice_t *voice;         // 语音数据指针
    xn_tts_config_t config;         // TTS配置参数
    bool is_playing;                // 播放状态标志
} xn_tts_context_t;

/**
 * @brief 获取默认TTS配置
 * 
 * @return 默认配置结构体
 */
xn_tts_config_t xn_tts_get_default_config(void)
{
    xn_tts_config_t config = {
        .speed = 0,                 // 默认语速（正常速度）
        .sample_rate = 16000,       // 采样率16kHz
        .callback = NULL,           // 无音频回调
        .user_ctx = NULL,           // 无用户上下文
    };
    return config;
}

/**
 * @brief 初始化TTS实例
 * 
 * @param config TTS配置参数，不能为NULL
 * @return TTS句柄，失败时返回NULL
 */
xn_tts_handle_t xn_tts_init(const xn_tts_config_t *config)
{
    // 参数检查
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return NULL;
    }

    // 分配TTS上下文内存
    xn_tts_context_t *ctx = (xn_tts_context_t *)calloc(1, sizeof(xn_tts_context_t));
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate context");
        return NULL;
    }

    // 保存用户配置到上下文
    memcpy(&ctx->config, config, sizeof(xn_tts_config_t));

    // 获取嵌入的语音数据
    size_t voice_data_size = voice_data_xiaoxin_end - voice_data_xiaoxin_start;
    ESP_LOGI(TAG, "Voice data size: %d bytes", voice_data_size);

    // 使用模板和数据初始化语音集
    ctx->voice = esp_tts_voice_set_init(&esp_tts_voice_xiaoxin, (void *)voice_data_xiaoxin_start);
    if (ctx->voice == NULL) {
        ESP_LOGE(TAG, "Failed to init voice set");
        free(ctx);
        return NULL;
    }

    ESP_LOGI(TAG, "Voice set initialized: %s", ctx->voice->voice_name);
    
    // 创建ESP-TTS实例
    ctx->tts_handle = esp_tts_create(ctx->voice);
    if (ctx->tts_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create TTS");
        esp_tts_voice_set_free(ctx->voice);
        free(ctx);
        return NULL;
    }

    // 初始化播放状态
    ctx->is_playing = false;
    ESP_LOGI(TAG, "TTS initialized successfully");
    return (xn_tts_handle_t)ctx;
}

/**
 * @brief 同步播放中文文本
 * 
 * 此函数会阻塞直到播放完成或被停止
 * 
 * @param handle TTS句柄
 * @param text 要播放的中文文本
 * @return 0成功，-1失败
 */
int xn_tts_speak_chinese(xn_tts_handle_t handle, const char *text)
{
    // 参数验证
    if (handle == NULL || text == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 解析中文文本为音素序列
    if (esp_tts_parse_chinese(ctx->tts_handle, text) != 1) {
        ESP_LOGE(TAG, "Failed to parse chinese text");
        return -1;
    }

    // 设置播放状态
    ctx->is_playing = true;
    ESP_LOGI(TAG, "Start speaking: %s", text);

    // 流式播放循环
    int len = 0;
    while (ctx->is_playing) {
        // 获取音频数据流
        short *data = esp_tts_stream_play(ctx->tts_handle, &len, ctx->config.speed);
        
        if (len == 0) {
            // 音频数据播放完成
            ESP_LOGI(TAG, "Speaking completed");
            break;
        }

        if (data != NULL && len > 0) {
            // 调用用户音频回调函数
            if (ctx->config.callback != NULL) {
                bool continue_play = ctx->config.callback(data, len, ctx->config.user_ctx);
                if (!continue_play) {
                    ESP_LOGI(TAG, "Speaking stopped by callback");
                    break;
                }
            }
        }

        // 让出CPU时间片，避免占用过多CPU
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // 清理播放状态
    ctx->is_playing = false;
    esp_tts_stream_reset(ctx->tts_handle);
    return 0;
}

/**
 * @brief 同步播放拼音文本
 * 
 * 此函数会阻塞直到播放完成或被停止
 * 
 * @param handle TTS句柄
 * @param pinyin 拼音字符串，如"ni3 hao3"
 * @return 0成功，-1失败
 */
int xn_tts_speak_pinyin(xn_tts_handle_t handle, const char *pinyin)
{
    // 参数验证
    if (handle == NULL || pinyin == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 解析拼音文本为音素序列
    if (esp_tts_parse_pinyin(ctx->tts_handle, pinyin) != 1) {
        ESP_LOGE(TAG, "Failed to parse pinyin text");
        return -1;
    }

    // 设置播放状态
    ctx->is_playing = true;
    ESP_LOGI(TAG, "Start speaking pinyin: %s", pinyin);

    // 流式播放循环
    int len = 0;
    while (ctx->is_playing) {
        // 获取音频数据流
        short *data = esp_tts_stream_play(ctx->tts_handle, &len, ctx->config.speed);
        
        if (len == 0) {
            // 音频数据播放完成
            ESP_LOGI(TAG, "Speaking completed");
            break;
        }

        if (data != NULL && len > 0) {
            // 调用用户音频回调函数
            if (ctx->config.callback != NULL) {
                bool continue_play = ctx->config.callback(data, len, ctx->config.user_ctx);
                if (!continue_play) {
                    ESP_LOGI(TAG, "Speaking stopped by callback");
                    break;
                }
            }
        }

        // 让出CPU时间片
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // 清理播放状态
    ctx->is_playing = false;
    esp_tts_stream_reset(ctx->tts_handle);
    return 0;
}

/**
 * @brief 启动中文文本的异步播放
 * 
 * 此函数立即返回，不等待播放完成
 * 需要配合xn_tts_get_audio_stream获取音频数据
 * 
 * @param handle TTS句柄
 * @param text 要播放的中文文本
 * @return 0成功，-1失败
 */
int xn_tts_start_chinese(xn_tts_handle_t handle, const char *text)
{
    // 参数验证
    if (handle == NULL || text == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 如果正在播放，先停止当前播放
    if (ctx->is_playing) {
        xn_tts_stop(handle);
    }

    // 解析中文文本为音素序列
    if (esp_tts_parse_chinese(ctx->tts_handle, text) != 1) {
        ESP_LOGE(TAG, "Failed to parse chinese text");
        return -1;
    }

    // 设置播放状态为开始
    ctx->is_playing = true;
    ESP_LOGI(TAG, "TTS started for: %s", text);
    return 0;
}

/**
 * @brief 启动拼音文本的异步播放
 * 
 * 此函数立即返回，不等待播放完成
 * 需要配合xn_tts_get_audio_stream获取音频数据
 * 
 * @param handle TTS句柄
 * @param pinyin 拼音字符串，如"ni3 hao3"
 * @return 0成功，-1失败
 */
int xn_tts_start_pinyin(xn_tts_handle_t handle, const char *pinyin)
{
    // 参数验证
    if (handle == NULL || pinyin == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 如果正在播放，先停止当前播放
    if (ctx->is_playing) {
        xn_tts_stop(handle);
    }

    // 解析拼音文本为音素序列
    if (esp_tts_parse_pinyin(ctx->tts_handle, pinyin) != 1) {
        ESP_LOGE(TAG, "Failed to parse pinyin text");
        return -1;
    }

    // 设置播放状态为开始
    ctx->is_playing = true;
    ESP_LOGI(TAG, "TTS started for pinyin: %s", pinyin);
    return 0;
}

/**
 * @brief 获取音频数据流
 * 
 * 用于异步模式下获取音频数据，需要先调用start函数
 * 
 * @param handle TTS句柄
 * @param data 输出音频数据指针
 * @param len 输出音频数据长度（采样点数）
 * @return 0有数据，1播放完成，-1错误
 */
int xn_tts_get_audio_stream(xn_tts_handle_t handle, int16_t **data, int *len)
{
    // 参数验证
    if (handle == NULL || data == NULL || len == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 检查播放状态
    if (!ctx->is_playing) {
        *data = NULL;
        *len = 0;
        return 1; // 播放已完成或未开始
    }

    // 从TTS引擎获取音频流
    short *audio_data = esp_tts_stream_play(ctx->tts_handle, len, ctx->config.speed);
    
    if (*len == 0) {
        // 音频数据生成完成
        ctx->is_playing = false;
        *data = NULL;
        return 1;
    }

    // 返回音频数据
    *data = audio_data;
    return 0; // 成功获取数据
}

/**
 * @brief 停止TTS播放
 * 
 * @param handle TTS句柄
 */
void xn_tts_stop(xn_tts_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;
    
    // 停止播放并重置TTS流
    ctx->is_playing = false;
    esp_tts_stream_reset(ctx->tts_handle);
    ESP_LOGI(TAG, "TTS stopped");
}

/**
 * @brief 设置TTS播放速度
 * 
 * @param handle TTS句柄
 * @param speed 播放速度（0-5），0为正常速度，5为最快
 */
void xn_tts_set_speed(xn_tts_handle_t handle, uint8_t speed)
{
    if (handle == NULL) {
        return;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;
    
    // 限制速度范围
    if (speed > 5) {
        speed = 5;
    }
    
    // 保存速度设置
    ctx->config.speed = speed;
    ESP_LOGI(TAG, "Speed set to %d", speed);
}

/**
 * @brief 获取当前TTS播放速度
 * 
 * @param handle TTS句柄
 * @return 当前播放速度
 */
uint8_t xn_tts_get_speed(xn_tts_handle_t handle)
{
    if (handle == NULL) {
        return 0;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;
    return ctx->config.speed;
}

/**
 * @brief 销毁TTS实例并释放资源
 * 
 * @param handle TTS句柄
 */
void xn_tts_deinit(xn_tts_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

    xn_tts_context_t *ctx = (xn_tts_context_t *)handle;

    // 停止当前播放
    xn_tts_stop(handle);

    // 销毁ESP-TTS实例
    if (ctx->tts_handle != NULL) {
        esp_tts_destroy(ctx->tts_handle);
    }

    // 释放语音集
    if (ctx->voice != NULL) {
        esp_tts_voice_set_free(ctx->voice);
    }

    // 释放上下文内存
    free(ctx);
    ESP_LOGI(TAG, "TTS deinitialized");
}
