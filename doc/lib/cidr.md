# `cap:cidr` — IPv4/IPv6 CIDR 解析、子网切分、超网聚合与 LPM 路由库

> **模块路径**: `lib/cidr/`  
> **导入协议**: `import { Ipv4Addr, Ipv6Addr, Cidr4, Cidr6, LpmTable4, CidrError } from "cap:cidr";`  
> **动态挂载**: `import cap from "cap";` -> `cap.cidr.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:cidr` 是纯 CP 实现、零 C/POSIX 外部依赖（无 `std.posix`、`std.net` 或 `@import("bun")`）的网络地址与路由计算库，覆盖工业级 IPv4/IPv6 CIDR 全生命周期操作：

1. **IPv4 地址与分类 (`Ipv4Addr`)**：
   - 严格点分十进制解析（拒绝前导零八进制歧义与越界字节）与格式化输出。
   - 内置 RFC 1918 私有网段（`10.0.0.0/8`、`172.16.0.0/12`、`192.168.0.0/16`）、环回地址（`127.0.0.0/8`）与链路本地地址（`169.254.0.0/16`）判定。
2. **IPv6 地址与 RFC 5952 规范化 (`Ipv6Addr`)**：
   - 完整支持 `::` 零压缩解析、双 `::` 唯一性校验、前导/尾随 `::` 省略与大小写混合十六进制。
   - `formatCanonical` 严格遵循 RFC 5952 最长连续零段压缩输出（单组零不压缩、等长取最左）。
   - 支持环回（`::1`）与唯一本地地址（`fc00::/7`）判定。
3. **IPv4/IPv6 CIDR 块运算 (`Cidr4` / `Cidr6`)**：
   - `Cidr4.parse` 自动屏蔽主机位归一化网络地址；`Cidr4.parseStrict` 严格要求主机位全为 0。
   - 支持包含检测 `contains`、重叠检测 `overlaps`、`network`、`broadcast`/`lastAddr` 及 `hostCount`（`u64` 完整覆盖 `/0` 至 `/32`）。
   - `splitSubnets` 支持将任意前缀切分为更细粒度的子网切片；`summarize` 实现 O(N log N) 去重、被包含子网裁剪与相邻兄弟前缀递归超网聚合。
4. **IPv4 最长前缀匹配路由表 (`LpmTable4<V>`)**：
   - 基于二进制基数树（Binary Radix Trie），支持 `/0` 默认兜底路由至 `/32` 主机路由的 O(32) 常数时间最长前缀匹配。

---

## 二、 核心 API 速查表 (`lib/cidr/cidr.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `CidrError` | `InvalidIpAddress`, `InvalidPrefixLength`, `InvalidCidrNotation`, `InvalidSubnetSplit`, `OutOfMemory` |
| `Ipv4Addr.parse(str)` | 解析严格点分十进制 IPv4 地址 |
| `Ipv4Addr.format(allocator)` | 格式化为 `"a.b.c.d"` 字符串（调用方负责 `allocator.free`） |
| `Ipv4Addr.isPrivate()` / `isLoopback()` / `isLinkLocal()` | RFC 1918 私有、环回（`127.0.0.0/8`）、链路本地（`169.254.0.0/16`）判定 |
| `Ipv6Addr.parse(str)` | 解析 IPv6 地址（完整支持 `::` 零压缩与双 `::` 校验） |
| `Ipv6Addr.formatCanonical(allocator)` | 按 RFC 5952 输出最短规范小写字符串（调用方负责 `allocator.free`） |
| `Ipv6Addr.isLoopback()` / `isUniqueLocal()` | 环回（`::1`）与 ULA（`fc00::/7`）判定 |
| `Cidr4.parse(str)` / `Cidr4.parseStrict(str)` | 解析 IPv4 CIDR（`parse` 自动归一化主机位，`parseStrict` 拒绝非零主机位） |
| `Cidr4.contains(ip)` / `overlaps(other)` | IP 归属与 CIDR 重叠判定 |
| `Cidr4.network()` / `broadcast()` / `hostCount()` | 网络地址、广播地址与总地址数（`u64`，支持 `/0` = `4294967296`） |
| `Cidr4.splitSubnets(allocator, new_prefix_len)` | 切分为 `2^(new_prefix_len - prefix_len)` 个子网 |
| `Cidr4.summarize(allocator, cidrs)` | 去重包含子网并合并相邻兄弟前缀为超网 |
| `Cidr6.parse(str)` / `contains` / `overlaps` / `network` / `lastAddr` | IPv6 CIDR 解析、归属、重叠与首尾地址计算 |
| `LpmTable4<V>.init(allocator)` / `deinit()` | 初始化与释放 IPv4 LPM 路由表 |
| `LpmTable4<V>.insert(cidr, value)` / `lookup(ip)` | 插入/更新路由条目与最长前缀匹配查询（返回 `V?`） |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { Ipv4Addr, Ipv6Addr, Cidr4, Cidr6, LpmTable4 } from "cap:cidr";

fn demo(alloc: std.mem.Allocator): !void {
    const net = Cidr4.parse("192.168.1.42/24")?;
    const ip = Ipv4Addr.parse("192.168.1.100")?;
    const inside = net.contains(ip); // true

    const subs = net.splitSubnets(alloc, 26)?;
    defer alloc.free(subs); // 4 个 /26 子网

    const agg = Cidr4.summarize(alloc, subs)?;
    defer alloc.free(agg); // 聚合还原为 [192.168.1.0/24]

    let rt = LpmTable4<u32>.init(alloc);
    defer rt.deinit();
    rt.insert(Cidr4.parse("0.0.0.0/0")?, 1)?;
    rt.insert(Cidr4.parse("10.20.0.0/16")?, 2)?;
    rt.insert(Cidr4.parse("10.20.30.0/24")?, 3)?;
    const gw = rt.lookup(Ipv4Addr.parse("10.20.30.40")?); // 3
    _ = inside;
    _ = gw;
}
```
