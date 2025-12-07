#ifndef XN_TTS_H
#define XN_TTS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TTS实例句柄
 */
typedef void* xn_tts_handle_t;

/**
 * @brief TTS音频数据回调函数
 * 
 * @param data 音频数据指针 (16-bit PCM)
 * @param len 音频数据长度 (采样点数)
 * @param user_ctx 用户上下文
 * @return true 继续播放, false 停止播放
 */
typedef bool (*xn_tts_audio_callback_t)(const int16_t *data, int len, void *user_ctx);

/**
 * @brief TTS配置结构
 */
typedef struct {
    uint8_t speed;              /*!< 语速: 0(最慢) - 5(最快) */
    uint32_t sample_rate;       /*!< 采样率 (Hz), 默认16000 */
    xn_tts_audio_callback_t callback;  /*!< 音频数据回调 */
    void *user_ctx;             /*!< 用户上下文指针 */
} xn_tts_config_t;

/**
 * @brief 获取默认TTS配置
 * 
 * @return 默认配置
 */
xn_tts_config_t xn_tts_get_default_config(void);

/**
 * @brief 初始化TTS模块
 * 
 * @param config TTS配置
 * @return TTS句柄, NULL表示失败
 */
xn_tts_handle_t xn_tts_init(const xn_tts_config_t *config);

/**
 * @brief 合成并播放中文文本 (阻塞模式)
 * 
 * @param handle TTS句柄
 * @param text 中文文本字符串
 * @return 
 *     - 0: 成功
 *     - -1: 失败
 */
int xn_tts_speak_chinese(xn_tts_handle_t handle, const char *text);

/**
 * @brief 合成并播放拼音文本 (阻塞模式)
 * 
 * @param handle TTS句柄
 * @param pinyin 拼音字符串, 格式: "da4 jia1 hao3"
 * @return 
 *     - 0: 成功
 *     - -1: 失败
 */
int xn_tts_speak_pinyin(xn_tts_handle_t handle, const char *pinyin);

/**
 * @brief 开始异步合成中文文本
 * 
 * @param handle TTS句柄
 * @param text 中文文本字符串
 * @return 
 *     - 0: 成功
 *     - -1: 失败
 */
int xn_tts_start_chinese(xn_tts_handle_t handle, const char *text);

/**
 * @brief 开始异步合成拼音文本
 * 
 * @param handle TTS句柄
 * @param pinyin 拼音字符串
 * @return 
 *     - 0: 成功
 *     - -1: 失败
 */
int xn_tts_start_pinyin(xn_tts_handle_t handle, const char *pinyin);

/**
 * @brief 流式获取音频数据 (用于异步模式)
 * 
 * @param handle TTS句柄
 * @param data 输出音频数据指针
 * @param len 输出音频数据长度
 * @return 
 *     - 0: 成功获取数据
 *     - 1: 播放完成
 *     - -1: 失败
 */
int xn_tts_get_audio_stream(xn_tts_handle_t handle, int16_t **data, int *len);

/**
 * @brief 停止当前播放并重置
 * 
 * @param handle TTS句柄
 */
void xn_tts_stop(xn_tts_handle_t handle);

/**
 * @brief 设置语速
 * 
 * @param handle TTS句柄
 * @param speed 语速: 0(最慢) - 5(最快)
 */
void xn_tts_set_speed(xn_tts_handle_t handle, uint8_t speed);

/**
 * @brief 获取当前语速
 * 
 * @param handle TTS句柄
 * @return 当前语速
 */
uint8_t xn_tts_get_speed(xn_tts_handle_t handle);

/**
 * @brief 反初始化TTS模块
 * 
 * @param handle TTS句柄
 */
void xn_tts_deinit(xn_tts_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // XN_TTS_H
