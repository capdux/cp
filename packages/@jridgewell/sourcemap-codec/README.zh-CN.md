[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @jridgewell/sourcemap-codec

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Source Map v3 Base64 VLQ (Variable-Length Quantity) signed integer sequence encoder and decoder.

## 安装与导入

```bash
cap add @jridgewell/sourcemap-codec
```

```cp
import { b64Char } from "@jridgewell/sourcemap-codec";
```

## 核心导出 API

- `export fn b64Char(v: u8): u8;`
- `export fn b64Val(c: u8): i32;`
- `export fn encodeOne(val: i64, out: []u8): usize;`
- `export fn decodeOne(input: string, start: usize): DecodeResult;`
- `export fn encodeSlice(values: &[]i64, out: []u8): usize;`
- `export fn decodeSlice(input: string, out: []i64): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`@jridgewell/sourcemap-codec@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`vlq@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`vlq@1.0.0`)，许可证：MIT
