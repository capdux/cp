[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jwt-decode

Zero-allocation RFC 7519 JWT segment splitter, Base64URL decoder, exp/nbf leeway validator, and alg:none guard in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jwt-decode` ([https://github.com/auth0/jwt-decode](https://github.com/auth0/jwt-decode)) / `crates.io:jsonwebtoken` ([https://github.com/Keats/jsonwebtoken](https://github.com/Keats/jsonwebtoken))（原库名 `jwt-decode` → CP 包名 `jwt-decode`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `jwt-decode` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add jwt-decode
```

```cp
import {
    JwtParts,
    invalidParts,
    b64UrlVal,
    splitToken,
    decodeBase64Url,
    decodeHeader,
    decodePayload,
    isExpired,
} from "jwt-decode";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `JwtParts` | `class` | 字段: `header_b64: string, payload_b64: string, signature_b64: string, signing_input: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidParts` | `(): JwtParts` | 调用 `invalidParts` 并返回 `JwtParts`。 |
| `b64UrlVal` | `(b: u8): i32` | 调用 `b64UrlVal` 并返回 `i32`。 |
| `splitToken` | `(token: string): JwtParts` | 调用 `splitToken` 并返回 `JwtParts`。 |
| `decodeBase64Url` | `(input: string, out: []u8): i64` | 通过 `decodeBase64Url` 解析输入数据，返回 `i64`。 |
| `decodeHeader` | `(token: string, out: []u8): i64` | 通过 `decodeHeader` 解析输入数据，返回 `i64`。 |
| `decodePayload` | `(token: string, out: []u8): i64` | 通过 `decodePayload` 解析输入数据，返回 `i64`。 |
| `isExpired` | `(exp_unix: i64, now_unix: i64, leeway_secs: i64): bool` | 执行 `isExpired` 条件判定，返回 `bool`。 |
| `isNotYetValid` | `(nbf_unix: i64, now_unix: i64, leeway_secs: i64): bool` | 执行 `isNotYetValid` 条件判定，返回 `bool`。 |
| `isJsonSpace` | `(b: u8): bool` | 执行 `isJsonSpace` 条件判定，返回 `bool`。 |
| `isAlgNone` | `(header_json: string): bool` | 执行 `isAlgNone` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { decodeBase64Url } from "jwt-decode";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeBase64Url("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
