[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# totp-rs

Zero-allocation Google Authenticator otpauth:// TOTP/HOTP Key URI parser, Base32 secret validator, and builder for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:totp-rs` ([https://github.com/constantoine/totp-rs](https://github.com/constantoine/totp-rs)) / `npm:otpauth` ([https://github.com/hectorm/otpauth](https://github.com/hectorm/otpauth))（原库名 `totp-rs` → CP 包名 `totp-rs`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `totp-rs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add totp-rs
```

```cp
import {
    OtpAuthUri,
    LabelParts,
    otpIsUnreserved,
    otpParseStrictU64,
    otpWriteRaw,
    otpWritePctEncoded,
    otpWriteU64,
    isValidBase32Secret,
} from "totp-rs";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OtpAuthUri` | `class` | 字段: `kind: u8, label: string, secret: string, issuer: string, algorithm: u8, digits: u8, +3 项` |
| `LabelParts` | `class` | 字段: `issuer: string, account: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `otpIsUnreserved` | `(c: u8): bool` | 调用 `otpIsUnreserved` 并返回 `bool`。 |
| `otpParseStrictU64` | `(s: string): u64?` | 调用 `otpParseStrictU64` 并返回 `u64?`。 |
| `otpWriteRaw` | `(out: []u8, pos: &mut usize, src: string): bool` | 调用 `otpWriteRaw` 并返回 `bool`。 |
| `otpWritePctEncoded` | `(out: []u8, pos: &mut usize, src: string, allow_label_chars: bool): bool` | 调用 `otpWritePctEncoded` 并返回 `bool`。 |
| `otpWriteU64` | `(out: []u8, pos: &mut usize, val: u64): bool` | 调用 `otpWriteU64` 并返回 `bool`。 |
| `isValidBase32Secret` | `(secret: string): bool` | 执行 `isValidBase32Secret` 条件判定，返回 `bool`。 |
| `parseOtpAuthUri` | `(uri: string): OtpAuthUri` | 通过 `parseOtpAuthUri` 解析输入数据，返回 `OtpAuthUri`。 |
| `formatOtpAuthUri` | `(cfg: OtpAuthUri, out: []u8): usize` | 通过 `formatOtpAuthUri` 编码或格式化输出，返回 `usize`。 |
| `splitLabel` | `(label: string): LabelParts` | 调用 `splitLabel` 并返回 `LabelParts`。 |

## 最小调用示例

```cp
import { isValidBase32Secret } from "totp-rs";

pub fn main() {
    let result = isValidBase32Secret("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
