# `cap:pkcs8` — 零分配 RFC 5958 PKCS#8 & RFC 5280 SPKI DER 密钥容器解析标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

`cap:pkcs8` 是纯 CP、零堆分配、零外部依赖的 ASN.1 DER 密钥容器解析库，支持 RFC 5958 PKCS#8 `PrivateKeyInfo`（v1/v2 `OneAsymmetricKey`）、`EncryptedPrivateKeyInfo`（PBES2/PBKDF2）以及 RFC 5280 `SubjectPublicKeyInfo`（SPKI）。

---

## 快速导入

```typescript
import {
    OID_RSA_ENCRYPTION,
    OID_EC_PUBLIC_KEY,
    OID_P256,
    OID_P384,
    OID_P521,
    OID_ED25519,
    OID_X25519,
    OID_PBES2,
    KeyAlgorithm,
    PrivateKeyInfo,
    EncryptedPrivateKeyInfo,
    SubjectPublicKeyInfo,
    parsePkcs8PrivateKey,
    parsePkcs8Encrypted,
    parseSpkiPublicKey,
} from "cap:crypto";
```

---

## 核心 API

- **OID 常量**：`OID_RSA_ENCRYPTION`, `OID_EC_PUBLIC_KEY`, `OID_P256`, `OID_P384`, `OID_P521`, `OID_ED25519`, `OID_ED448`, `OID_X25519`, `OID_X448`, `OID_PBES2`, `OID_PBKDF2`
- **算法枚举**：`KeyAlgorithm: u8`（`rsa`, `ec_p256`, `ec_p384`, `ec_p521`, `ec_other`, `ed25519`, `ed448`, `x25519`, `x448`, `unknown`）
- **私钥容器**：`parsePkcs8PrivateKey(der_data): PrivateKeyInfo?`（支持 v1/v2 与 `[1]` 可选公钥，附 `rawCurvePrivateKey(): ([]const u8)?` 剥离 Ed25519/X25519/Ed448/X448 内嵌 `OCTET STRING` 返回裸私钥种子）
- **加密私钥容器**：`parsePkcs8Encrypted(der_data): EncryptedPrivateKeyInfo?`（附 `isPbes2(): bool`）
- **公钥容器**：`parseSpkiPublicKey(der_data): SubjectPublicKeyInfo?`（强制校验 `unused_bits == 0`）
