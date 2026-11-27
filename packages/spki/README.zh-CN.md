[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# spki

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation RFC 5280 SubjectPublicKeyInfo (SPKI) & AlgorithmIdentifier DER/PEM codec (reuses @cap/const-oid and @cap/pem-rfc7468)

## 安装与导入

```bash
cap add spki
```

```cp
import { encodeAlgorithmId } from "spki";
```

## 核心导出 API

- `export fn encodeAlgorithmId(alg: AlgorithmIdentifier, out: []u8): i64;`
- `export fn decodeAlgorithmId(der: &[]u8): AlgIdDecodeResult;`
- `export fn encodeSpkiDer(spki: SubjectPublicKeyInfo, out: []u8): i64;`
- `export fn decodeSpkiDer(der: &[]u8): SpkiDecodeResult;`
- `export fn encodeSpkiPem(spki: SubjectPublicKeyInfo, crlf: bool, out: []u8): i64;`
- `export fn decodeSpkiPem(pem: string): SpkiDecodeResult;`
- `export fn fingerprintSpki(der: &[]u8): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`spki@1.0.0`)，许可证：MIT
