[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# saxes

Zero-allocation streaming SAX XML 1.0 event parser with tag stack validation and entity decoding (npm saxes parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:saxes` ([https://github.com/lddubeau/saxes](https://github.com/lddubeau/saxes))（原库名 `saxes` → CP 包名 `saxes`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `saxes` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add saxes
```

```cp
import {
    XmlEventKind,
    XmlAttr,
    XmlEvent,
    SaxesParser,
    saxEmptyAttrs,
    saxMakeEvent,
    saxIsWs,
    saxIsNameStart,
} from "saxes";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `XmlEventKind` | `OpenTag, CloseTag, SelfClosingTag, Text, Cdata, Comment, ProcessingInstruction, Error` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `XmlAttr` | `class` | 字段: `name: string, value: string` |
| `XmlEvent` | `class` | 字段: `kind: XmlEventKind, name: string, text: string, attrs: [8]XmlAttr, attr_count: usize` |
| `SaxesParser` | `class` | 字段: `stack: [32]string, depth: usize, has_err: bool, root_count: usize` · 方法: `new, parseAll, isWellFormed, decodeEntities` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `saxEmptyAttrs` | `(): [8]XmlAttr` | 调用 `saxEmptyAttrs` 并返回 `[8]XmlAttr`。 |
| `saxMakeEvent` | `(kind: XmlEventKind, name: string, text: string): XmlEvent` | 调用 `saxMakeEvent` 并返回 `XmlEvent`。 |
| `saxIsWs` | `(ch: u8): bool` | 调用 `saxIsWs` 并返回 `bool`。 |
| `saxIsNameStart` | `(ch: u8): bool` | 调用 `saxIsNameStart` 并返回 `bool`。 |
| `saxIsNameChar` | `(ch: u8): bool` | 调用 `saxIsNameChar` 并返回 `bool`。 |
| `saxStrEq` | `(a: string, b: string): bool` | 调用 `saxStrEq` 并返回 `bool`。 |
| `saxStartsWithAt` | `(s: string, pos: usize, prefix: string): bool` | 调用 `saxStartsWithAt` 并返回 `bool`。 |
| `saxFindSubstrFrom` | `(s: string, start: usize, needle: string): i64` | 调用 `saxFindSubstrFrom` 并返回 `i64`。 |
| `saxAllWs` | `(s: string): bool` | 调用 `saxAllWs` 并返回 `bool`。 |
| `saxEmit` | `(out: []XmlEvent, count: usize, ev: XmlEvent): usize` | 调用 `saxEmit` 并返回 `usize`。 |
| `saxEncodeUtf8` | `(cp: u32, out: []u8, pos: usize): usize` | 调用 `saxEncodeUtf8` 并返回 `usize`。 |
| `saxHexVal` | `(ch: u8): i32` | 调用 `saxHexVal` 并返回 `i32`。 |
| `saxDecodeEntitiesImpl` | `(raw: string, out: []u8): usize` | 调用 `saxDecodeEntitiesImpl` 并返回 `usize`。 |

## 最小调用示例

```cp
import { saxMakeEvent } from "saxes";

pub fn main() {
    let result = saxMakeEvent(/* kind: XmlEventKind */, "example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
