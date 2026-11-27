[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# properties-reader

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Java / Spring Boot / i18n .properties parser and formatter with =/:/space separators, backslash line continuations, comments, and \uXXXX Unicode escapes.

## 安装与导入

```bash
cap add properties-reader
```

```cp
import { isLineWs } from "properties-reader";
```

## 核心导出 API

- `export fn isLineWs(c: u8): bool;`
- `export fn fromHex(c: u8): i32;`
- `export fn bytesEq(a: &[]u8, b: &[]u8): bool;`
- `export fn decodeEscapeInto(src: &[]u8, esc_pos: usize, dst: []u8, dst_pos: usize): EscDecodeResult;`
- `export fn pushByte(dst: []u8, pos: usize, b: u8): i64;`
- `export fn pushEscaped(dst: []u8, pos: usize, ch: u8): i64;`
- `export fn formatEntry(key: &[]u8, value: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`properties-reader@1.0.0`)，许可证：MIT
