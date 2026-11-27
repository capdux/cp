[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# litemap

Zero-allocation compact sorted flat-array map with O(log N) binary search and deterministic ordered iteration (Rust ICU4X litemap parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:litemap` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))

| 元信息 | 值 |
| --- | --- |
| 包名 | `litemap` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add litemap
```

```cp
import {
    SearchResult,
    StrEntry,
    StrInsertResult,
    U64Entry,
    U64InsertResult,
    LiteMapStr,
    LiteMapU64,
    cmpStr,
} from "litemap";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SearchResult` | `class` | 字段: `found: bool, index: usize` |
| `StrEntry` | `class` | 字段: `key: string, value: string` |
| `StrInsertResult` | `class` | 字段: `ok: bool, replaced: bool, prev: string` |
| `U64Entry` | `class` | 字段: `key: u64, value: i64` |
| `U64InsertResult` | `class` | 字段: `ok: bool, replaced: bool, prev: i64` |
| `LiteMapStr` | `class` | 字段: `entries: [LITEMAP_CAP]StrEntry, count: usize` · 方法: `init, len, isEmpty, isFull, clear, binarySearch, +5 项` |
| `LiteMapU64` | `class` | 字段: `entries: [LITEMAP_CAP]U64Entry, count: usize` · 方法: `init, len, isEmpty, isFull, clear, binarySearch, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cmpStr` | `(a: string, b: string): i8` | 调用 `cmpStr` 并返回 `i8`。 |

## 最小调用示例

```cp
import { cmpStr } from "litemap";

pub fn main() {
    let result = cmpStr("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
