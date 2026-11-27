[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ipaddr.js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation IPv4 & IPv6 parser, RFC 5952 formatter, CIDR matcher, IPv4-mapped IPv6 converter, and address range classifier (npm ipaddr.js parity) for CP

## 安装与导入

```bash
cap add ipaddr.js
```

```cp
import { parseIpv4 } from "ipaddr.js";
```

## 核心导出 API

- `export fn parseIpv4(s: string): Ipv4Addr;`
- `export fn parseIpv6(s: string): Ipv6Addr;`
- `export fn parseCidrV4(s: string): CidrV4;`
- `export fn parseCidrV6(s: string): CidrV6;`
- `export fn isValidIp(s: string): bool;`
- `export fn isValidIpv4(s: string): bool;`
- `export fn isValidIpv6(s: string): bool;`
- `export fn ipKind(s: string): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ipaddr.js@1.0.0`)，许可证：MIT
