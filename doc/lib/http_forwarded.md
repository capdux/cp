# `cap:http_forwarded` — RFC 7239 `Forwarded` 与 `X-Forwarded-For` 解析器

> 来源：吸收自 `/data/dev/bun/src/http/HeaderValueIterator.cp`

`cap:http_forwarded` 提供纯 CP、零外部依赖、零堆分配的 RFC 7239 `Forwarded` 多跳代理头部解析、`X-Forwarded-For` 可信代理客户端 IP 提取及 IPv4/IPv6 端口剥离能力。

## 核心类型

### `ForwardedHop`

```cp
pub struct ForwardedHop {
    for_addr: []const u8 = "",
    by_addr: []const u8 = "",
    host: []const u8 = "",
    proto: []const u8 = "",
}
```

### `ForwardedIterator`

```cp
pub struct ForwardedIterator {
    header: []const u8,
    pos: usize = 0,

    static fn init(header: []const u8): ForwardedIterator
    fn next(*_): ForwardedHop?
}
```

- 按逗号切分多跳代理条目，按分号切分 `for` / `by` / `host` / `proto` 参数（参数名大小写无关）。
- 严格保护引号字符串 `"..."` 内部的逗号、分号与 `\"` 转义，自动剥离外层引号（如 `for="[2001:db8::1]:4711"` → `for_addr == "[2001:db8::1]:4711"`）。

## 核心函数

### `parseFirstHop`

```cp
pub fn parseFirstHop(header: []const u8): ForwardedHop?
```

### `extractClientIpFromXff`

```cp
pub fn extractClientIpFromXff(xff_header: []const u8, trusted_proxies: usize): ([]const u8)?
```

- 从 `X-Forwarded-For: client, proxy1, proxy2` 自动裁剪空白并跳过空项，按可信代理跳数 `trusted_proxies` 从右向左定位客户端真实 IP（超出链长度时安全回退至最左侧项）。

### `stripPortFromForwardedFor`

```cp
pub fn stripPortFromForwardedFor(val: []const u8): []const u8
```

- 将 `[2001:db8::1]:4711` → `2001:db8::1`，`192.0.2.60:8080` → `192.0.2.60`，`_hidden` / `unknown` / 裸 IPv6 保持原样。

## 使用示例

```cp
import {
    ForwardedIterator,
    parseFirstHop,
    extractClientIpFromXff,
    stripPortFromForwardedFor,
} from "cap:http_forwarded";

const hop = parseFirstHop("For=\"[2001:db8::1]:4711\";proto=https;host=example.com").?;
const ip = stripPortFromForwardedFor(hop.for_addr); // "2001:db8::1"
const client = extractClientIpFromXff("203.0.113.1, 10.0.0.1, 10.0.0.2", 2).?; // "203.0.113.1"
```
