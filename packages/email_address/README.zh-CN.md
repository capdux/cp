[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# email_address

Zero-heap RFC 5321/5322 email address parser and validator

| 元信息 | 值 |
| --- | --- |
| 包名 | `email_address` |
| 版本 | `0.2.9` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add email_address
```

```cp
import {
    EmailAddress,
    emIsAtext,
    emIsLabelChar,
    emTrim,
    emValidLocal,
    emValidDomain,
    parseEmail,
    isValidEmail,
} from "email_address";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EmailAddress` | `class` | 字段: `local_part: string, domain: string, display_name: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `emIsAtext` | `(b: u8): bool` | 调用 `emIsAtext` 并返回 `bool`。 |
| `emIsLabelChar` | `(b: u8): bool` | 调用 `emIsLabelChar` 并返回 `bool`。 |
| `emTrim` | `(s: string): string` | 调用 `emTrim` 并返回 `string`。 |
| `emValidLocal` | `(local: string): bool` | 调用 `emValidLocal` 并返回 `bool`。 |
| `emValidDomain` | `(domain: string): bool` | 调用 `emValidDomain` 并返回 `bool`。 |
| `parseEmail` | `(input: string): EmailAddress` | 通过 `parseEmail` 解析输入数据，返回 `EmailAddress`。 |
| `isValidEmail` | `(input: string): bool` | 执行 `isValidEmail` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { parseEmail } from "email_address";

pub fn main() {
    let result = parseEmail("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
