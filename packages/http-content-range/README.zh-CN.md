[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# http-content-range

Zero-allocation RFC 9110 HTTP Content-Range header parser and formatter (clean-room MIT implementation of crates.io http-content-range)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:http-content-range` ([https://github.com/briansmith/http-content-range](https://github.com/briansmith/http-content-range)) | 许可：`MIT OR Apache-2.0`

| 元信息 | 值 |
| --- | --- |
| 包名 | `http-content-range` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add http-content-range
```

```cp
import {
    ContentRangeKind,
    ContentRange,
    hcrParseU64,
    hcrPush,
    hcrPushU64,
    parseContentRange,
    formatContentRangeHeader,
} from "http-content-range";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `ContentRangeKind` | `Invalid, Bytes, BytesUnknownLength, Unsatisfied` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ContentRange` | `class` | 字段: `kind: ContentRangeKind, first_byte: u64, last_byte: u64, complete_length: u64` · 方法: `isValid, rangeLen` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hcrParseU64` | `(s: string): u64?` | 调用 `hcrParseU64` 并返回 `u64?`。 |
| `hcrPush` | `(out: []u8, pos: usize, s: string): usize` | 调用 `hcrPush` 并返回 `usize`。 |
| `hcrPushU64` | `(out: []u8, pos: usize, val: u64): usize` | 调用 `hcrPushU64` 并返回 `usize`。 |
| `parseContentRange` | `(header: string): ContentRange` | 通过 `parseContentRange` 解析输入数据，返回 `ContentRange`。 |
| `formatContentRangeHeader` | `(cr: ContentRange, out: []u8): usize` | 通过 `formatContentRangeHeader` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseContentRange } from "http-content-range";

pub fn main() {
    let result = parseContentRange("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
