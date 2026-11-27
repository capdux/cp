[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pkcs8

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 5958 PKCS#8 PrivateKeyInfo / OneAsymmetricKey & RFC 8410 Ed25519 DER/PEM codec reusing @cap/spki, @cap/const-oid, @cap/pem-rfc7468, and @cap/subtle-ct in pure CP.

## 安装与导入

```bash
cap add pkcs8
```

```cp
import { pk8ZeroPrivBuf } from "pkcs8";
```

## 核心导出 API

- `export fn pk8ZeroPrivBuf(): [160]u8;`
- `export fn pk8ZeroPubBuf(): [64]u8;`
- `export fn pk8DerLenSize(n: usize): usize;`
- `export fn pk8WriteDerLen(out: []u8, pos: usize, n: usize): usize;`
- `export fn pk8ReadDerLen(der: &[]u8, pos: usize): Pk8LenDecode;`
- `export fn pk8StrEqBytes(s: string, b: &[]u8): bool;`
- `export fn encodePkcs8Der(info: PrivateKeyInfo, out: []u8): i64;`
- `export fn decodePkcs8Der(der: &[]u8): Pkcs8DecodeResult;`
- `export fn encodePkcs8Pem(info: PrivateKeyInfo, crlf: bool, out: []u8): i64;`
- `export fn decodePkcs8Pem(pem: string): Pkcs8DecodeResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`pkcs8@1.0.0`)，许可证：MIT
