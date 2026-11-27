[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cast

Zero-allocation checked primitive numeric casts preventing silent overflow, underflow, and NaN/Inf truncation (clean-room MIT implementation of crates.io cast, #443 / 335M+)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cast` ([https://github.com/japaric/cast.rs](https://github.com/japaric/cast.rs)) | 许可：`MIT OR Apache-2.0`

| 元信息 | 值 |
| --- | --- |
| 包名 | `cast` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cast
```

```cp
import {
    CastError,
    u8FromI64,
    u8FromU64,
    u16FromI64,
    u32FromI64,
    u32FromU64,
    i32FromU64,
    i32FromI64,
} from "cast";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CastError` | `Overflow, Underflow, Infinite, NaN` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `u8FromI64` | `(v: i64): u8?` | 调用 `u8FromI64` 并返回 `u8?`。 |
| `u8FromU64` | `(v: u64): u8?` | 调用 `u8FromU64` 并返回 `u8?`。 |
| `u16FromI64` | `(v: i64): u16?` | 调用 `u16FromI64` 并返回 `u16?`。 |
| `u32FromI64` | `(v: i64): u32?` | 调用 `u32FromI64` 并返回 `u32?`。 |
| `u32FromU64` | `(v: u64): u32?` | 调用 `u32FromU64` 并返回 `u32?`。 |
| `i32FromU64` | `(v: u64): i32?` | 调用 `i32FromU64` 并返回 `i32?`。 |
| `i32FromI64` | `(v: i64): i32?` | 调用 `i32FromI64` 并返回 `i32?`。 |
| `u64FromI64` | `(v: i64): u64?` | 调用 `u64FromI64` 并返回 `u64?`。 |
| `i64FromU64` | `(v: u64): i64?` | 调用 `i64FromU64` 并返回 `i64?`。 |
| `usizeFromI64` | `(v: i64): usize?` | 调用 `usizeFromI64` 并返回 `usize?`。 |
| `u32FromF64` | `(v: f64): u32?` | 调用 `u32FromF64` 并返回 `u32?`。 |
| `i32FromF64` | `(v: f64): i32?` | 调用 `i32FromF64` 并返回 `i32?`。 |
| `u8FromF64` | `(v: f64): u8?` | 调用 `u8FromF64` 并返回 `u8?`。 |
| `checkF64ToI32` | `(v: f64): CastError?` | 校验输入合法性，返回 `CastError?`。 |

## 最小调用示例

```cp
import { u8FromI64 } from "cast";

pub fn main() {
    let result = u8FromI64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
