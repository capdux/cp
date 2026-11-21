# cap:no_proxy

> 源码吸收自 `/data/dev/bun/src/http/ProxyTunnel.cp`，提供纯 CP、零外部依赖、零堆分配的 HTTP `NO_PROXY` 代理绕过匹配器与 IPv4 CIDR 网段引擎。

## 导入方式

```cp
import { Ipv4Addr, Ipv4Cidr, shouldBypassProxy } from "cap:no_proxy";
```

## 核心类型与 API

### `Ipv4Addr`

- 字段：`octets: [4]u8`
- `Ipv4Addr.parse(s: []const u8): Ipv4Addr?`：严格解析点分十进制 IPv4 地址（拒绝前导零、空段与越界值）。
- `toU32(): u32`：返回大端网络字节序 32 位整数。
- `isLoopback(): bool`：是否属于 `127.0.0.0/8`。
- `isPrivate(): bool`：是否属于 RFC 1918 私网段（`10.0.0.0/8`、`172.16.0.0/12`、`192.168.0.0/16`）。
- `eql(other: Ipv4Addr): bool`：比较两个 IPv4 地址是否相等。

### `Ipv4Cidr`

- 字段：`network: u32`、`prefix_len: u8`
- `Ipv4Cidr.parse(s: []const u8): Ipv4Cidr?`：解析 `a.b.c.d/prefix`（`prefix` 范围 `0..32`）。
- `contains(ip: Ipv4Addr): bool`：判断目标 IPv4 是否命中该 CIDR 网段。

### `shouldBypassProxy(no_proxy_env: []const u8, host: []const u8, port: u16): bool`

按逗号分隔解析 `NO_PROXY` 规则列表并判断目标 `(host, port)` 是否绕过代理：
- `*`：全局绕过。
- 精确主机匹配（大小写无关）与 IPv6 方括号剥离（`[::1]` / `[::1]:8443`）。
- `example.com` / `.example.com` / `*.example.com`：严格按点边界执行子域后缀匹配，防止 `badexample.com` 误匹配。
- 带端口规则：`internal.corp:8443`、`[::1]:3000`。
- IPv4 CIDR 网段匹配：`10.0.0.0/8`、`172.16.0.0/12`、`192.168.0.0/16`、`0.0.0.0/0` 等。

## 使用示例

```cp
import { shouldBypassProxy, Ipv4Addr, Ipv4Cidr } from "cap:no_proxy";

fn example() {
    const rules = "localhost, .internal.corp, 10.0.0.0/8, api.dev:8443, [::1]";
    * = shouldBypassProxy(rules, "svc.internal.corp", 443); // true
    * = shouldBypassProxy(rules, "10.24.0.5", 80);          // true
    * = shouldBypassProxy(rules, "api.dev", 80);            // false（端口不匹配）

    if (cidr := Ipv4Cidr.parse("192.168.1.0/24")) {
        if (ip := Ipv4Addr.parse("192.168.1.42")) {
            * = cidr.contains(ip); // true
        }
    }
}
```
