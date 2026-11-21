# `cap:pkcs7` — 零分配 RFC 5652 CMS / PKCS#7 ASN.1 DER 解析库

> **模块路径**：`lib/pkcs7/`（入口 `lib/pkcs7/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 RFC 5652 (CMS) 与 RFC 2315 (PKCS#7) 严格 ASN.1 DER 编码规范：支持 `ContentInfo` OID 分发与 `[0] EXPLICIT` 解包、`SignedData`（附带内容与 detached 签名）解析、X.509 `CertificateSet` 证书链迭代及 `SignerInfo`（摘要/签名算法 OID、`signedAttrs [0]`、原始签名值）零拷贝迭代。

---

## 一、导入方式

```cp
import {
    OID_DATA,
    OID_SIGNED_DATA,
    OID_ENVELOPED_DATA,
    OID_DIGESTED_DATA,
    OID_ENCRYPTED_DATA,
    Pkcs7ContentType,
    Pkcs7ContentInfo,
    Pkcs7SignedData,
    Pkcs7SignerInfo,
    Pkcs7CertIter,
    Pkcs7SignerIter,
    parseContentInfo,
    isPkcs7SignedData,
    parseSignedData,
} from "cap:pkcs7";
```

---

## 二、核心常量与数据结构

| 符号 | 值 / 字段 | 说明 |
| :--- | :--- | :--- |
| OID 常量 | `OID_DATA`, `OID_SIGNED_DATA`, `OID_ENVELOPED_DATA`, `OID_DIGESTED_DATA`, `OID_ENCRYPTED_DATA` | `1.2.840.113549.1.7.{1,2,3,5,6}` 原始 DER OID 编码字节 |
| `Pkcs7ContentType: u8` | `data`, `signed_data`, `enveloped_data`, `digested_data`, `encrypted_data`, `unknown` | CMS `ContentInfo.contentType` 枚举 |
| `Pkcs7ContentInfo` | `content_type`, `content_type_oid`, `content_der` | 外层 `ContentInfo` 零拷贝视图（已剥离 `[0] EXPLICIT` 包装） |
| `Pkcs7SignedData` | `version`, `digest_algs_set`, `encap_content_type_oid`, `encap_content`, `is_detached`, `certificates_der`, `crls_der`, `signer_infos_set`; `certs()`, `signers()` | `SignedData` 零拷贝视图 |
| `Pkcs7SignerInfo` | `version`, `digest_alg_oid`, `sig_alg_oid`, `signature`, `has_signed_attrs` | 单个 `SignerInfo` 零拷贝视图 |

---

## 三、核心 API

- `parseContentInfo(der_data: []const u8): Pkcs7ContentInfo?` — 严格 DER 解析外层 `ContentInfo`。
- `isPkcs7SignedData(der_data: []const u8): bool` — 校验是否为携带内容的 `id-signedData`。
- `parseSignedData(der_data: []const u8): Pkcs7SignedData?` — 直接接收完整 `ContentInfo` DER 并解包 `SignedData`。
- `Pkcs7CertIter` — 零拷贝迭代 `certificates_der` 中每张完整 DER X.509 证书。
- `Pkcs7SignerIter` — 零分配迭代 `signer_infos_set` 中每个 `Pkcs7SignerInfo`。

---

## 四、使用示例

```cp
import { parseSignedData } from "cap:pkcs7";

fn inspectPkcs7(der: []const u8) {
    if (sd := parseSignedData(der)) {
        let certs = sd.certs();
        while (cert_der := certs.next()) {
            * = cert_der;
        }
        let signers = sd.signers();
        while (si := signers.next()) {
            * = si.digest_alg_oid;
            * = si.sig_alg_oid;
            * = si.signature;
        }
    }
}
```
