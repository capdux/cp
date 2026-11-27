[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/cidr-tools

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cidr-tools` ([https://github.com/silverwind/cidr-tools](https://github.com/silverwind/cidr-tools))

Zero-allocation IPv4 CIDR block set-algebra engine (`parse`, `format`, `contains`, `overlaps`, `mergeCidrs`, `excludeCidr`) for CP — clean-room MIT parity with npm `cidr-tools` and `ip-cidr`.

纯 CP 零堆分配 IPv4 CIDR 块集合运算引擎，支持相邻/重叠路由自动最小前缀聚合与安全组差集排除，适用于云防火墙、WAF 黑白名单与路由表优化。

## Install / 安装

```bash
cap add @cap/cidr-tools
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Cidr4` | `struct { network: u32, prefix: u8, valid: bool }` | Canonical IPv4 CIDR block |
| `parseIpv4` / `parseCidr4` | `(s: []const u8) -> i64 / Cidr4` | Strict parser (rejects leading zeros & `> 255`; bare IP defaults to `/32`) |
| `formatCidr4` | `(c: Cidr4, dst: []u8) -> usize` | Format `"a.b.c.d/prefix"` |
| `cidrStart` / `cidrEnd` / `cidrHostCount` | `(c: Cidr4) -> u32 / u64` | Block boundaries & address count (handles `/0`..`/32`) |
| `containsIp` / `containsCidr` / `overlaps` | `-> bool` | Containment & overlap predicates |
| `mergeCidrs` | `(inputs: []const Cidr4, out: []Cidr4) -> usize` | Merge overlapping/adjacent blocks into minimal canonical CIDRs |
| `excludeCidr` | `(base: Cidr4, remove: Cidr4, out: []Cidr4) -> usize` | Subtract `remove` from `base` into minimal canonical CIDRs |

## Usage / 使用示例

```cp
import { Cidr4, parseCidr4, mergeCidrs, excludeCidr } from "@cap/cidr-tools";

const pair: [2]Cidr4 = [parseCidr4("192.168.1.0/25"), parseCidr4("192.168.1.128/25")];
let merged: [4]Cidr4 = undefined;
const n = mergeCidrs(pair[0..2], merged[0..4]); // 1 block: 192.168.1.0/24
```
