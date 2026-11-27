[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/speexdsp

> 版本：`1.2.1` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Xiph SpeexDSP 1.2.1 real-time acoustic echo cancellation (AEC), noise suppression, AGC, and VAD for CP

## 安装与导入

```bash
cap add @cap/speexdsp
```

```cp
import { speex_echo_state_init } from "@cap/speexdsp";
```

## 核心导出 API

- `export fn speex_echo_state_init(frame_size: i32, filter_length: i32): *auto!?;`
- `export fn speex_echo_state_destroy(st: *auto!?);`
- `export fn speex_echo_cancellation(st: *auto!?, rec: [*]&i16!, play: [*]&i16!, out: [*]i16!);`
- `export fn speex_echo_state_reset(st: *auto!?);`
- `export fn speex_echo_ctl(st: *auto!?, request: i32, ptr: *auto!?): i32;`
- `export fn speex_preprocess_state_init(frame_size: i32, sampling_rate: i32): *auto!?;`
- `export fn speex_preprocess_state_destroy(st: *auto!?);`
- `export fn speex_preprocess_run(st: *auto!?, x: [*]i16!): i32;`
- `export fn speex_preprocess_ctl(st: *auto!?, request: i32, ptr: *auto!?): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`speexdsp@1.2.1`)，许可证：BSD-3-Clause
