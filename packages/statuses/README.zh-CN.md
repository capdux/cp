[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# statuses

Zero-heap RFC 9110 / IANA HTTP status code lookup, reason phrases, reverse lookup, and category predicates (npm statuses parity).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:statuses` ([https://github.com/jshttp/statuses](https://github.com/jshttp/statuses))

| 元信息 | 值 |
| --- | --- |
| 包名 | `statuses` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add statuses
```

```cp
import {
    message,
    codeFromMessage,
    isValid,
    isInformational,
    isSuccess,
    isRedirect,
    isEmpty,
    isRetry,
} from "statuses";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `message` | `(code: u16): string` | 调用 `message` 并返回 `string`。 |
| `codeFromMessage` | `(msg: string): u16` | 调用 `codeFromMessage` 并返回 `u16`。 |
| `isValid` | `(code: u16): bool` | 执行 `isValid` 条件判定，返回 `bool`。 |
| `isInformational` | `(code: u16): bool` | 执行 `isInformational` 条件判定，返回 `bool`。 |
| `isSuccess` | `(code: u16): bool` | 执行 `isSuccess` 条件判定，返回 `bool`。 |
| `isRedirect` | `(code: u16): bool` | 执行 `isRedirect` 条件判定，返回 `bool`。 |
| `isEmpty` | `(code: u16): bool` | 执行 `isEmpty` 条件判定，返回 `bool`。 |
| `isRetry` | `(code: u16): bool` | 执行 `isRetry` 条件判定，返回 `bool`。 |
| `isClientError` | `(code: u16): bool` | 执行 `isClientError` 条件判定，返回 `bool`。 |
| `isServerError` | `(code: u16): bool` | 执行 `isServerError` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { codeFromMessage } from "statuses";

pub fn main() {
    let result = codeFromMessage("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
