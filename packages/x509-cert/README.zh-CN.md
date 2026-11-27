[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# x509-cert

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation RFC 5280 X.509 v1/v2/v3 certificate DER & PEM codec (reuses @cap/der-asn1, @cap/const-oid, @cap/spki, and @cap/pem-rfc7468)

## 安装与导入

```bash
cap add x509-cert
```

```cp
import { encodeX509Der } from "x509-cert";
```

## 核心导出 API

- `export fn encodeX509Der(cert: X509Certificate, out: []u8): i64;`
- `export fn decodeX509Der(der: &[]u8): X509DecodeResult;`
- `export fn encodeX509Pem(cert: X509Certificate, crlf: bool, out: []u8): i64;`
- `export fn decodeX509Pem(pem: string): X509DecodeResult;`
- `export fn isSelfSigned(cert: X509Certificate): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`x509-cert@1.0.0`)，许可证：MIT
