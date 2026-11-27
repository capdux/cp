[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# express-bearer-token

Zero-allocation RFC 6750 HTTP Bearer token extractor, JWT structure precheck, constant-time verifier, and WWW-Authenticate challenge formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:express-bearer-token` ([https://github.com/tkellen/js-express-bearer-token](https://github.com/tkellen/js-express-bearer-token))（原库名 `express-bearer-token` → CP 包名 `express-bearer-token`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `express-bearer-token` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add express-bearer-token
```

```cp
import {
    isAuthSpace,
    isB64TokenCore,
    isValidTokenChars,
    extractBearer,
    formatBearer,
    isBase64UrlChar,
    isJwtLike,
    constantTimeEq,
} from "express-bearer-token";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isAuthSpace` | `(b: u8): bool` | 执行 `isAuthSpace` 条件判定，返回 `bool`。 |
| `isB64TokenCore` | `(b: u8): bool` | 执行 `isB64TokenCore` 条件判定，返回 `bool`。 |
| `isValidTokenChars` | `(token: string): bool` | 执行 `isValidTokenChars` 条件判定，返回 `bool`。 |
| `extractBearer` | `(header: string): string` | 调用 `extractBearer` 并返回 `string`。 |
| `formatBearer` | `(token: string, out: []u8): usize` | 通过 `formatBearer` 编码或格式化输出，返回 `usize`。 |
| `isBase64UrlChar` | `(b: u8): bool` | 执行 `isBase64UrlChar` 条件判定，返回 `bool`。 |
| `isJwtLike` | `(token: string): bool` | 执行 `isJwtLike` 条件判定，返回 `bool`。 |
| `constantTimeEq` | `(a: string, b: string): bool` | 调用 `constantTimeEq` 并返回 `bool`。 |
| `verifyBearer` | `(header: string, expected_token: string): bool` | 校验输入合法性，返回 `bool`。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `pushQuotedParam` | `(dst: []u8, pos: usize, first: bool, key: string, val: string): i64` | 写入或更新状态数据。 |
| `formatChallenge` | `(realm: string, err_code: string, err_desc: string, out: []u8): usize` | 通过 `formatChallenge` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidTokenChars } from "express-bearer-token";

pub fn main() {
    let result = isValidTokenChars("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
