[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/saxes

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:saxes` ([https://github.com/lddubeau/saxes](https://github.com/lddubeau/saxes))（原库名 `saxes` → CP 包名 `@cap/saxes`）

Zero-allocation streaming SAX XML 1.0 event parser with tag balance validation and entity decoding for CP — clean-room ISC implementation inspired by npm `saxes`.

纯 CP 零堆分配流式 SAX XML 1.0 事件解析器（对标 `saxes`），内置闭合标签栈校验、属性解析、CDATA / 注释 / 处理指令识别，以及命名与数字（十进制/十六进制）XML 实体解码。

## Install / 安装

```bash
cap add @cap/saxes
```

## API

### Enums & Structs / 核心类型

| Symbol | Description |
| --- | --- |
| `XmlEventKind` | `OpenTag`, `CloseTag`, `SelfClosingTag`, `Text`, `Cdata`, `Comment`, `ProcessingInstruction`, `Error` |
| `XmlAttr` | `struct { name: string, value: string }` |
| `XmlEvent` | `struct { kind: XmlEventKind, name: string, text: string, attrs: [8]XmlAttr, attr_count: usize }` |
| `SaxesParser` | Stateful XML 1.0 parser with internal tag nesting stack |

### Methods / 核心方法

| Method | Signature | Description |
| --- | --- | --- |
| `SaxesParser.new` | `(): SaxesParser` | Create a new zero-allocation SAX parser |
| `parseAll` | `(*_, xml: string, out: []XmlEvent): usize` | Parse XML into event buffer and return event count |
| `isWellFormed` | `(*_, xml: string): bool` | Validate XML 1.0 well-formedness (tag balance, single root, syntax) |
| `decodeEntities` | `(raw: string, out: []u8): usize` | Decode `&lt;`, `&gt;`, `&amp;`, `&quot;`, `&apos;`, `&#NNN;`, `&#xHHH;` into UTF-8 bytes |

## Usage / 使用示例

```cp
import { SaxesParser, XmlEvent, XmlEventKind } from "@cap/saxes";

let parser = SaxesParser.new();
let events: [16]XmlEvent = undefined;
const count = parser.parseAll("<book id=\"1\"><title>CP</title></book>", events[0..16]);
const ok = parser.isWellFormed("<book id=\"1\"><title>CP</title></book>"); // true
```
