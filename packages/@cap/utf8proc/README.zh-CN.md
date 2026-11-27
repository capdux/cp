[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/utf8proc

Clean Unicode 16.0 normalization (NFC/NFD/NFKC/NFKD), case-folding & charwidth library for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/utf8proc` |
| 版本 | `2.10.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/utf8proc
```

```cp
import {
    Utf8ProcError,
    NormString,
    utf8proc_version,
    utf8proc_unicode_version,
    utf8proc_charwidth,
    utf8proc_tolower,
    utf8proc_toupper,
    utf8proc_NFC,
} from "@cap/utf8proc";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `Utf8ProcError` | `NormalizeFailed, OutOfMemory` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `NormString` | `class` | 字段: `raw: [*:0]u8!?, len: usize` · 方法: `nfc, nfd, nfkc, nfkcCasefold, runOp, slice, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `utf8proc_version` | `(): [*:0]&u8!` | 调用 `utf8proc_version` 并返回 `[*:0]&u8!`。 |
| `utf8proc_unicode_version` | `(): [*:0]&u8!` | 调用 `utf8proc_unicode_version` 并返回 `[*:0]&u8!`。 |
| `utf8proc_charwidth` | `(codepoint: i32): i32` | 调用 `utf8proc_charwidth` 并返回 `i32`。 |
| `utf8proc_tolower` | `(c: i32): i32` | 调用 `utf8proc_tolower` 并返回 `i32`。 |
| `utf8proc_toupper` | `(c: i32): i32` | 调用 `utf8proc_toupper` 并返回 `i32`。 |
| `utf8proc_NFC` | `(str: [*:0]&u8!): [*:0]u8!?` | 调用 `utf8proc_NFC` 并返回 `[*:0]u8!?`。 |
| `utf8proc_NFD` | `(str: [*:0]&u8!): [*:0]u8!?` | 调用 `utf8proc_NFD` 并返回 `[*:0]u8!?`。 |
| `utf8proc_NFKC` | `(str: [*:0]&u8!): [*:0]u8!?` | 调用 `utf8proc_NFKC` 并返回 `[*:0]u8!?`。 |
| `utf8proc_NFKD` | `(str: [*:0]&u8!): [*:0]u8!?` | 调用 `utf8proc_NFKD` 并返回 `[*:0]u8!?`。 |
| `utf8proc_NFKC_Casefold` | `(str: [*:0]&u8!): [*:0]u8!?` | 调用 `utf8proc_NFKC_Casefold` 并返回 `[*:0]u8!?`。 |
| `free` | `(ptr: *auto!?)` | 释放模块分配的底层资源。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |
| `unicodeVersion` | `(): string` | 调用 `unicodeVersion` 并返回 `string`。 |
| `charWidth` | `(cp: i32): i32` | 调用 `charWidth` 并返回 `i32`。 |
| `toLower` | `(cp: i32): i32` | 调用 `toLower` 并返回 `i32`。 |
| `toUpper` | `(cp: i32): i32` | 调用 `toUpper` 并返回 `i32`。 |

## 最小调用示例

```cp
import { versionString } from "@cap/utf8proc";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
