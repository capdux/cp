[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `spki`

CP 零堆分配 RFC 5280 `SubjectPublicKeyInfo`（SPKI）与 `AlgorithmIdentifier` DER/PEM 编解码库 —— 对齐 RustCrypto [`spki`](https://github.com/RustCrypto/formats)（v0.7）的纯 CP MIT 净室实现，底层组合 `const-oid` 与 `pem-rfc7468`。

## 安装与导入

```bash
cap add spki
```

```cp
import {
    SPKI_MAX_KEY_LEN, SPKI_PEM_LABEL,
    SPKI_TAG_BIT_STRING, SPKI_TAG_NULL, SPKI_TAG_SEQUENCE,
    SPKI_OK, SPKI_ERR_BUFFER_TOO_SMALL, SPKI_ERR_INVALID_OID, SPKI_ERR_INVALID_DER,
    SPKI_ERR_INVALID_TAG, SPKI_ERR_TRUNCATED, SPKI_ERR_NON_ZERO_UNUSED_BITS,
    SPKI_ERR_KEY_TOO_LONG, SPKI_ERR_INVALID_PARAMS, SPKI_ERR_INVALID_PEM,
    SPKI_ERR_INVALID_PEM_LABEL, SPKI_ERR_TRAILING_DATA,
    AlgorithmIdentifier, AlgIdDecodeResult, SubjectPublicKeyInfo, SpkiDecodeResult,
    encodeAlgorithmId, decodeAlgorithmId,
    encodeSpkiDer, decodeSpkiDer, encodeSpkiPem, decodeSpkiPem,
    fingerprintSpki,
} from "spki";
```

## 核心 API

### 常量与错误码

| 分类 | 常量 | 说明 |
| :--- | :--- | :--- |
| 格式与标签 | `SPKI_MAX_KEY_LEN` (160), `SPKI_PEM_LABEL` (`"PUBLIC KEY"`), `SPKI_TAG_BIT_STRING` (0x03), `SPKI_TAG_NULL` (0x05), `SPKI_TAG_SEQUENCE` (0x30) | 容量上限、PEM 标签与 DER 标签 |
| 错误码 (`i64`) | `SPKI_OK` (0), `SPKI_ERR_BUFFER_TOO_SMALL` (-1), `SPKI_ERR_INVALID_OID` (-2), `SPKI_ERR_INVALID_DER` (-3), `SPKI_ERR_INVALID_TAG` (-4), `SPKI_ERR_TRUNCATED` (-5), `SPKI_ERR_NON_ZERO_UNUSED_BITS` (-6), `SPKI_ERR_KEY_TOO_LONG` (-7), `SPKI_ERR_INVALID_PARAMS` (-8), `SPKI_ERR_INVALID_PEM` (-9), `SPKI_ERR_INVALID_PEM_LABEL` (-10), `SPKI_ERR_TRAILING_DATA` (-11) | 编码失败返回负值；解码失败写入 `.err` |

### 类型与编解码函数

| 符号 | 签名 / 方法 | 说明 |
| :--- | :--- | :--- |
| `AlgorithmIdentifier` | `empty()`, `fromOid(oid)`, `withNull(oid)`, `withParamOid(oid, param_oid)`, `eq(other): bool` | RFC 5280 算法标识（支持无参数、`NULL` 参数或曲线 OID 参数） |
| `AlgIdDecodeResult` | `ok: bool`, `alg: AlgorithmIdentifier`, `consumed: usize`, `err: i64` | 算法标识解码结果与消耗字节数 |
| `SubjectPublicKeyInfo` | `empty()`, `ed25519(pub_key: []const u8): SpkiDecodeResult`, `p256(pub_key: []const u8): SpkiDecodeResult`, `rsa(pub_key_der: []const u8): SpkiDecodeResult` | 公钥信息容器（`algorithm`、`key_bytes: [160]u8`、`key_len`、`unused_bits`） |
| `SpkiDecodeResult` | `ok: bool`, `spki: SubjectPublicKeyInfo`, `err: i64` | SPKI 构造/解码结果 |
| `encodeAlgorithmId` / `decodeAlgorithmId` | `(alg, out: []u8): i64` / `(der: []const u8): AlgIdDecodeResult` | 独立 `AlgorithmIdentifier` DER 编解码 |
| `encodeSpkiDer` / `decodeSpkiDer` | `(spki, out: []u8): i64` / `(der: []const u8): SpkiDecodeResult` | `SubjectPublicKeyInfo` DER 编解码 |
| `encodeSpkiPem` / `decodeSpkiPem` | `(spki, crlf: bool, out: []u8): i64` / `(pem: string): SpkiDecodeResult` | RFC 7468 PEM 编解码（`-----BEGIN PUBLIC KEY-----`） |
| `fingerprintSpki` | `(der: []const u8): u64` | 计算 SPKI DER 的 64 位快速指纹（用于公钥固定/索引） |

## 代码示例

### 1. 将 Ed25519 公钥编码为 SPKI DER 与 PEM

```cp
import { print } from "cap:io";
import { SubjectPublicKeyInfo, encodeSpkiDer, encodeSpkiPem, fingerprintSpki } from "spki";

fn exportEd25519(raw_pub: []const u8) {
    const built = SubjectPublicKeyInfo.ed25519(raw_pub);
    if (!built.ok) {
        print("invalid key, err:", built.err);
        return;
    }

    let der: [128]u8 = undefined;
    const der_len = encodeSpkiDer(built.spki, der[0..128]);
    if (der_len > 0) {
        const fp = fingerprintSpki(der[0..(der_len as usize)]);
        print("SPKI DER bytes:", der_len, "fingerprint:", fp);
    }

    let pem: [256]u8 = undefined;
    const pem_len = encodeSpkiPem(built.spki, false, pem[0..256]);
    if (pem_len > 0) {
        print(pem[0..(pem_len as usize)]);
    }
}
```

### 2. 解析 PEM 公钥并比对算法标识

```cp
import { print } from "cap:io";
import { decodeSpkiPem, AlgorithmIdentifier } from "spki";
import { ObjectIdentifier, OID_EC_PUBLIC_KEY, OID_PRIME256V1 } from "const-oid";

fn verifyIsP256(pem: string) {
    const res = decodeSpkiPem(pem);
    if (!res.ok) {
        print("PEM decode error:", res.err);
        return;
    }
    const ec_oid = ObjectIdentifier.fromStr(OID_EC_PUBLIC_KEY).oid;
    const p256_oid = ObjectIdentifier.fromStr(OID_PRIME256V1).oid;
    const expected = AlgorithmIdentifier.withParamOid(ec_oid, p256_oid);
    print("is P-256:", res.spki.algorithm.eq(expected), "key_len:", res.spki.key_len);
}
```

## 与源包差异及性能亮点

- **零堆分配**：`SubjectPublicKeyInfo` 在栈上内联 160 字节密钥缓冲（覆盖 Ed25519、P-256/P-384/P-521 等主流公钥）。
- **严格 DER 校验**：校验 `BIT STRING` 未使用比特必须为 0（`SPKI_ERR_NON_ZERO_UNUSED_BITS`）、PEM 标签严格匹配 `"PUBLIC KEY"`、拒绝尾随多余字节（`SPKI_ERR_TRAILING_DATA`）。
- **内置公钥指纹**：`fingerprintSpki` 直接对 DER 字节流计算 64 位指纹，便于缓存与公钥固定校验。
