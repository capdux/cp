[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`ipaddr.js`](https://github.com/whitequark/ipaddr.js)（`MIT`）（原库名 `ipaddr.js`，CP 包名 `ipaddr.js`）。

# ipaddr.js

Zero-allocation IPv4 & IPv6 parser, RFC 5952 canonical formatter, CIDR matcher, IPv4-mapped IPv6 converter, and IANA address range classifier for CP — clean-room MIT parity with npm `ipaddr.js`.

纯 CP 零堆分配 IPv4 / IPv6 网络地址解析、RFC 5952 最长零段压缩格式化、CIDR 子网匹配、IPv4-mapped IPv6 双向转换及 IANA 地址段分类库（Express / `proxy-addr` / Fastify 核心底层依赖对标实现）。

## Install / 安装

```bash
cap add ipaddr.js
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Ipv4Addr` | `struct { octets: [4]u8, ok: bool }` | IPv4 address value with `.toU32()`, `.toStr(out)`, `.matchCidr(net, prefix)`, `.range()`, `.toIpv4MappedIpv6()`, `.equals()`, `.prefixLengthFromSubnetMask()` |
| `Ipv6Addr` | `struct { parts: [8]u16, ok: bool }` | IPv6 address value with `.toStr(out)` (RFC 5952), `.toNormalizedStr(out)`, `.matchCidr(net, prefix)`, `.isIpv4Mapped()`, `.toIpv4()`, `.range()`, `.equals()` |
| `CidrV4` / `CidrV6` | `struct { addr, prefix: u8, ok: bool }` | Parsed CIDR blocks with `.matches(ip)` |
| `parseIpv4` | `(s: string): Ipv4Addr` | Strict dotted-decimal IPv4 parser (rejects leading zeros, `> 255`, trailing chars) |
| `parseIpv6` | `(s: string): Ipv6Addr` | Full RFC 4291 IPv6 parser (`::` compression, embedded IPv4, case-insensitive hex) |
| `parseCidrV4` / `parseCidrV6` | `(s: string): CidrV4 / CidrV6` | Strict CIDR parser (`0..32` / `0..128`) |
| `isValidIp` / `isValidIpv4` / `isValidIpv6` | `(s: string): bool` | Fast zero-allocation IP validation predicates |
| `ipKind` | `(s: string): string` | Returns `"ipv4"`, `"ipv6"`, or `"invalid"` |

## Usage / 使用示例

```cp
import { parseIpv4, parseIpv6, parseCidrV4, isValidIp } from "ipaddr.js";

const v4 = parseIpv4("192.168.1.42");
const rng = v4.range(); // "private"
const cidr = parseCidrV4("192.168.0.0/16");
const hit = cidr.matches(v4); // true

const v6 = parseIpv6("2001:0db8:0000:0000:0000:0000:0000:0001");
let buf: [48]u8 = undefined;
const n = v6.toStr(buf[0..48]); // "2001:db8::1"
```
