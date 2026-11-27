[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# addressparser

Zero-allocation RFC 5322 email address and recipient list parser and formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:addressparser` ([https://github.com/nodemailer/addressparser](https://github.com/nodemailer/addressparser))（原库名 `addressparser` → CP 包名 `addressparser`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `addressparser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add addressparser
```

```cp
import {
    MailAddress,
    AddressList,
    invalidMail,
    stripEdgeComments,
    isValidLocal,
    isValidDomain,
    makeAddrFromSpec,
    unwrapDisplayName,
} from "addressparser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MailAddress` | `class` | 字段: `valid: bool, display_name: string, address: string, local: string, domain: string` |
| `AddressList` | `class` | 字段: `items: [MAX_ADDRESSES]MailAddress, count: usize` · 方法: `get` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidMail` | `(): MailAddress` | 调用 `invalidMail` 并返回 `MailAddress`。 |
| `stripEdgeComments` | `(raw: string): string` | 调用 `stripEdgeComments` 并返回 `string`。 |
| `isValidLocal` | `(local: string): bool` | 执行 `isValidLocal` 条件判定，返回 `bool`。 |
| `isValidDomain` | `(domain: string): bool` | 执行 `isValidDomain` 条件判定，返回 `bool`。 |
| `makeAddrFromSpec` | `(displayName: string, addrSpec: string): MailAddress` | 调用 `makeAddrFromSpec` 并返回 `MailAddress`。 |
| `unwrapDisplayName` | `(rawName: string): string` | 调用 `unwrapDisplayName` 并返回 `string`。 |
| `parseMailbox` | `(raw: string): MailAddress` | 通过 `parseMailbox` 解析输入数据，返回 `MailAddress`。 |
| `parseAddressList` | `(raw: string): AddressList` | 通过 `parseAddressList` 解析输入数据，返回 `AddressList`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `needsQuoting` | `(name: string): bool` | 调用 `needsQuoting` 并返回 `bool`。 |
| `formatMailbox` | `(m: MailAddress, out: []u8): string` | 通过 `formatMailbox` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { isValidLocal } from "addressparser";

pub fn main() {
    let result = isValidLocal("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
