[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# set-cookie-parser

Zero-allocation RFC 6265 Set-Cookie HTTP header parser and splitter

| 元信息 | 值 |
| --- | --- |
| 包名 | `set-cookie-parser` |
| 版本 | `2.7.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add set-cookie-parser
```

```cp
import {
    ParsedCookie,
    scpStripQuotes,
    parseSetCookie,
    splitCookiesCount,
    splitCookiesGet,
} from "set-cookie-parser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParsedCookie` | `class` | 字段: `name: string, value: string, path: string, domain: string, expires: string, same_site: string, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `scpStripQuotes` | `(s: string): string` | 调用 `scpStripQuotes` 并返回 `string`。 |
| `parseSetCookie` | `(header: string): ParsedCookie` | 通过 `parseSetCookie` 解析输入数据，返回 `ParsedCookie`。 |
| `splitCookiesCount` | `(combined: string): usize` | 调用 `splitCookiesCount` 并返回 `usize`。 |
| `splitCookiesGet` | `(combined: string, target_idx: usize): string?` | 调用 `splitCookiesGet` 并返回 `string?`。 |

## 最小调用示例

```cp
import { parseSetCookie } from "set-cookie-parser";

pub fn main() {
    let result = parseSetCookie("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
