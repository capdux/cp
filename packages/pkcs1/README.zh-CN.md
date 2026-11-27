[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pkcs1

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 8017 PKCS#1 RSAPublicKey and RSAPrivateKey DER/PEM codec reusing @cap/der-asn1, @cap/pem-rfc7468, and @cap/subtle-ct in pure CP.

## 安装与导入

```bash
cap add pkcs1
```

```cp
import { pk1WriteBigUint } from "pkcs1";
```

## 核心导出 API

- `export fn pk1WriteBigUint(w: &mut DerWriter, raw: &[]u8): bool;`
- `export fn pk1ReadBigUint(r: &mut DerReader, dst: []u8, len_out: &mut usize): i64;`
- `export fn modulusBitLen(pub_key: RsaPublicKey): usize;`
- `export fn encodeRsaPublicKeyDer(pub_key: RsaPublicKey, out: []u8): i64;`
- `export fn decodeRsaPublicKeyDer(der: &[]u8): RsaPublicKeyResult;`
- `export fn encodeRsaPublicKeyPem(pub_key: RsaPublicKey, crlf: bool, out: []u8): i64;`
- `export fn decodeRsaPublicKeyPem(pem: string): RsaPublicKeyResult;`
- `export fn encodeRsaPrivateKeyDer(priv_key: RsaPrivateKey, out: []u8): i64;`
- `export fn decodeRsaPrivateKeyDer(der: &[]u8): RsaPrivateKeyResult;`
- `export fn encodeRsaPrivateKeyPem(priv_key: RsaPrivateKey, crlf: bool, out: []u8): i64;`
- `export fn decodeRsaPrivateKeyPem(pem: string): RsaPrivateKeyResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`pkcs1@1.0.0`)，许可证：MIT
