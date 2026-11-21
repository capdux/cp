# `cap:http_host_header` — RFC 9110 §7.2 HTTP `Host` / `:authority` 解析与校验库

> 来源：吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9110 Host Header Validator & Parser）

`cap:http_host_header` 提供纯 CP、零外部依赖、零堆分配的 HTTP `Host` 与 HTTP/2 `:authority` 安全校验、端口拆分、回环判定与规范化输出能力。

## 核心类型

### `HostHeader`

```cp
pub struct HostHeader {
    host: []const u8 = "",      // 原始主机名（IPv6 时保留 `[...]`）
    port: u16? = null,
    is_ipv6: bool = false,

    static fn parse(raw: []const u8): HostHeader?
    fn unbracketedHost(): []const u8
    fn effectivePort(is_tls: bool): u16
    fn isLoopback(): bool
    fn formatCanonical(is_tls: bool, out: []u8): ([]const u8)?
}
```

- `HostHeader.parse(raw)`：严格遵循 RFC 9110 §7.2，支持域名、IPv4 与带方括号 IPv6（如 `[::1]:8080`），拒绝空串、空白/控制字符、userinfo `@`、路径 `/`、`?`、`#`、多余冒号、端口 `0` / 前导零（如 `080`）/ `> 65535`。
- `unbracketedHost()`：对 IPv6 剥离外层 `[...]` 返回裸地址（如 `"::1"`），其余原样返回。
- `effectivePort(is_tls)`：无显式端口时 HTTPS 返回 `443`，HTTP 返回 `80`。
- `isLoopback()`：大小写无关识别 `localhost`、`127.0.0.1`、`[::1]`。
- `formatCanonical(is_tls, out)`：将主机名小写化并在端口为协议默认端口时自动省略。

## 使用示例

```cp
import { HostHeader } from "cap:http_host_header";

const h = HostHeader.parse("API.Example.COM:443").?;
let buf: [64]u8 = [0; 64];
const canon = h.formatCanonical(true, buf[0..]).?; // "api.example.com"
```
