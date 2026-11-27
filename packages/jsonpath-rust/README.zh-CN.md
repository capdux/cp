[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jsonpath-rust

Zero-heap RFC 9535 JSONPath expression parser and canonical serializer in pure CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `jsonpath-rust` |
| 版本 | `0.7.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add jsonpath-rust
```

```cp
import {
    JpSegKind,
    JpSegment,
    JprIntRes,
    jprIsWs,
    jprIsDigit,
    jprIsIdentStart,
    jprIsIdentCont,
    jprIsSimpleIdent,
} from "jsonpath-rust";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `JpSegKind` | `Root, Property, Index, Wildcard, RecursiveProp, RecursiveWildcard, Slice` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `JpSegment` | `class` | 字段: `kind: JpSegKind, name: string, index: i64, slice_start: i64, slice_end: i64, has_start: bool, +1 项` |
| `JprIntRes` | `class` | 字段: `val: i64, next_pos: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `jprIsWs` | `(c: u8): bool` | 调用 `jprIsWs` 并返回 `bool`。 |
| `jprIsDigit` | `(c: u8): bool` | 调用 `jprIsDigit` 并返回 `bool`。 |
| `jprIsIdentStart` | `(c: u8): bool` | 调用 `jprIsIdentStart` 并返回 `bool`。 |
| `jprIsIdentCont` | `(c: u8): bool` | 调用 `jprIsIdentCont` 并返回 `bool`。 |
| `jprIsSimpleIdent` | `(s: string): bool` | 调用 `jprIsSimpleIdent` 并返回 `bool`。 |
| `jprMakeSeg` | `(kind: JpSegKind, name: string): JpSegment` | 调用 `jprMakeSeg` 并返回 `JpSegment`。 |
| `jprParseInt` | `(s: string, start_pos: usize): JprIntRes` | 调用 `jprParseInt` 并返回 `JprIntRes`。 |
| `parseJsonPath` | `(expr: string, out: []JpSegment): i64` | 通过 `parseJsonPath` 解析输入数据，返回 `i64`。 |
| `jprPushByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `jprPushByte` 并返回 `usize`。 |
| `jprPushSlice` | `(out: []u8, pos: usize, s: string): usize` | 调用 `jprPushSlice` 并返回 `usize`。 |
| `jprPushI64` | `(out: []u8, pos: usize, val: i64): usize` | 调用 `jprPushI64` 并返回 `usize`。 |
| `formatJsonPath` | `(segs: &[]JpSegment, out: []u8): usize` | 通过 `formatJsonPath` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseJsonPath } from "jsonpath-rust";

pub fn main() {
    let result = parseJsonPath("example", /* out: []JpSegment */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
