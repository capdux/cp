[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# utf16

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap UTF-8 <-> UTF-16 / UTF-16LE codec with full surrogate pair support, lone-surrogate rejection, BOM detection, and UTF-16 unit counting.

## 安装与导入

```bash
cap add utf16
```

```cp
import { detectBom } from "utf16";
```

## 核心导出 API

- `export fn detectBom(data: &[]u8): u8;`
- `export fn decodeUtf8Next(s: string, pos: usize, out_cp: &mut u32): i64;`
- `export fn encodeUtf8Scalar(cp: u32, out: []u8, pos: usize): i64;`
- `export fn countUtf16Units(utf8: string): i64;`
- `export fn utf8ToUtf16(utf8: string, out: []u16): i64;`
- `export fn utf16ToUtf8(utf16: &[]u16, out: []u8): i64;`
- `export fn utf8ToUtf16LeBytes(utf8: string, out: []u8): i64;`
- `export fn utf16LeBytesToUtf8(bytes: &[]u8, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`utf16@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`widestring@1.0.0`)，许可证：MIT
