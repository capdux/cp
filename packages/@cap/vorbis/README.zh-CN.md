[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/vorbis

> 版本：`1.3.7` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Xiph libvorbis 1.3.7 royalty-free audio codec (self-contained with libogg) for CP

## 安装与导入

```bash
cap add @cap/vorbis
```

```cp
import { cp_vorbis_version_string } from "@cap/vorbis";
```

## 核心导出 API

- `export fn cp_vorbis_version_string(): [*:0]&u8!;`
- `export fn cp_vorbis_encoder_test_init(channels: i32, rate: isize, quality: f32): i32;`
- `export fn versionString(): string;`
- `export fn encoderTestInit(channels: i32, rate: isize, quality: f32): !void;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`vorbis@1.3.7`)，许可证：BSD-3-Clause
