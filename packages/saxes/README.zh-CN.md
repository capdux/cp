[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# saxes

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation streaming SAX XML 1.0 event parser with tag stack validation and entity decoding (npm saxes parity) for CP

## 安装与导入

```bash
cap add saxes
```

```cp
import { saxEmptyAttrs } from "saxes";
```

## 核心导出 API

- `export enum XmlEventKind`
- `export fn saxEmptyAttrs(): [8]XmlAttr;`
- `export fn saxMakeEvent(kind: XmlEventKind, name: string, text: string): XmlEvent;`
- `export fn saxIsWs(ch: u8): bool;`
- `export fn saxIsNameStart(ch: u8): bool;`
- `export fn saxIsNameChar(ch: u8): bool;`
- `export fn saxStrEq(a: string, b: string): bool;`
- `export fn saxStartsWithAt(s: string, pos: usize, prefix: string): bool;`
- `export fn saxFindSubstrFrom(s: string, start: usize, needle: string): i64;`
- `export fn saxAllWs(s: string): bool;`
- `export fn saxEmit(out: []XmlEvent, count: usize, ev: XmlEvent): usize;`
- `export fn saxEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;`
- `export fn saxHexVal(ch: u8): i32;`
- `export fn saxDecodeEntitiesImpl(raw: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`saxes@1.0.0`)，许可证：ISC
