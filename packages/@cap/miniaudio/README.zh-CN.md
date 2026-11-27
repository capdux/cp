[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/miniaudio

> 版本：`0.11.21` | 许可证：`MIT-0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Cross-platform audio playback, capture, decoding (WAV/FLAC/MP3) and PCM resampling (miniaudio 0.11.21) for CP

## 安装与导入

```bash
cap add @cap/miniaudio
```

```cp
import { cp_ma_version_string } from "@cap/miniaudio";
```

## 核心导出 API

- `export fn cp_ma_version_string(): [*:0]&u8!?;`
- `export fn cp_ma_decoder_from_memory(data: [*]&u8!, size: usize, out_channels: *u32!?, out_sample_rate: *u32!?, out_frames: *u64!?): [*]f32!?;`
- `export fn cp_ma_free(ptr: *auto!?);`
- `export fn cp_ma_resampler_create(in_rate: u32, out_rate: u32, channels: u32): *auto!?;`
- `export fn cp_ma_resampler_process(resampler: *auto!?, frames_in: [*]&f32!, frame_count_in: *u64!?, frames_out: [*]f32!, frame_count_out: *u64!?): i32;`
- `export fn cp_ma_resampler_free(resampler: *auto!?);`
- `export fn versionString(): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`miniaudio@0.11.21`)，许可证：MIT-0
