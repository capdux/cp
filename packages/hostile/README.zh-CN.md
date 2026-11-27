[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hostile

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation POSIX/Windows /etc/hosts parser, case-insensitive forward/family/reverse lookup, and canonical formatter (hostile / Rust hostsfile parity) for CP

## 安装与导入

```bash
cap add hostile
```

```cp
import { isBlank } from "hostile";
```

## 核心导出 API

- `export fn isBlank(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn containsChar(s: string, ch: u8): bool;`
- `export fn parseHosts(content: string): HostsTable;`
- `export fn lookupIp(table: HostsTable, hostname: string): string?;`
- `export fn lookupIpByFamily(table: HostsTable, hostname: string, ipv6: bool): string?;`
- `export fn reverseLookup(table: HostsTable, ip: string): string?;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn formatHosts(table: HostsTable, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`hostile@1.0.0`)，许可证：MIT
