[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# proxy-addr

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation reverse-proxy client IP resolver and trusted proxy chain validator (npm proxy-addr parity)

## 安装与导入

```bash
cap add proxy-addr
```

```cp
import { isLoopback } from "proxy-addr";
```

## 核心导出 API

- `export fn isLoopback(ip: string): bool;`
- `export fn isLinkLocal(ip: string): bool;`
- `export fn isUniqueLocal(ip: string): bool;`
- `export fn isTrustedBuiltin(ip: string, preset: u8): bool;`
- `export fn parseXffAddresses(remote_addr: string, xff_header: string, out: []IpSpan): usize;`
- `export fn resolveClientIp(remote_addr: string, xff_header: string, trust_preset: u8, max_hops: usize, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`proxy-addr@1.0.0`)，许可证：MIT
