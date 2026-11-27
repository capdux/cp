[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/rnnoise

> 版本：`0.1.1` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Xiph RNNoise recurrent neural network speech denoising and VAD for CP voice AI agents

## 安装与导入

```bash
cap add @cap/rnnoise
```

```cp
import { rnnoise_get_size } from "@cap/rnnoise";
```

## 核心导出 API

- `export fn rnnoise_get_size(): i32;`
- `export fn rnnoise_get_frame_size(): i32;`
- `export fn rnnoise_create(model: *auto!?): *auto!?;`
- `export fn rnnoise_destroy(st: *auto!?);`
- `export fn rnnoise_process_frame(st: *auto!?, out: [*]f32!, input: [*]&f32!): f32;`
- `export fn queryFrameSize(): usize;`
- `export fn frameSize(): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`rnnoise@0.1.1`)，许可证：BSD-3-Clause
