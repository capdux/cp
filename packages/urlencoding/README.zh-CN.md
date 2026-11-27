[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# urlencoding

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 3986 percent-encoding and decoding in pure CP (modeled after Rust urlencoding v2)

## 安装与导入

```bash
cap add urlencoding
```

```cp
import { urlencHexUpper } from "urlencoding";
```

## 核心导出 API

- `export fn urlencHexUpper(nibble: u8): u8;`
- `export fn urlencHexVal(ch: u8): i16;`
- `export fn isUnreserved(b: u8): bool;`
- `export fn encodedLen(input: string): usize;`
- `export fn encodeBinary(data: &[]u8, out: []u8): usize;`
- `export fn encode(input: string, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`
- `export fn decodeLossy(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`urlencoding@1.0.0`)，许可证：MIT
