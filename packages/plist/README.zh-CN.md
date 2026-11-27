[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# plist

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Apple XML Property List (.plist) dictionary parser and serializer in pure CP

## 安装与导入

```bash
cap add plist
```

```cp
import { plstInvalid } from "plist";
```

## 核心导出 API

- `export enum PlistValKind`
- `export fn plstInvalid(): PlistValue;`
- `export fn plstEq(a: string, b: string): bool;`
- `export fn plstStartsAt(hay: string, pos: usize, needle: string): bool;`
- `export fn plstFindSub(hay: string, start: usize, needle: string): usize?;`
- `export fn plstSkipWs(s: string, start: usize): usize;`
- `export fn plstParseI64(s: string): i64;`
- `export fn plstSkipBalanced(xml: string, pos: usize, open_tag: string, close_tag: string): usize;`
- `export fn plstSkipElement(xml: string, pos: usize): usize;`
- `export fn plstParseValueAt(xml: string, pos: usize): PlistValue;`
- `export fn plistGetDictValue(xml: string, key_name: string): PlistValue;`
- `export fn plstWriteBytes(dst: []u8, pos: usize, src: string): usize;`
- `export fn plstFormatI64(val: i64, buf: []u8): usize;`
- `export fn plistWriteDictEntry(key: string, val: PlistValue, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`plist@1.0.0`)，许可证：MIT
