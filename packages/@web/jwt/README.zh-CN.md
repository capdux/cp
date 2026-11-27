[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/jwt

Official zero-heap HS256 JWT authentication plugin for cap:web (pure-CP SHA-256, HMAC-SHA256, Base64URL)

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/jwt` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/jwt
```

```cp
import {
    Sha256Hasher,
    JwtConfig,
    rotr32,
    sha256Compress,
    sha256,
    hmacSha256,
    base64UrlEncode,
    base64UrlCharVal,
} from "@web/jwt";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Sha256Hasher` | `class` | 字段: `state: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64` · 方法: `init, update, finalize` |
| `JwtConfig` | `class` | 字段: `secret: string, issuer: string, header_name: string, optional_auth: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rotr32` | `(x: u32, n: u5): u32` | 调用 `rotr32` 并返回 `u32`。 |
| `sha256Compress` | `(state: &mut [8]u32, block: &[64]u8)` | 就地执行 `sha256Compress` 操作。 |
| `sha256` | `(data: &[]u8, out: &mut [32]u8)` | 就地执行 `sha256` 操作。 |
| `hmacSha256` | `(key: string, msg: &[]u8, out: &mut [32]u8)` | 就地执行 `hmacSha256` 操作。 |
| `base64UrlEncode` | `(input: &[]u8, out_buf: []u8): usize?` | 调用 `base64UrlEncode` 并返回 `usize?`。 |
| `base64UrlCharVal` | `(ch: u8): u8?` | 调用 `base64UrlCharVal` 并返回 `u8?`。 |
| `base64UrlDecode` | `(input: string, out_buf: []u8): usize?` | 调用 `base64UrlDecode` 并返回 `usize?`。 |
| `jwtConstantTimeEql` | `(a: &[]u8, b: &[]u8): bool` | 调用 `jwtConstantTimeEql` 并返回 `bool`。 |
| `jwtAsciiLower` | `(c: u8): u8` | 调用 `jwtAsciiLower` 并返回 `u8`。 |
| `jwtAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `jwtAsciiEqlIgnoreCase` 并返回 `bool`。 |
| `jwtTrimSpaces` | `(s: string): string` | 调用 `jwtTrimSpaces` 并返回 `string`。 |
| `jwtContainsSubstr` | `(haystack: string, needle: string): bool` | 调用 `jwtContainsSubstr` 并返回 `bool`。 |
| `jwtWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `jwtWriteRaw` 并返回 `bool`。 |
| `signHs256` | `(secret: string, payload_json: string, out_buf: []u8): usize?` | 调用 `signHs256` 并返回 `usize?`。 |
| `verifyHs256` | `(secret: string, token: string, out_payload: []u8): usize?` | 校验输入合法性，返回 `usize?`。 |
| `jwtCheckIssuerClaim` | `(payload: string, issuer: string): bool` | 调用 `jwtCheckIssuerClaim` 并返回 `bool`。 |
| `configure` | `(cfg: JwtConfig)` | 就地执行 `configure` 操作。 |
| `authHook` | `(ctx: &mut Context): bool` | 调用 `authHook` 并返回 `bool`。 |
| `createJwtPlugin` | `(cfg: JwtConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerJwt` | `(app: &mut App, cfg: JwtConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { base64UrlDecode } from "@web/jwt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = base64UrlDecode("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
