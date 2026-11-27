[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# iconv-lite

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation character encoding converter supporting UTF-8, ASCII, Latin1, Windows-1252, UTF-16LE/BE with BOM and surrogate pairs for CP

## 安装与导入

```bash
cap add iconv-lite
```

```cp
import { icvStrEq } from "iconv-lite";
```

## 核心导出 API

- `export enum Charset`
- `export fn icvStrEq(a: string, b: string): bool;`
- `export fn parseCharset(name: string): Charset;`
- `export fn encodingExists(name: string): bool;`
- `export fn icvWin1252ToUnicode(b: u8): u32;`
- `export fn icvUnicodeToWin1252(cp: u32): u8?;`
- `export fn icvWriteUtf8(out: []u8, pos: usize, cp: u32): usize;`
- `export fn icvReadUtf8(s: string, pos: usize): IcvUtf8Step;`
- `export fn icvReadU16(data: &[]u8, pos: usize, is_le: bool): u16;`
- `export fn icvWriteU16(out: []u8, pos: usize, val: u16, is_le: bool): usize;`
- `export fn decodeToUtf8(data: &[]u8, charset: Charset, strip_bom: bool, out: []u8): usize;`
- `export fn encodeFromUtf8(utf8: string, charset: Charset, add_bom: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`iconv-lite@1.0.0`)，许可证：MIT
