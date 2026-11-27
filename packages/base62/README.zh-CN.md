[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# base62

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap URL-safe Base62 (0-9A-Za-z) u64 and arbitrary byte-slice encoder/decoder in pure CP

## 安装与导入

```bash
cap add base62
```

```cp
import { charToVal } from "base62";
```

## 核心导出 API

- `export fn charToVal(c: u8): i32;`
- `export fn isValidBase62(src: &[]u8): bool;`
- `export fn encodeU64(value: u64, dst: []u8): usize;`
- `export fn encodeU64Padded(value: u64, width: usize, dst: []u8): usize;`
- `export fn decodeU64(src: &[]u8): DecodeU64Result;`
- `export fn encodeBytes(src: &[]u8, dst: []u8): i64;`
- `export fn decodeBytes(src: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`base62@1.0.0`)，许可证：MIT
