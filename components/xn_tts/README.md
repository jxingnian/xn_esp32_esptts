# XN TTS 组件

基于 ESP-TTS 的中文语音合成组件，提供简洁易用的 API 接口。

## 功能特性

- ✅ 支持中文文本转语音
- ✅ 支持拼音文本转语音
- ✅ 实时流式音频输出
- ✅ 可调节语速 (0-5级)
- ✅ 同步/异步播放模式
- ✅ 音频数据回调接口
- ✅ 基于 ESP32-S3 优化

## API 使用示例

### 1. 基础使用 (阻塞模式)

```c
#include "xn_tts.h"

// 音频数据回调函数
bool audio_callback(const int16_t *data, int len, void *user_ctx)
{
    // 将音频数据发送到 I2S 或其他音频输出设备
    // data: 16-bit PCM 音频数据
    // len: 采样点数量
    
    // 返回 true 继续播放，false 停止播放
    return true;
}

void app_main(void)
{
    // 1. 获取默认配置
    xn_tts_config_t config = xn_tts_get_default_config();
    config.speed = 0;  // 语速: 0(最慢) - 5(最快)
    config.callback = audio_callback;
    config.user_ctx = NULL;
    
    // 2. 初始化 TTS
    xn_tts_handle_t tts = xn_tts_init(&config);
    if (tts == NULL) {
        printf("TTS init failed\n");
        return;
    }
    
    // 3. 播放中文文本 (阻塞)
    xn_tts_speak_chinese(tts, "你好世界");
    
    // 4. 播放拼音文本 (阻塞)
    xn_tts_speak_pinyin(tts, "ni3 hao3 shi4 jie4");
    
    // 5. 反初始化
    xn_tts_deinit(tts);
}
```

### 2. 异步模式 (流式获取)

```c
void tts_task(void *arg)
{
    xn_tts_handle_t tts = (xn_tts_handle_t)arg;
    
    // 开始异步合成
    xn_tts_start_chinese(tts, "这是异步播放模式");
    
    // 循环获取音频流
    int16_t *data;
    int len;
    
    while (1) {
        int ret = xn_tts_get_audio_stream(tts, &data, &len);
        
        if (ret == 1) {
            // 播放完成
            break;
        } else if (ret == 0 && data != NULL) {
            // 处理音频数据
            // 例如: i2s_write(data, len * sizeof(int16_t));
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    xn_tts_config_t config = xn_tts_get_default_config();
    xn_tts_handle_t tts = xn_tts_init(&config);
    
    // 创建任务进行异步播放
    xTaskCreate(tts_task, "tts_task", 4096, tts, 5, NULL);
}
```

### 3. 动态控制

```c
// 设置语速
xn_tts_set_speed(tts, 3);  // 0-5

// 获取当前语速
uint8_t speed = xn_tts_get_speed(tts);

// 停止当前播放
xn_tts_stop(tts);
```

## API 参考

### 初始化与配置

- `xn_tts_config_t xn_tts_get_default_config(void)` - 获取默认配置
- `xn_tts_handle_t xn_tts_init(const xn_tts_config_t *config)` - 初始化 TTS
- `void xn_tts_deinit(xn_tts_handle_t handle)` - 反初始化

### 同步播放

- `int xn_tts_speak_chinese(xn_tts_handle_t handle, const char *text)` - 播放中文
- `int xn_tts_speak_pinyin(xn_tts_handle_t handle, const char *pinyin)` - 播放拼音

### 异步播放

- `int xn_tts_start_chinese(xn_tts_handle_t handle, const char *text)` - 开始合成中文
- `int xn_tts_start_pinyin(xn_tts_handle_t handle, const char *pinyin)` - 开始合成拼音
- `int xn_tts_get_audio_stream(xn_tts_handle_t handle, int16_t **data, int *len)` - 获取音频流

### 控制接口

- `void xn_tts_stop(xn_tts_handle_t handle)` - 停止播放
- `void xn_tts_set_speed(xn_tts_handle_t handle, uint8_t speed)` - 设置语速
- `uint8_t xn_tts_get_speed(xn_tts_handle_t handle)` - 获取语速

## 音频参数

- **采样率**: 16000 Hz
- **位宽**: 16-bit
- **声道**: 单声道 (Mono)
- **格式**: PCM
- **音色**: xiaole (小乐)

## 注意事项

1. **内存要求**: TTS 需要较大内存，建议在 ESP32-S3 上使用
2. **回调函数**: 音频回调在 TTS 线程中执行，避免阻塞操作
3. **线程安全**: 同一个 handle 不支持多线程同时调用
4. **语速范围**: 0(最慢) - 5(最快)，超出范围会自动限制

## 依赖

- ESP-IDF v4.4+
- ESP32-S3 芯片
- FreeRTOS

## 许可证

基于 ESP-TTS (Apache License 2.0)
