[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# dom-serializer

Zero-heap HTML/XML DOM node serializer (covers domelementtype and w3c-xmlserializer)

| 元信息 | 值 |
| --- | --- |
| 包名 | `dom-serializer` |
| 版本 | `2.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add dom-serializer
```

```cp
import {
    DomNodeKind,
    DomAttr,
    SerializeOptions,
    dsrlWriteByte,
    dsrlWriteAttrValue,
    isVoidElement,
    serializeText,
    serializeOpenTag,
} from "dom-serializer";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `DomNodeKind` | `Element, Text, Comment, Cdata, Doctype` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DomAttr` | `class` | 字段: `name: string, value: string` |
| `SerializeOptions` | `class` | 字段: `xml_mode: bool, self_closing_tags: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `dsrlWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `dsrlWriteByte` 并返回 `usize`。 |
| `dsrlWriteAttrValue` | `(out: []u8, pos: usize, val: string): usize` | 调用 `dsrlWriteAttrValue` 并返回 `usize`。 |
| `isVoidElement` | `(tag: string): bool` | 执行 `isVoidElement` 条件判定，返回 `bool`。 |
| `serializeText` | `(text: string, out: []u8): usize` | 调用 `serializeText` 并返回 `usize`。 |
| `serializeOpenTag` | `(tag: string, attrs: &[]DomAttr, self_close: bool, out: []u8): usize` | 调用 `serializeOpenTag` 并返回 `usize`。 |
| `serializeElement` | `(tag: string, attrs: &[]DomAttr, inner_html: string, opts: SerializeOptions, out: []u8): usize` | 调用 `serializeElement` 并返回 `usize`。 |
| `serializeComment` | `(text: string, out: []u8): usize` | 调用 `serializeComment` 并返回 `usize`。 |
| `serializeCdata` | `(text: string, out: []u8): usize` | 调用 `serializeCdata` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isVoidElement } from "dom-serializer";

pub fn main() {
    let result = isVoidElement("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
