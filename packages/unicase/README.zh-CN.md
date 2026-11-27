[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unicase

Zero-heap Unicode & ASCII case-insensitive comparison, ordering, matching, and hashing (clean-room port of Rust unicase)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicase` ([https://github.com/seanmonstar/unicase](https://github.com/seanmonstar/unicase))

| 元信息 | 值 |
| --- | --- |
| 包名 | `unicase` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unicase
```

```cp
import {
    Ascii,
    UniCase,
    ucDecodeUtf8,
    ucEncodeUtf8,
    foldAsciiByte,
    foldCodepoint,
    caseFold,
    eqAscii,
} from "unicase";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Ascii` | `class` | 字段: `raw: string` · 方法: `new, intoInner, eq, cmp, hash` |
| `UniCase` | `class` | 字段: `raw: string` · 方法: `new, ascii, intoInner, eq, cmp, hash` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ucDecodeUtf8` | `(s: string, pos: usize, step_out: []usize): u32` | 调用 `ucDecodeUtf8` 并返回 `u32`。 |
| `ucEncodeUtf8` | `(cp: u32, out: []u8, pos: usize): usize` | 调用 `ucEncodeUtf8` 并返回 `usize`。 |
| `foldAsciiByte` | `(b: u8): u8` | 调用 `foldAsciiByte` 并返回 `u8`。 |
| `foldCodepoint` | `(cp: u32, folded_out: []u32): usize` | 调用 `foldCodepoint` 并返回 `usize`。 |
| `caseFold` | `(src: string, out: []u8): usize` | 调用 `caseFold` 并返回 `usize`。 |
| `eqAscii` | `(a: string, b: string): bool` | 调用 `eqAscii` 并返回 `bool`。 |
| `cmpAscii` | `(a: string, b: string): i32` | 调用 `cmpAscii` 并返回 `i32`。 |
| `cmpUnicode` | `(a: string, b: string): i32` | 调用 `cmpUnicode` 并返回 `i32`。 |
| `eqUnicode` | `(a: string, b: string): bool` | 调用 `eqUnicode` 并返回 `bool`。 |
| `hashAscii` | `(s: string): u64` | 执行 `hashAscii` 条件判定，返回 `bool`。 |
| `hashUnicode` | `(s: string): u64` | 执行 `hashUnicode` 条件判定，返回 `bool`。 |
| `startsWithIgnoreCase` | `(haystack: string, prefix: string): bool` | 调用 `startsWithIgnoreCase` 并返回 `bool`。 |
| `endsWithIgnoreCase` | `(haystack: string, suffix: string): bool` | 调用 `endsWithIgnoreCase` 并返回 `bool`。 |
| `containsIgnoreCase` | `(haystack: string, needle: string): bool` | 调用 `containsIgnoreCase` 并返回 `bool`。 |

## 最小调用示例

```cp
import { hashAscii } from "unicase";

pub fn main() {
    let result = hashAscii("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
