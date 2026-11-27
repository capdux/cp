[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/opus

> 版本：`1.5.2` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Xiph libopus 1.5.2 low-latency audio codec for voice AI agents and WebRTC in CP

## 安装与导入

```bash
cap add @cap/opus
```

```cp
import { opus_get_version_string } from "@cap/opus";
```

## 核心导出 API

- `export fn opus_get_version_string(): [*:0]&u8!?;`
- `export fn opus_encoder_create(fs: i32, channels: i32, application: i32, err_out: *i32!?): *auto!?;`
- `export fn opus_encoder_destroy(st: *auto!?);`
- `export fn opus_encode(st: *auto!?, pcm: [*]&i16!, frame_size: i32, data: [*]u8!, max_data_bytes: i32): i32;`
- `export fn opus_decoder_create(fs: i32, channels: i32, err_out: *i32!?): *auto!?;`
- `export fn opus_decoder_destroy(st: *auto!?);`
- `export fn opus_decode(st: *auto!?, data: [*]&u8!, len: i32, pcm: [*]i16!, frame_size: i32, decode_fec: i32): i32;`
- `export fn mapOpusError(code: i32): OpusError;`
- `export fn versionString(): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`opus@1.5.2`)，许可证：BSD-3-Clause
