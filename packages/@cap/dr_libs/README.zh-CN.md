[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/dr_libs

> 版本：`0.13.17` | 许可证：`MIT-0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Single-header zero-dependency WAV, MP3, and FLAC audio decoders/encoders (dr_libs) for CP

## 安装与导入

```bash
cap add @cap/dr_libs
```

```cp
import { cp_drwav_encode_pcm16_mem } from "@cap/dr_libs";
```

## 核心导出 API

- `export fn cp_drwav_encode_pcm16_mem(pcm: [*]&i16!, frame_count: u64, channels: u32, sample_rate: u32, out_size: *usize!): [*]u8!?;`
- `export fn cp_drwav_decode_pcm16_mem(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?;`
- `export fn cp_drflac_decode_pcm16_mem(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?;`
- `export fn cp_dr_free(ptr: *auto!?);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`dr_libs@0.13.17`)，许可证：MIT-0
