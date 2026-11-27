[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mailto-link

Zero-allocation RFC 6068 mailto: URI parser, recipient counter, email checker, and percent-encoding builder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mailto-link` ([https://github.com/sindresorhus/mailto-link](https://github.com/sindresorhus/mailto-link)) / `crates.io:mailto` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `mailto-link` → CP 包名 `mailto-link`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `mailto-link` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mailto-link
```

```cp
import {
    MailtoUri,
    invalidMailto,
    mailtoIsSpace,
    trimSlice,
    parseMailto,
    countRecipients,
    isBasicEmail,
    isUnreserved,
} from "mailto-link";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MailtoUri` | `class` | 字段: `to: string, cc: string, bcc: string, subject: string, body: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidMailto` | `(): MailtoUri` | 调用 `invalidMailto` 并返回 `MailtoUri`。 |
| `mailtoIsSpace` | `(b: u8): bool` | 调用 `mailtoIsSpace` 并返回 `bool`。 |
| `trimSlice` | `(s: string): string` | 调用 `trimSlice` 并返回 `string`。 |
| `parseMailto` | `(uri: string): MailtoUri` | 通过 `parseMailto` 解析输入数据，返回 `MailtoUri`。 |
| `countRecipients` | `(to_list: string): usize` | 调用 `countRecipients` 并返回 `usize`。 |
| `isBasicEmail` | `(addr: string): bool` | 执行 `isBasicEmail` 条件判定，返回 `bool`。 |
| `isUnreserved` | `(b: u8): bool` | 执行 `isUnreserved` 条件判定，返回 `bool`。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `pushPctEncoded` | `(dst: []u8, pos: usize, src: string, allow_addr_chars: bool): i64` | 写入或更新状态数据。 |
| `formatMailto` | `(to: string, subject: string, body: string, cc: string, out: []u8): usize` | 通过 `formatMailto` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseMailto } from "mailto-link";

pub fn main() {
    let result = parseMailto("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
