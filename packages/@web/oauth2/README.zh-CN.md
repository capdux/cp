[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/oauth2

Official OAuth 2.0 + RFC 7636 PKCE authorization plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/oauth2` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/oauth2
```

```cp
import {
    OAuth2Config,
    OaStateSlot,
    oaBytesEql,
    oaWriteByte,
    oaWriteRaw,
    oaHexUpper,
    oaHexLower,
    oaIsUnreserved,
} from "@web/oauth2";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OAuth2Config` | `class` | 字段: `client_id: string, authorize_url: string, token_url: string, redirect_uri: string, scope: string, login_path: string, +1 项` |
| `OaStateSlot` | `class` | 字段: `buf: [OA_MAX_STATE_LEN]u8, len: usize, active: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `oaBytesEql` | `(a: string, b: string): bool` | 调用 `oaBytesEql` 并返回 `bool`。 |
| `oaWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `oaWriteByte` 并返回 `bool`。 |
| `oaWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `oaWriteRaw` 并返回 `bool`。 |
| `oaHexUpper` | `(n: u8): u8` | 调用 `oaHexUpper` 并返回 `u8`。 |
| `oaHexLower` | `(n: u8): u8` | 调用 `oaHexLower` 并返回 `u8`。 |
| `oaIsUnreserved` | `(c: u8): bool` | 调用 `oaIsUnreserved` 并返回 `bool`。 |
| `oaWriteUrlEncoded` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `oaWriteUrlEncoded` 并返回 `bool`。 |
| `oaWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `oaWriteJsonEscaped` 并返回 `bool`。 |
| `oaWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | 调用 `oaWriteU64` 并返回 `bool`。 |
| `oaHasQueryMark` | `(s: string): bool` | 调用 `oaHasQueryMark` 并返回 `bool`。 |
| `oauth2BuildAuthorizeUrl` | `(cfg: OAuth2Config, state: string, code_challenge: string, out_buf: []u8): usize?` | 调用 `oauth2BuildAuthorizeUrl` 并返回 `usize?`。 |
| `oauth2RegisterState` | `(state: string)` | 就地执行 `oauth2RegisterState` 操作。 |
| `oauth2ConsumeState` | `(state: string): bool` | 调用 `oauth2ConsumeState` 并返回 `bool`。 |
| `oauth2ResetStates` | `()` | 就地执行 `oauth2ResetStates` 操作。 |
| `oauth2BuildTokenFormBody` | `(cfg: OAuth2Config, code: string, code_verifier: string, out_buf: []u8): usize?` | 调用 `oauth2BuildTokenFormBody` 并返回 `usize?`。 |
| `oaLoginHandler` | `(ctx: &mut Context): Response` | 调用 `oaLoginHandler` 并返回 `Response`。 |
| `oaCallbackHandler` | `(ctx: &mut Context): Response` | 调用 `oaCallbackHandler` 并返回 `Response`。 |
| `registerOAuth2` | `(app: &mut App, cfg: OAuth2Config): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { oaBytesEql } from "@web/oauth2";

pub fn main() {
    let result = oaBytesEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
