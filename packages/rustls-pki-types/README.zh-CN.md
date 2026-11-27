[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rustls-pki-types

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation TLS/PKI core types (DnsName, ServerName, UnixTime, PEM object detection & DER decoding via pem-rfc7468) in pure CP (rustls-pki-types v1 parity).

## 安装与导入

```bash
cap add rustls-pki-types
```

```cp
import { pkiIsDnsChar } from "rustls-pki-types";
```

## 核心导出 API

- `export fn pkiIsDnsChar(ch: u8): bool;`
- `export fn pkiEqIgnoreCase(a: string, b: string): bool;`
- `export fn pkiClassifyPemLabel(label: &[]u8): u8;`
- `export fn pkiParseIpv4Slice(s: string, start: usize, end: usize, out: &mut [4]u8): bool;`
- `export fn pkiParseIpv6Slice(input: string, out: &mut [16]u8): bool;`
- `export fn isValidDnsName(input: string): bool;`
- `export fn parseDnsName(input: string): DnsName;`
- `export fn parseServerName(input: string): ServerName;`
- `export fn detectPemKind(pem: string): u8;`
- `export fn decodePemDer(pem: string, der_out: []u8): PkiPemResult;`
- `export fn pemKindLabel(kind: u8): string;`
- `export fn isPrivateKeyKind(kind: u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rustls-pki-types@1.0.0`)，许可证：MIT
