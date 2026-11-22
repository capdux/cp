# `cap:aac` — 纯 CP AAC ADTS 音频帧流与 AudioSpecificConfig 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

## 概述

`cap:aac` 提供纯 CP、零外部依赖、零堆分配的 MPEG-2/MPEG-4 AAC ADTS 音频帧流解析与 ISO/IEC 14496-3 `AudioSpecificConfig` 比特流解析能力，支持 7 字节无 CRC 与 9 字节含 16-bit CRC 帧头、13 档标准采样率与声道配置查表、扩展对象类型（5-bit escape `31`）与 24-bit 显式采样率解析，以及单遍帧数/总采样数/时长估算。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `ADTS_HEADER_SIZE` / `ADTS_HEADER_CRC_SIZE` / `AAC_SAMPLES_PER_BLOCK` | 常量：`7`、`9`、`1024` |
| `AAC_SAMPLE_RATES` | ISO/IEC 14496-3 13 档标准采样率表（96000 Hz .. 7350 Hz） |
| `aacSampleRate(idx: u8): u32?` | 查询索引 `0..12` 对应的采样率（Hz），越界返回 `null` |
| `aacChannelCount(cfg: u8): u8?` | 查询声道配置 `1..6`（直通）与 `7`（8 声道 7.1），其他返回 `null` |
| `AacProfile` | AAC 档次枚举：`unknown = 0`, `main = 1`, `lc = 2`, `ssr = 3`, `ltp = 4`, `sbr = 5` |
| `aacProfileFromObjectType(object_type: u8): AacProfile` | 将 AudioObjectType 映射为 `AacProfile` |
| `isAdts(data: []const u8): bool` | 校验 12-bit 同步字 `0xFFF`、layer `00`、合法采样率索引（`< 13`）及 `frame_length` 合法且不越界 |
| `AdtsFrame` | 零拷贝 ADTS 帧视图：`is_mpeg2`, `protection_absent`, `object_type`, `profile`, `sample_rate_idx`, `sample_rate_hz`, `channel_config`, `channels`, `frame_length`, `num_raw_data_blocks`, `crc: u16?`, `payload: []const u8`；附 `samples(): u64` |
| `AdtsIter` | 零分配逐帧迭代器：`init(data)`, `next(*_): AdtsFrame?` |
| `AudioSpecificConfig` | 解析后的 ISO/IEC 14496-3 配置：`object_type`, `profile`, `sample_rate_idx`, `sample_rate_hz`, `channel_config`, `channels` |
| `parseAudioSpecificConfig(data: []const u8): AudioSpecificConfig?` | 解析标准 2 字节配置，以及 `object_type == 31`（6-bit 扩展）与 `freq_idx == 15`（24-bit 显式采样率） |
| `AdtsStats` / `summarizeAdts(data: []const u8): AdtsStats` | 单遍统计 `frame_count`, `total_samples`, `sample_rate_hz`, `channels`, `total_bytes`, `duration_ms` |

## 示例

```cp
import { isAdts, AdtsIter, parseAudioSpecificConfig, summarizeAdts } from "cap:media";

fn inspectAac(stream: []const u8, asc_bytes: []const u8): u64 {
    if (cfg := parseAudioSpecificConfig(asc_bytes)) {
        if (cfg.sample_rate_hz == 0) return 0;
    }
    if (!isAdts(stream)) return 0;
    let it = AdtsIter.init(stream);
    while (frame := it.next()) {
        if (frame.payload.len == 0) break;
    }
    const stats = summarizeAdts(stream);
    return stats.duration_ms;
}
```
