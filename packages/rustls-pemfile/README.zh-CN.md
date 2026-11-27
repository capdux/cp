[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rustls-pemfile

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation streaming TLS PEM bundle reader for X.509 certificates, PKCS#8/PKCS#1/SEC1 private keys, SPKI public keys, CRLs, and CSRs reusing @cap/pem-rfc7468 (rustls-pemfile v2 parity) in pure CP.

## 安装与导入

```bash
cap add rustls-pemfile
```

```cp
import { rpfStrEq } from "rustls-pemfile";
```

## 核心导出 API

- `export fn rpfStrEq(a: string, b: string): bool;`
- `export fn isPrivateKeyKind(kind: u8): bool;`
- `export fn classifyPemLabel(label: string): u8;`
- `export fn readFirstPrivateKey(input: string, der_out: []u8): PemItem;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rustls-pemfile@1.0.0`)，许可证：MIT OR Apache-2.0
