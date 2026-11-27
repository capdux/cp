[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# http

Zero-allocation HTTP Method, StatusCode, Version, and HeaderName core types (crates.io http v1 parity) composing unicase for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:http` ([https://github.com/hyperium/http](https://github.com/hyperium/http))（原库名 `http` → CP 包名 `http`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `http` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add http
```

```cp
import {
    StatusCode,
    htpStrEq,
    htpIsTchar,
    parseMethod,
    methodAsStr,
    isSafeMethod,
    isIdempotentMethod,
    parseHttpVersion,
} from "http";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StatusCode` | `class` | 字段: `code: u16` · 方法: `fromU16, asU16, canonicalReason, isInformational, isSuccess, isRedirection, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `htpStrEq` | `(a: string, b: string): bool` | 调用 `htpStrEq` 并返回 `bool`。 |
| `htpIsTchar` | `(b: u8): bool` | 调用 `htpIsTchar` 并返回 `bool`。 |
| `parseMethod` | `(s: string): u8` | 通过 `parseMethod` 解析输入数据，返回 `u8`。 |
| `methodAsStr` | `(m: u8): string` | 调用 `methodAsStr` 并返回 `string`。 |
| `isSafeMethod` | `(m: u8): bool` | 执行 `isSafeMethod` 条件判定，返回 `bool`。 |
| `isIdempotentMethod` | `(m: u8): bool` | 执行 `isIdempotentMethod` 条件判定，返回 `bool`。 |
| `parseHttpVersion` | `(s: string): u8` | 通过 `parseHttpVersion` 解析输入数据，返回 `u8`。 |
| `httpVersionAsStr` | `(v: u8): string` | 调用 `httpVersionAsStr` 并返回 `string`。 |
| `isValidHeaderName` | `(name: string): bool` | 执行 `isValidHeaderName` 条件判定，返回 `bool`。 |
| `headerNameEq` | `(a: string, b: string): bool` | 调用 `headerNameEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parseMethod } from "http";

pub fn main() {
    let result = parseMethod("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
