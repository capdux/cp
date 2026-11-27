[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ipnet

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure-CP IPv4/CIDR subnet calculation engine (parse, format, masks, containment, supernet, host count — crates.io ipnet equivalent)

## 安装与导入

```bash
cap add ipnet
```

```cp
import { ipnParseIpv4Range } from "ipnet";
```

## 核心导出 API

- `export fn ipnParseIpv4Range(s: string, start: usize, end: usize): u32?;`
- `export fn ipnParsePrefixRange(s: string, start: usize, end: usize): u8?;`
- `export fn ipnWriteU8Dec(v: u8, out: []u8, pos: usize): usize;`
- `export fn ipnNetmask(prefix_len: u8): u32;`
- `export fn parseIpv4(s: string): u32?;`
- `export fn formatIpv4(ip: u32, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ipnet@1.0.0`)，许可证：MIT OR Apache-2.0
