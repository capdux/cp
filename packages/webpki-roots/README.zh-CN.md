[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# webpki-roots

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Mozilla NSS / WebPKI root CA trust anchor catalog and dynamic trust store reusing x509-cert and spki (webpki-roots parity) in pure CP.

## 安装与导入

```bash
cap add webpki-roots
```

```cp
import { wrStrEq } from "webpki-roots";
```

## 核心导出 API

- `export fn wrStrEq(a: string, b: string): bool;`
- `export fn wrBytesEqStr(b: &[]u8, s: string): bool;`
- `export fn wrIsDigit(ch: u8): bool;`
- `export fn wrParseCertNotAfterYear(cert: X509Certificate): u16;`
- `export fn wrMakeRoot(name: string, org: string, country: string, key_alg: u8, not_after_year: u16): TrustAnchorInfo;`
- `export fn mozillaRootAt(idx: usize): TrustAnchorInfo;`
- `export fn lookupMozillaRootByCn(cn: string): TrustAnchorResult;`
- `export fn certSpkiFingerprint(cert: X509Certificate): u64;`
- `export fn wrEmptyAnchors(): [32]TrustAnchorInfo;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`webpki-roots@1.0.0`)，许可证：MIT
