[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# crockford-base32

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Douglas Crockford Base32 encoder/decoder for 64-bit integers (with optional mod-37 check symbol), arbitrary byte streams, and grouped formatting in CP.

## 安装与导入

```bash
cap add crockford-base32
```

```cp
import { encodeU64 } from "crockford-base32";
```

## 核心导出 API

- `export fn encodeU64(val: u64, with_check: bool, out: []u8): usize;`
- `export fn decodeU64(input: string, with_check: bool, out_val: &mut u64): bool;`
- `export fn encodeBytes(data: &[]u8, out: []u8): usize;`
- `export fn decodeBytes(input: string, out: []u8): i64;`
- `export fn formatGrouped(raw: string, group_size: usize, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`crockford-base32@1.0.0`)，许可证：MIT
