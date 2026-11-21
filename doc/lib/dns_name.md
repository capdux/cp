# cap:dns_name

> 源码吸收自 `/data/dev/bun/src/dns.zig`（RFC 1035 / RFC 1123 规范），提供纯 CP、零外部依赖、零堆分配的 DNS 域名与标签合法性校验、线缆格式标签编解码与子域匹配库。

## 导入方式

```cp
import {
    isValidDnsLabel,
    isValidHostname,
    encodeWireName,
    decodeWireName,
    isSubdomainOf,
    normalizeHostname,
} from "cap:dns_name";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `isValidDnsLabel` | `fn isValidDnsLabel(label: []const u8): bool` | 长度 `1..=63`、`[A-Za-z0-9-]`、首尾不为 `-` |
| `isValidHostname` | `fn isValidHostname(host: []const u8): bool` | 总长 `<= 253`、允许末尾单 `.`、每段合法且无 `..` |
| `encodeWireName` | `fn encodeWireName(host: []const u8, out: []u8): ([]const u8)?` | 编码为小写 RFC 1035 长度前缀标签并以 `0x00` 结尾 |
| `decodeWireName` | `fn decodeWireName(wire: []const u8, out: []u8): ([]const u8)?` | 解码非压缩 RFC 1035 线缆格式，拒绝压缩指针与截断 |
| `isSubdomainOf` | `fn isSubdomainOf(host: []const u8, parent: []const u8): bool` | 大小写无关、忽略末尾 `.`、严格点边界子域匹配 |
| `normalizeHostname` | `fn normalizeHostname(host: []const u8, out: []u8): ([]const u8)?` | 校验并小写化，剥离末尾 `.` |

## 使用示例

```cp
import { encodeWireName, decodeWireName, isSubdomainOf } from "cap:dns_name";

fn example() {
    let wire_buf: [64]u8 = undefined;
    let host_buf: [64]u8 = undefined;
    if (wire := encodeWireName("API.CP.DEV.", &wire_buf)) {
        * = decodeWireName(wire, &host_buf); // "api.cp.dev"
    }
    * = isSubdomainOf("a.b.com", "b.com"); // true
}
```
