[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rle-decode-fast

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Apple/TIFF PackBits byte run-length compression and Prometheus/Gorilla-style Delta & Delta-of-Delta time-series integer encoding for CP

## 安装与导入

```bash
cap add rle-decode-fast
```

```cp
import { encodePackBits } from "rle-decode-fast";
```

## 核心导出 API

- `export fn encodePackBits(input: string, out: []u8): string?;`
- `export fn decodePackBits(encoded: string, out: []u8): string?;`
- `export fn encodeDeltaI64(values: &[]i64, out: []i64): usize;`
- `export fn decodeDeltaI64(deltas: &[]i64, out: []i64): usize;`
- `export fn encodeDeltaOfDeltaI64(values: &[]i64, out: []i64): usize;`
- `export fn decodeDeltaOfDeltaI64(dod: &[]i64, out: []i64): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rle-decode-fast@1.0.0`)，许可证：MIT
