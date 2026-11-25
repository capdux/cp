# `@cap/cookie` — 零分配 RFC 6265 HTTP Cookie 解析与 `Set-Cookie` 构造器

> 独立 Clean-Room 纯 CP 实现，遵循 RFC 6265 与 CHIPS `Partitioned` Cookie 规范（对标 npm `cookie` [周下载 110M+] 与 Rust `cookie` crate）| 许可协议：MIT

## 概述

`@cap/cookie` 提供纯 CP、零外部依赖、零堆分配、防头注入的 HTTP Cookie 编解码器：
- **零拷贝请求解析**：`CookieIter` / `getCookie` / `countCookies`，自动修剪 OWS 空白并剥离 RFC 6265 双引号包裹；
- **完整 `Set-Cookie` 序列化**：支持 `Max-Age`、`Domain`、`Path`、`Expires`、`HttpOnly`、`Secure`、`Partitioned`、`SameSite=Strict|Lax|None` 以及便捷清退函数 `serializeClearCookie`；
- **严格防注入校验**：拒绝非法 token 字符、控制字符、CRLF 与属性内分号注入。

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `CookieIter` (`init` / `next`) | 零拷贝遍历 `Cookie` 请求头中的所有 `name=value` 对 |
| `getCookie(header, name): ([]const u8)?` | 按名称精确查找 Cookie 值 |
| `countCookies(header): usize` | 统计合法 Cookie 个数 |
| `SetCookieOptions` / `SameSite` | `Set-Cookie` 响应头属性配置结构 |
| `serializeSetCookie(name, value, opts, out)` | 严格校验并序列化 `Set-Cookie` 头 |
| `serializeClearCookie(name, path, out)` | 构造立即过期的清除 Cookie 响应头（`Max-Age=0` + Epoch Expires） |
| `isValidCookieName` / `isValidCookieValue` | RFC 6265 名称与值字符集校验 |

## 使用示例

### 示例 1：解析请求头 `Cookie`

```cp
import { getCookie, CookieIter } from "@cap/cookie";

fn demoParse() {
    const hdr = "sid=abc123; theme=\"dark\"; lang=zh-CN";
    const sid = getCookie(hdr, "sid").?;     // "abc123"
    const theme = getCookie(hdr, "theme").?; // "dark" (自动剥离双引号)
    * = sid;
    * = theme;
}
```

### 示例 2：构造安全会话 `Set-Cookie` 响应头

```cp
import { serializeSetCookie, SetCookieOptions, SameSite } from "@cap/cookie";

fn demoSerialize() {
    let buf: [256]u8 = [0; 256];
    const hdr = serializeSetCookie(
        "session_id",
        "tok_9f8e7d",
        SetCookieOptions{
            path: "/",
            max_age: 3600,
            http_only: true,
            secure: true,
            same_site: SameSite.Lax,
        },
        buf[0..],
    ) catch return;
    // "session_id=tok_9f8e7d; Max-Age=3600; Path=/; HttpOnly; Secure; SameSite=Lax"
    * = hdr;
}
```
