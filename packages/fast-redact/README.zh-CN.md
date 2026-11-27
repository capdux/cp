[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fast-redact

Zero-allocation PII & secret redaction/masking engine (fast-redact / maskdata / secrecy parity: emails, Luhn credit cards, JWT/API tokens, phones, IPv4, and sensitive JSON fields) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-redact` ([https://github.com/davidmarkclements/fast-redact](https://github.com/davidmarkclements/fast-redact))（原库名 `fast-redact` → CP 包名 `fast-redact`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `fast-redact` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fast-redact
```

```cp
import {
    RedactOptions,
    isTokenChar,
    isEmailLocalChar,
    isDomainChar,
    appendStr,
    appendByte,
    startsWithAt,
    maskEmail,
} from "fast-redact";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RedactOptions` | `class` | 字段: `redact_emails: bool, redact_tokens: bool, redact_cards: bool, redact_phones: bool, redact_ipv4: bool` · 方法: `defaultAll, strict` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isTokenChar` | `(c: u8): bool` | 执行 `isTokenChar` 条件判定，返回 `bool`。 |
| `isEmailLocalChar` | `(c: u8): bool` | 执行 `isEmailLocalChar` 条件判定，返回 `bool`。 |
| `isDomainChar` | `(c: u8): bool` | 执行 `isDomainChar` 条件判定，返回 `bool`。 |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `appendStr` 并返回 `usize`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `startsWithAt` | `(s: string, idx: usize, prefix: string): bool` | 调用 `startsWithAt` 并返回 `bool`。 |
| `maskEmail` | `(email: string, out: []u8): string` | 调用 `maskEmail` 并返回 `string`。 |
| `maskPhone` | `(phone: string, out: []u8): string` | 调用 `maskPhone` 并返回 `string`。 |
| `maskCard` | `(card: string, out: []u8): string` | 调用 `maskCard` 并返回 `string`。 |
| `maskSecret` | `(secret: string, out: []u8): string` | 调用 `maskSecret` 并返回 `string`。 |
| `maskIp` | `(ip: string, out: []u8): string` | 调用 `maskIp` 并返回 `string`。 |
| `checkLuhnRange` | `(s: string, start: usize, end: usize): bool` | 校验输入合法性，返回 `bool`。 |
| `matchTokenEnd` | `(s: string, idx: usize): usize` | 调用 `matchTokenEnd` 并返回 `usize`。 |
| `matchEmailEnd` | `(s: string, idx: usize): usize` | 调用 `matchEmailEnd` 并返回 `usize`。 |
| `matchCardEnd` | `(s: string, idx: usize): usize` | 调用 `matchCardEnd` 并返回 `usize`。 |
| `matchPhoneEnd` | `(s: string, idx: usize): usize` | 调用 `matchPhoneEnd` 并返回 `usize`。 |
| `matchIpv4End` | `(s: string, idx: usize): usize` | 调用 `matchIpv4End` 并返回 `usize`。 |
| `redactText` | `(input: string, opts: RedactOptions, out: []u8): string` | 调用 `redactText` 并返回 `string`。 |
| `containsSensitive` | `(input: string): bool` | 调用 `containsSensitive` 并返回 `bool`。 |
| `isSensitiveKey` | `(key: string): bool` | 执行 `isSensitiveKey` 条件判定，返回 `bool`。 |
| `findJsonStringEnd` | `(s: string, open_quote: usize): usize` | 查询或读取目标数据，返回 `usize`。 |
| `redactJsonFields` | `(json_str: string, out: []u8): string` | 调用 `redactJsonFields` 并返回 `string`。 |

## 最小调用示例

```cp
import { isSensitiveKey } from "fast-redact";

pub fn main() {
    let result = isSensitiveKey("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
