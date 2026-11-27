[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# dom-serializer

> 版本：`2.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTML/XML DOM node serializer (covers domelementtype and w3c-xmlserializer)

## 安装与导入

```bash
cap add dom-serializer
```

```cp
import { dsrlToLowerByte } from "dom-serializer";
```

## 核心导出 API

- `export enum DomNodeKind`
- `export fn dsrlToLowerByte(c: u8): u8;`
- `export fn dsrlEqIgnoreCase(a: string, b: string): bool;`
- `export fn dsrlWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn dsrlWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn dsrlWriteAttrValue(out: []u8, pos: usize, val: string): usize;`
- `export fn isVoidElement(tag: string): bool;`
- `export fn serializeText(text: string, out: []u8): usize;`
- `export fn serializeOpenTag(tag: string, attrs: &[]DomAttr, self_close: bool, out: []u8): usize;`
- `export fn serializeElement(tag: string, attrs: &[]DomAttr, inner_html: string, opts: SerializeOptions, out: []u8): usize;`
- `export fn serializeComment(text: string, out: []u8): usize;`
- `export fn serializeCdata(text: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`dom-serializer@2.0.0`)（https://github.com/cheeriojs/dom-serializer），许可证：MIT
