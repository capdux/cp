[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# p-try

Zero-heap structured sync/async execution outcome wrapper preventing escaping errors, with safe integer parsing, division, and short-circuit chaining (clean-room port of npm p-try)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-try` ([https://github.com/sindresorhus/p-try](https://github.com/sindresorhus/p-try))

| 元信息 | 值 |
| --- | --- |
| 包名 | `p-try` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add p-try
```

```cp
import {
    TryOutcome,
    ptryIsSpace,
    ptryIsDigit,
    ptryI64Min,
    ptryAbsU64,
    tryOkInt,
    tryOkStr,
    tryErr,
} from "p-try";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TryOutcome` | `class` | 字段: `ok: bool, value_i64: i64, value_str: string, err_code: i32, err_msg: string` · 方法: `okInt, okStr, err, isOk, isErr` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ptryIsSpace` | `(ch: u8): bool` | 调用 `ptryIsSpace` 并返回 `bool`。 |
| `ptryIsDigit` | `(ch: u8): bool` | 调用 `ptryIsDigit` 并返回 `bool`。 |
| `ptryI64Min` | `(): i64` | 调用 `ptryI64Min` 并返回 `i64`。 |
| `ptryAbsU64` | `(v: i64): u64` | 调用 `ptryAbsU64` 并返回 `u64`。 |
| `tryOkInt` | `(v: i64): TryOutcome` | 调用 `tryOkInt` 并返回 `TryOutcome`。 |
| `tryOkStr` | `(s: string): TryOutcome` | 调用 `tryOkStr` 并返回 `TryOutcome`。 |
| `tryErr` | `(code: i32, msg: string): TryOutcome` | 调用 `tryErr` 并返回 `TryOutcome`。 |
| `unwrapOrInt` | `(o: TryOutcome, fallback: i64): i64` | 调用 `unwrapOrInt` 并返回 `i64`。 |
| `unwrapOrStr` | `(o: TryOutcome, fallback: string): string` | 调用 `unwrapOrStr` 并返回 `string`。 |
| `tryParseI64` | `(s: string): TryOutcome` | 调用 `tryParseI64` 并返回 `TryOutcome`。 |
| `tryDivI64` | `(a: i64, b: i64): TryOutcome` | 调用 `tryDivI64` 并返回 `TryOutcome`。 |
| `tryChain` | `(first: TryOutcome, next_ok: bool, next_val: i64, next_err_code: i32, next_err_msg: string): TryOutcome` | 调用 `tryChain` 并返回 `TryOutcome`。 |

## 最小调用示例

```cp
import { tryOkStr } from "p-try";

pub fn main() {
    let result = tryOkStr("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
