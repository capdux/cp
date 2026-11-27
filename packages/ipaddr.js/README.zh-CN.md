[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `ipaddr.js`

CP 零堆分配 IPv4 / IPv6 解析器、RFC 5952 规范格式化器、CIDR 匹配器、IPv4 映射 IPv6 转换器与 IANA 地址段分类器 —— 对齐 npm [`ipaddr.js`](https://github.com/whitequark/ipaddr.js)（v2）的纯 CP MIT 净室实现。

## 安装与导入

```bash
cap add ipaddr.js
```

```cp
import {
    Ipv4Addr,
    Ipv6Addr,
    CidrV4,
    CidrV6,
    parseIpv4,
    parseIpv6,
    parseCidrV4,
    parseCidrV6,
    isValidIp,
    isValidIpv4,
    isValidIpv6,
    ipKind,
} from "ipaddr.js";
```

## 核心 API

### 顶层解析与断言函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parseIpv4` | `(s: string): Ipv4Addr` | 解析点分十进制 IPv4，使用前检查 `.ok` |
| `parseIpv6` | `(s: string): Ipv6Addr` | 解析压缩、完整或内嵌 IPv4 的 IPv6，使用前检查 `.ok` |
| `parseCidrV4` | `(s: string): CidrV4` | 解析 IPv4 CIDR（如 `"10.0.0.0/8"`） |
| `parseCidrV6` | `(s: string): CidrV6` | 解析 IPv6 CIDR（如 `"2001:db8::/32"`） |
| `isValidIp` / `isValidIpv4` / `isValidIpv6` | `(s: string): bool` | 快速合法性校验 |
| `ipKind` | `(s: string): string` | 返回 `"ipv4"`、`"ipv6"` 或 `"invalid"` |

### `Ipv4Addr`（`octets: [4]u8`, `ok: bool`）

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `Ipv4Addr.fromU32` | `(val: u32): Ipv4Addr` | 从 32 位整数构造 IPv4 地址 |
| `Ipv4Addr.subnetMaskFromPrefixLength` | `(prefix: u8): Ipv4Addr` | 根据前缀长度 `0..=32` 生成子网掩码 |
| `kind` / `range` | `(): string` | 返回 `"ipv4"` 及地址段分类（`"private"`、`"loopback"`、`"unicast"` 等） |
| `toU32` / `toByteArray` | `(): u32` / `(): [4]u8` | 转为大端 `u32` 或 4 字节数组 |
| `equals` | `(other: Ipv4Addr): bool` | 精确相等比较 |
| `toStr` | `(out: []u8): usize` | 写入点分十进制字符串到 `out`，返回写入字节数 |
| `matchCidr` | `(net: Ipv4Addr, prefix: u8): bool` | 判断是否归属于 `net/prefix` 网段 |
| `prefixLengthFromSubnetMask` | `(): i8` | 从子网掩码反算前缀长度（非法掩码返回 `-1`） |
| `toIpv4MappedIpv6` | `(): Ipv6Addr` | 转换为 `::ffff:a.b.c.d` IPv6 地址 |

### `Ipv6Addr`（`parts: [8]u16`, `ok: bool`）与 CIDR 类型

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `Ipv6Addr.kind` / `range` | `(): string` | 返回 `"ipv6"` 及地址段分类（`"loopback"`、`"uniqueLocal"`、`"linkLocal"` 等） |
| `Ipv6Addr.toByteArray` / `equals` | `(): [16]u8` / `(other: Ipv6Addr): bool` | 转为 16 字节网络序数组 / 相等比较 |
| `Ipv6Addr.toStr` / `toNormalizedStr` | `(out: []u8): usize` | 输出 RFC 5952 最短压缩形式（`::`）或完整 8 组十六进制形式 |
| `Ipv6Addr.matchCidr` | `(net: Ipv6Addr, prefix: u8): bool` | 判断是否归属于 `net/prefix` 网段 |
| `Ipv6Addr.isIpv4Mapped` / `toIpv4` | `(): bool` / `(): Ipv4Addr` | 判断是否为 `::ffff:0:0/96` 并提取内嵌 IPv4 |
| `CidrV4.matches` / `CidrV6.matches` | `(ip: Ipv4Addr / Ipv6Addr): bool` | 判断 IP 是否匹配当前 CIDR 网段 |

## 代码示例

### 1. 地址解析、网段分类与 CIDR 匹配

```cp
import { print } from "cap:io";
import { parseIpv4, parseCidrV4 } from "ipaddr.js";

fn main() {
    const ip = parseIpv4("192.168.1.42");
    const cidr = parseCidrV4("192.168.0.0/16");
    if (ip.ok && cidr.ok) {
        print("range:", ip.range());          // "private"
        print("matches:", cidr.matches(ip));  // true
        print("u32:", ip.toU32());            // 3232235818
    }
}
```

### 2. RFC 5952 压缩格式化与 IPv4 映射互转

```cp
import { print } from "cap:io";
import { parseIpv6, parseIpv4 } from "ipaddr.js";

fn main() {
    const v6 = parseIpv6("2001:0db8:0000:0000:0000:0000:0000:0001");
    if (v6.ok) {
        let short_buf: [64]u8 = undefined;
        const n = v6.toStr(short_buf[0..64]);
        print("RFC 5952:", short_buf[0..n]); // "2001:db8::1"
    }

    const mapped = parseIpv4("127.0.0.1").toIpv4MappedIpv6();
    if (mapped.isIpv4Mapped()) {
        const back = mapped.toIpv4();
        print("loopback:", back.range()); // "loopback"
    }
}
```

## 与源包差异及性能亮点

- **零堆分配**：`Ipv4Addr` 与 `Ipv6Addr` 均为极小值类型结构体，字符串格式化直接写入调用方栈缓冲 `[]u8`，在高频网关/防火墙/路由过滤路径零 GC 负担。
- **无异常开销**：解析失败时返回 `.ok == false`，无需 `try/catch`。
- **完整遵循 RFC 5952**：`Ipv6Addr.toStr` 自动选择最长连续全零段（长度 >= 2）压缩为 `::`，小写输出。
