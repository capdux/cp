[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# utf16_iter

Zero-allocation bidirectional UTF-16 code point iterator, Latin-1 to UTF-16 expander, and UTF-16 to UTF-8 transcoder in CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:utf16_iter` ([https://github.com/hsivonen/utf16_iter](https://github.com/hsivonen/utf16_iter), `Apache-2.0 OR MIT`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `utf16_iter` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0 OR MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add utf16_iter
```

```cp
import {
    Utf16Chars,
    Utf16Latin1,
    u16iIsHighSurrogate,
    u16iIsLowSurrogate,
    u16iCombineSurrogates,
    u16iEncodeUtf8,
    countUtf16Chars,
    utf16ToUtf8,
} from "utf16_iter";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Utf16Chars` | `class` | 字段: `slice: &[]u16, pos: usize, end: usize` · 方法: `new, isEmpty, asSlice, next, nextBack` |
| `Utf16Latin1` | `class` | 字段: `bytes: &[]u8, pos: usize, end: usize` · 方法: `new, isEmpty, asSlice, next, nextBack, writeTo, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `u16iIsHighSurrogate` | `(unit: u16): bool` | 调用 `u16iIsHighSurrogate` 并返回 `bool`。 |
| `u16iIsLowSurrogate` | `(unit: u16): bool` | 调用 `u16iIsLowSurrogate` 并返回 `bool`。 |
| `u16iCombineSurrogates` | `(high: u16, low: u16): u32` | 调用 `u16iCombineSurrogates` 并返回 `u32`。 |
| `u16iEncodeUtf8` | `(cp: u32, out: []u8, offset: usize): usize` | 调用 `u16iEncodeUtf8` 并返回 `usize`。 |
| `countUtf16Chars` | `(slice: &[]u16): usize` | 调用 `countUtf16Chars` 并返回 `usize`。 |
| `utf16ToUtf8` | `(slice: &[]u16, out: []u8): usize` | 调用 `utf16ToUtf8` 并返回 `usize`。 |

## 最小调用示例

```cp
import { u16iIsHighSurrogate } from "utf16_iter";

pub fn main() {
    let result = u16iIsHighSurrogate(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
