# `cap:csr` — 零分配 RFC 2986 / PKCS#10 证书签发请求 (CSR) DER 解析库

> **模块路径**：`lib/csr/`（入口 `lib/csr/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 RFC 2986 (PKCS#10 v1.7) 严格 ASN.1 DER 规范：校验 `version == 0` 与 `BIT STRING` `unused_bits == 0`，暴露完整含 TL 的 `cri_raw_der` 供直接验签，零拷贝提取 `SubjectPublicKeyInfo` 公钥字节、Subject 多 RDN 属性及 `extensionRequest` SAN 域名/IP 列表。

---

## 一、导入方式

```cp
import {
    OID_CN,
    OID_C,
    OID_L,
    OID_ST,
    OID_O,
    OID_OU,
    OID_EXTENSION_REQUEST,
    OID_SAN,
    CsrRdnAttr,
    CsrSanEntry,
    CsrParsed,
    CsrSubjectIter,
    CsrSanIter,
    parseCsrDer,
    isCsrDer,
} from "cap:csr";
```

---

## 二、核心常量与数据结构

| 符号 | 值 / 字段 | 说明 |
| :--- | :--- | :--- |
| Subject OID | `OID_CN` (`2.5.4.3`), `OID_C` (`2.5.4.6`), `OID_L` (`2.5.4.7`), `OID_ST` (`2.5.4.8`), `OID_O` (`2.5.4.10`), `OID_OU` (`2.5.4.11`) | X.500 Subject 属性 DER OID 字节 |
| 扩展 OID | `OID_EXTENSION_REQUEST` (`1.2.840.113549.1.9.14`), `OID_SAN` (`2.5.29.17`) | PKCS#9 扩展请求与 X.509 SAN OID |
| `CsrParsed` | `cri_raw_der`, `version`, `subject_der`, `spki_der`, `pub_alg_oid`, `pub_key_bytes`, `attributes_der`, `sig_alg_oid`, `signature`; `subjectAttrs()`, `findSubjectAttr(oid)`, `commonName()`, `organization()`, `country()`, `sans()` | 零拷贝 CSR 视图 |
| `CsrRdnAttr` / `CsrSubjectIter` | `oid`, `value` | 迭代 `subject_der` 内全部 `SET -> SEQUENCE { oid, value }` |
| `CsrSanEntry` / `CsrSanIter` | `tag_num: u8`（`1`=rfc822, `2`=dNSName, `6`=URI, `7`=iPAddress）, `value` | 零分配提取 `extensionRequest` 中全部 SAN 项 |

---

## 三、核心 API

- `parseCsrDer(der_data: []const u8): CsrParsed?` — 严格 DER 解析 PKCS#10 CSR。
- `isCsrDer(der_data: []const u8): bool` — 校验是否为合法 DER CSR。

---

## 四、使用示例

```cp
import { parseCsrDer } from "cap:csr";

fn inspectCsr(der: []const u8) {
    if (csr := parseCsrDer(der)) {
        if (cn := csr.commonName()) {
            * = cn;
        }
        let sans = csr.sans();
        while (s := sans.next()) {
            * = s.tag_num;
            * = s.value;
        }
    }
}
```
