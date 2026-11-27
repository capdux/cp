[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/flac

> 版本：`1.4.3` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Xiph libFLAC 1.4.3 lossless audio encoder and decoder engine for CP

## 安装与导入

```bash
cap add @cap/flac
```

```cp
import { cp_flac_version } from "@cap/flac";
```

## 核心导出 API

- `export fn cp_flac_version(): [*:0]&u8!?;`
- `export fn cp_flac_vendor(): [*:0]&u8!?;`
- `export fn cp_flac_encode_pcm16_test(pcm: [*]&i16!, samples: u32, sample_rate: u32, out_bytes_written: *u64!): i32;`
- `export fn version(): string;`
- `export fn vendor(): string;`
- `export fn encodePcm16Test(pcm: &[]i16, sample_rate: u32): !u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`flac@1.4.3`)，许可证：BSD-3-Clause
