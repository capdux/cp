[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bstr

Zero-allocation byte string utilities (Rust bstr v1 parity) powered by memchr SWAR search, line splitting, lossy UTF-8 decoding, and ASCII transforms

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bstr` ([https://github.com/BurntSushi/bstr](https://github.com/BurntSushi/bstr))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bstr` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bstr
```

```cp
import {
    SliceRange,
    bsIsContByte,
    bsUtf8ValidLen,
    bsUtf8InvalidSkip,
    bstrFind,
    bstrRfind,
    bstrFindByte,
    bstrRfindByte,
} from "bstr";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SliceRange` | `class` | 字段: `start: usize, end: usize` · 方法: `len, isEmpty` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bsIsContByte` | `(b: u8): bool` | 调用 `bsIsContByte` 并返回 `bool`。 |
| `bsUtf8ValidLen` | `(src: &[]u8, i: usize): usize` | 调用 `bsUtf8ValidLen` 并返回 `usize`。 |
| `bsUtf8InvalidSkip` | `(src: &[]u8, i: usize): usize` | 调用 `bsUtf8InvalidSkip` 并返回 `usize`。 |
| `bstrFind` | `(haystack: &[]u8, needle: &[]u8): i64` | 调用 `bstrFind` 并返回 `i64`。 |
| `bstrRfind` | `(haystack: &[]u8, needle: &[]u8): i64` | 调用 `bstrRfind` 并返回 `i64`。 |
| `bstrFindByte` | `(haystack: &[]u8, b: u8): i64` | 调用 `bstrFindByte` 并返回 `i64`。 |
| `bstrRfindByte` | `(haystack: &[]u8, b: u8): i64` | 调用 `bstrRfindByte` 并返回 `i64`。 |
| `bstrContains` | `(haystack: &[]u8, needle: &[]u8): bool` | 调用 `bstrContains` 并返回 `bool`。 |
| `bstrStartsWith` | `(s: &[]u8, prefix: &[]u8): bool` | 调用 `bstrStartsWith` 并返回 `bool`。 |
| `bstrEndsWith` | `(s: &[]u8, suffix: &[]u8): bool` | 调用 `bstrEndsWith` 并返回 `bool`。 |
| `bstrTrimStart` | `(s: &[]u8): SliceRange` | 调用 `bstrTrimStart` 并返回 `SliceRange`。 |
| `bstrTrimEnd` | `(s: &[]u8): SliceRange` | 调用 `bstrTrimEnd` 并返回 `SliceRange`。 |
| `bstrTrim` | `(s: &[]u8): SliceRange` | 调用 `bstrTrim` 并返回 `SliceRange`。 |
| `bstrLineCount` | `(s: &[]u8): usize` | 调用 `bstrLineCount` 并返回 `usize`。 |
| `bstrLineAt` | `(s: &[]u8, idx: usize): SliceRange` | 调用 `bstrLineAt` 并返回 `SliceRange`。 |
| `bstrReplaceAll` | `(src: &[]u8, pattern: &[]u8, replacement: &[]u8, out: []u8): usize` | 调用 `bstrReplaceAll` 并返回 `usize`。 |
| `bstrIsUtf8` | `(src: &[]u8): bool` | 调用 `bstrIsUtf8` 并返回 `bool`。 |
| `bstrToUtf8Lossy` | `(src: &[]u8, out: []u8): usize` | 调用 `bstrToUtf8Lossy` 并返回 `usize`。 |
| `bstrIsAscii` | `(s: &[]u8): bool` | 调用 `bstrIsAscii` 并返回 `bool`。 |
| `bstrToLowerAscii` | `(src: &[]u8, out: []u8): usize` | 调用 `bstrToLowerAscii` 并返回 `usize`。 |
| `bstrToUpperAscii` | `(src: &[]u8, out: []u8): usize` | 调用 `bstrToUpperAscii` 并返回 `usize`。 |
| `bstrEqIgnoreAsciiCase` | `(a: &[]u8, b: &[]u8): bool` | 调用 `bstrEqIgnoreAsciiCase` 并返回 `bool`。 |

## 最小调用示例

```cp
import { bsIsContByte } from "bstr";

pub fn main() {
    let result = bsIsContByte(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
