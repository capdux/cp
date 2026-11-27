[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# forwarded

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 7239 Forwarded & X-Forwarded-For proxy chain parser with anti-spoofing trusted-hop client IP extraction and private IP detection (forwarded / proxy-addr parity) for CP

## 安装与导入

```bash
cap add forwarded
```

```cp
import { isSpace } from "forwarded";
```

## 核心导出 API

- `export fn isSpace(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn trim(s: string): string;`
- `export fn unquote(raw: string): string;`
- `export fn eqIgnoreCase(a: string, lower_lit: string): bool;`
- `export fn stripAddr(raw: string): string;`
- `export fn parseForwardedElement(elem: string): ForwardedHop;`
- `export fn parseForwarded(header: string): ForwardedChain;`
- `export fn parseXForwardedFor(xff: string, remote_addr: string): ForwardedChain;`
- `export fn extractClientIp(xff: string, remote_addr: string, trusted_hops: usize): string;`
- `export fn hexVal(c: u8): i8;`
- `export fn isPrivateIpv4(s: string): bool;`
- `export fn isPrivateIp(ip: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`forwarded@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`forwarded-header-value@1.0.0`)，许可证：MIT
