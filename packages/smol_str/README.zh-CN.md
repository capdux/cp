[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# smol_str

Zero-heap pure-CP O(1)-clone small string optimization (24-byte inline SSO + read-only slice fallback, crates.io smol_str equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:smol_str` ([https://github.com/rust-analyzer/smol_str](https://github.com/rust-analyzer/smol_str))（原库名 `smol_str` → CP 包名 `smol_str`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `smol_str` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add smol_str
```

```cp
import {
    SmolStr,
    smolGetByte,
    smolMinUsize,
    smolStartsWith,
    smolEndsWith,
} from "smol_str";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SmolStr` | `class` | 字段: `inline_buf: [24]u8, inline_len: u8, is_inline: bool, ext: string` · 方法: `empty, new, fromInt, len, isEmpty, isInline, +7 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `smolGetByte` | `(s: SmolStr, idx: usize): u8` | 调用 `smolGetByte` 并返回 `u8`。 |
| `smolMinUsize` | `(a: usize, b: usize): usize` | 调用 `smolMinUsize` 并返回 `usize`。 |
| `smolStartsWith` | `(s: SmolStr, prefix: string): bool` | 调用 `smolStartsWith` 并返回 `bool`。 |
| `smolEndsWith` | `(s: SmolStr, suffix: string): bool` | 调用 `smolEndsWith` 并返回 `bool`。 |

## 最小调用示例

```cp
import { smolStartsWith } from "smol_str";

pub fn main() {
    let result = smolStartsWith(/* s: SmolStr */, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
