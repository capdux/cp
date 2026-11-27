[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rustls-webpki

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation WebPKI X.509 certificate verification engine (RFC 6125 DNS hostname matching, RFC 5280 validity & chain verification; reuses x509-cert)

## 安装与导入

```bash
cap add rustls-webpki
```

```cp
import { verifyDnsHostnameBytes } from "rustls-webpki";
```

## 核心导出 API

- `export fn verifyDnsHostnameBytes(pattern: &[]u8, hostname: &[]u8): bool;`
- `export fn verifyDnsHostname(pattern: string, hostname: string): bool;`
- `export fn parseX509TimeUtc(time_bytes: &[]u8): X509TimeResult;`
- `export fn verifyValidityWindow(validity: X509Validity, now_unix: u64): i32;`
- `export fn verifyValidityAt(cert: X509Certificate, now_unix: u64): i32;`
- `export fn verifyCertForDnsName(cert: X509Certificate, hostname: string, now_unix: u64): i32;`
- `export fn verifyDerCertForDnsName(der: &[]u8, hostname: string, now_unix: u64): i32;`
- `export fn verifyChainLink(leaf: X509Certificate, issuer: X509Certificate): bool;`
- `export fn verifyTrustAnchor(anchor: X509Certificate, now_unix: u64): i32;`
- `export fn verifyEndEntityChain(leaf: X509Certificate, issuer: X509Certificate, hostname: string, now_unix: u64): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rustls-webpki@1.0.0`)，许可证：MIT
