[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unicode-ident

Zero-heap UAX #31 Unicode identifier classification (XID_Start / XID_Continue) and UTF-8 identifier scanner in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-ident` ([https://github.com/dtolnay/unicode-ident](https://github.com/dtolnay/unicode-ident))

| 元信息 | 值 |
| --- | --- |
| 包名 | `unicode-ident` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unicode-ident
```

```cp
import {
    Utf8Codepoint,
    uidInRange,
    decodeUtf8At,
    uidIsXidStartNonAscii,
    uidIsXidContinueExtra,
    isXidStart,
    isXidContinue,
    isIdentStart,
} from "unicode-ident";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Utf8Codepoint` | `class` | 字段: `cp: u32, len: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `uidInRange` | `(cp: u32, lo: u32, hi: u32): bool` | 调用 `uidInRange` 并返回 `bool`。 |
| `decodeUtf8At` | `(s: string, offset: usize): Utf8Codepoint` | 通过 `decodeUtf8At` 解析输入数据，返回 `Utf8Codepoint`。 |
| `uidIsXidStartNonAscii` | `(cp: u32): bool` | 调用 `uidIsXidStartNonAscii` 并返回 `bool`。 |
| `uidIsXidContinueExtra` | `(cp: u32): bool` | 调用 `uidIsXidContinueExtra` 并返回 `bool`。 |
| `isXidStart` | `(cp: u32): bool` | 执行 `isXidStart` 条件判定，返回 `bool`。 |
| `isXidContinue` | `(cp: u32): bool` | 执行 `isXidContinue` 条件判定，返回 `bool`。 |
| `isIdentStart` | `(cp: u32): bool` | 执行 `isIdentStart` 条件判定，返回 `bool`。 |
| `scanIdentLen` | `(s: string): usize` | 调用 `scanIdentLen` 并返回 `usize`。 |
| `isValidIdent` | `(s: string): bool` | 执行 `isValidIdent` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { decodeUtf8At } from "unicode-ident";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
