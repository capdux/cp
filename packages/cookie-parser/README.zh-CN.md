[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cookie-parser

纯 CP 实现的零堆分配 HTTP `Cookie` 请求头解析器，支持 HMAC-SHA256 签名 Cookie 验签（`s:`）、多密钥轮换（最多 4 个密钥）及 JSON Cookie（`j:`）前缀剥离，底层组合 `tough-cookie` 与 `cookie-signature`。

- **源包对齐**：对标 `npm` `cookie-parser`（MIT，expressjs/cookie-parser）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，采用栈上固定容量 `ParsedCookies`（最多 32 个普通 + 32 个签名条目）与零拷贝切片。

## 安装与导入

```bash
cap add cookie-parser
```

```cp
import {
    CookieEntry,
    ParsedCookies,
    cprStartsWith2,
    isSignedCookie,
    isJsonCookie,
    stripJsonCookiePrefix,
    signCookieValue,
    signedCookie,
    signedCookieWithSecrets,
    lookupRawCookie,
    cprParseInto,
    parseCookieHeader,
    parseCookieHeaderWithSecrets,
} from "cookie-parser";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `CookieEntry` | `class CookieEntry { name: string, value: string }` | 借用自原始请求头的零拷贝 Cookie 键值对。 |
| `ParsedCookies` | `class ParsedCookies { cookies: [32]CookieEntry, cookie_count: usize, signed_cookies: [32]CookieEntry, signed_count: usize }` | 固定容量解析结果；提供 `ParsedCookies.empty()`、`get(name): string?`、`getSigned(name): string?`、`totalCount(): usize`。 |
| `isSignedCookie` / `isJsonCookie` | `fn isSignedCookie(val: string): bool` / `fn isJsonCookie(val: string): bool` | 判断 `val` 是否以 Express `"s:"` 或 `"j:"` 前缀开头。 |
| `stripJsonCookiePrefix` | `fn stripJsonCookiePrefix(val: string): string?` | 剥离 `"j:"` 前缀返回 JSON 切片，非 JSON Cookie 返回 `null`。 |
| `signCookieValue` | `fn signCookieValue(val: string, secret: string, out: []u8): usize` | 生成 `"s:<val>.<hmac-sha256>"` 写入 `out`，返回写入字节数。 |
| `signedCookie` | `fn signedCookie(val: string, secret: string): string?` | 使用 `secret` 校验 `"s:"` 签名值，通过返回原始值切片，失败返回 `null`。 |
| `signedCookieWithSecrets` | `fn signedCookieWithSecrets(val: string, secrets: [4]string, secret_count: usize): string?` | 按顺序使用最多 4 个轮换密钥校验 `"s:"` 签名值。 |
| `lookupRawCookie` | `fn lookupRawCookie(header: string, name: string): string?` | 直接从 `Cookie` 请求头按名查找原始值，无需构造 `ParsedCookies`。 |
| `parseCookieHeader` | `fn parseCookieHeader(header: string, secret: string): ParsedCookies` | 解析 `Cookie` 请求头，验签通过的 `"s:"` 条目进入 `signed_cookies`，其余进入 `cookies`。 |
| `parseCookieHeaderWithSecrets` | `fn parseCookieHeaderWithSecrets(header: string, secrets: [4]string, secret_count: usize): ParsedCookies` | 支持多密钥轮换的 `Cookie` 请求头解析函数。 |
| `cprParseInto` / `cprStartsWith2` | `fn cprParseInto(header, secrets, secret_count): ParsedCookies` / `fn cprStartsWith2(s, c0, c1): bool` | 底层解析入口与 2 字节前缀辅助函数。 |

## 使用示例

### 1. 签名与解析 HTTP `Cookie` 请求头

```cp
import { signCookieValue, parseCookieHeader } from "cookie-parser";

fn cookieDemo(): void {
    let mut signed_buf: [128]u8 = [0; 128];
    const n = signCookieValue("user-42", "my-secret", signed_buf[0..]);
    // signed_buf[0..n] == "s:user-42.<signature>"

    const parsed = parseCookieHeader("theme=dark; lang=zh", "my-secret");
    const theme = parsed.get("theme"); // Some("dark")
}
```

### 2. 多密钥轮换与 JSON Cookie 提取

```cp
import { parseCookieHeaderWithSecrets, stripJsonCookiePrefix } from "cookie-parser";

fn rotationDemo(header: string): void {
    const secrets: [4]string = ["new-secret", "old-secret", "", ""];
    const parsed = parseCookieHeaderWithSecrets(header, secrets, 2);
    if (let sid = parsed.getSigned("session")) {
        // 使用 "new-secret" 或 "old-secret" 验签成功
    }
    if (let prefs = parsed.get("prefs")) {
        if (let json_str = stripJsonCookiePrefix(prefs)) {
            // 剥离 "j:" 后的原始 JSON 字符串
        }
    }
}
```

## 与源包差异与安全亮点

- **零堆分配栈存储**：`ParsedCookies` 在栈上最多保存 32 个普通 Cookie 与 32 个签名 Cookie，所有键值直接借用自 `header`。
- **恒定时间验签**：底层组合 `cookie-signature` 与 `tough-cookie`，防止签名比对侧信道泄露。
- **原生密钥轮换**：通过 `secrets: [4]string` 与 `secret_count` 支持无停机平滑密钥轮换。
