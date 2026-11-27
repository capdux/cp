[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cidr-tools

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation IPv4 CIDR parser, formatter, overlap/containment checker, merge, and exclude engine (npm cidr-tools / ip-cidr parity) for CP

## 安装与导入

```bash
cap add cidr-tools
```

```cp
import { parseIpv4 } from "cidr-tools";
```

## 核心导出 API

- `export fn parseIpv4(s: &[]u8): i64;`
- `export fn parseCidr4(s: &[]u8): Cidr4;`
- `export fn formatCidr4(c: Cidr4, dst: []u8): usize;`
- `export fn cidrHostCount(c: Cidr4): u64;`
- `export fn cidrStart(c: Cidr4): u32;`
- `export fn cidrEnd(c: Cidr4): u32;`
- `export fn containsIp(c: Cidr4, ip: u32): bool;`
- `export fn containsCidr(outer: Cidr4, inner: Cidr4): bool;`
- `export fn overlaps(a: Cidr4, b: Cidr4): bool;`
- `export fn mergeCidrs(inputs: &[]Cidr4, out: []Cidr4): usize;`
- `export fn excludeCidr(base: Cidr4, remove: Cidr4, out: []Cidr4): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cidr-tools@1.0.0`)，许可证：BSD-2-Clause
