[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sec1

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation SEC1 v2.0 elliptic curve point encoding & RFC 5915 ECPrivateKey DER/PEM codec (reuses @cap/const-oid, @cap/pem-rfc7468, and @cap/subtle-ct)

## 安装与导入

```bash
cap add sec1
```

```cp
import { makeEcPrivateKey } from "sec1";
```

## 核心导出 API

- `export fn makeEcPrivateKey(priv_bytes: &[]u8, has_curve: bool, curve_oid: ObjectIdentifier, has_pub_key: bool, pub_key: EncodedPoint): EcPrivateKeyResult;`
- `export fn encodeEcPrivateKeyDer(key: EcPrivateKey, out: []u8): i64;`
- `export fn decodeEcPrivateKeyDer(der: &[]u8): EcPrivateKeyResult;`
- `export fn encodeEcPrivateKeyPem(key: EcPrivateKey, crlf: bool, out: []u8): i64;`
- `export fn decodeEcPrivateKeyPem(pem: string): EcPrivateKeyResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`sec1@1.0.0`)，许可证：MIT
