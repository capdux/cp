[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# leb128

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ULEB128 (unsigned) and SLEB128 (signed) variable-length integer encoder/decoder for WebAssembly, DWARF & binary formats (compatible with Rust leb128)

## 安装与导入

```bash
cap add leb128
```

```cp
import { encodeUnsigned } from "leb128";
```

## 核心导出 API

- `export fn encodeUnsigned(val: u64, out: []u8): usize;`
- `export fn decodeUnsigned(data: &[]u8, start: usize): UlebResult;`
- `export fn encodeSigned(val: i64, out: []u8): usize;`
- `export fn decodeSigned(data: &[]u8, start: usize): SlebResult;`
- `export fn unsignedEncodedLen(val: u64): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`leb128@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`leb@1.0.0`)，许可证：MIT
