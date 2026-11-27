[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# http-status-codes

Zero-allocation RFC 9110, WebDAV, and Cloudflare HTTP status code constants, bidirectional phrase lookup, and semantic predicates for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`http-status-codes`](https://github.com/prettymuchbryce/http-status-codes)（`MIT`）（原库名 `http-status-codes`，CP 包名 `http-status-codes`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `http-status-codes` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add http-status-codes
```

```cp
import {
    StatusClass,
    reasonPhrase,
    isSep,
    eqNormalizedPhrase,
    codeFromPhrase,
    statusClass,
    isValidStatus,
    isKnownStatus,
} from "http-status-codes";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `StatusClass` | `Invalid, Informational, Success, Redirection, ClientError, ServerError` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `reasonPhrase` | `(code: u16): string` | 调用 `reasonPhrase` 并返回 `string`。 |
| `isSep` | `(c: u8): bool` | 执行 `isSep` 条件判定，返回 `bool`。 |
| `eqNormalizedPhrase` | `(input: string, target: string): bool` | 调用 `eqNormalizedPhrase` 并返回 `bool`。 |
| `codeFromPhrase` | `(phrase: string): u16?` | 调用 `codeFromPhrase` 并返回 `u16?`。 |
| `statusClass` | `(code: u16): StatusClass` | 调用 `statusClass` 并返回 `StatusClass`。 |
| `isValidStatus` | `(code: u16): bool` | 执行 `isValidStatus` 条件判定，返回 `bool`。 |
| `isKnownStatus` | `(code: u16): bool` | 执行 `isKnownStatus` 条件判定，返回 `bool`。 |
| `isInformational` | `(code: u16): bool` | 执行 `isInformational` 条件判定，返回 `bool`。 |
| `isSuccess` | `(code: u16): bool` | 执行 `isSuccess` 条件判定，返回 `bool`。 |
| `isRedirect` | `(code: u16): bool` | 执行 `isRedirect` 条件判定，返回 `bool`。 |
| `isClientError` | `(code: u16): bool` | 执行 `isClientError` 条件判定，返回 `bool`。 |
| `isServerError` | `(code: u16): bool` | 执行 `isServerError` 条件判定，返回 `bool`。 |
| `isEmptyBody` | `(code: u16): bool` | 执行 `isEmptyBody` 条件判定，返回 `bool`。 |
| `isRetryable` | `(code: u16): bool` | 执行 `isRetryable` 条件判定，返回 `bool`。 |
| `isCacheableByDefault` | `(code: u16): bool` | 执行 `isCacheableByDefault` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { eqNormalizedPhrase } from "http-status-codes";

pub fn main() {
    let result = eqNormalizedPhrase("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
