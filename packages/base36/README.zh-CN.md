[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# base36

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Base36 and arbitrary radix 2..36 unsigned 64-bit integer encoder, overflow-safe decoder, and validator.

## 安装与导入

```bash
cap add base36
```

```cp
import { digitChar } from "base36";
```

## 核心导出 API

- `export fn digitChar(v: u8, uppercase: bool): u8;`
- `export fn digitVal(c: u8): i32;`
- `export fn encodeRadix(val: u64, radix: u8, uppercase: bool, out: []u8): usize;`
- `export fn decodeRadix(input: string, radix: u8): ParseU64Result;`
- `export fn encodeBase36(val: u64, out: []u8): usize;`
- `export fn decodeBase36(input: string): ParseU64Result;`
- `export fn isValidBase36(input: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`base36@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`radix_fmt@1.0.0`)，许可证：MIT
