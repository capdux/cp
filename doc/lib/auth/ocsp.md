# `cap:ocsp` — 零分配 RFC 6960 OCSP 请求与响应 DER 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:auth`，导入请使用 `import { ... } from "cap:auth"`。

> **模块路径**：`lib/ocsp/`（入口 `lib/ocsp/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 RFC 6960 Online Certificate Status Protocol (OCSP) 严格 ASN.1 DER 规范：自动解包 `OCSPResponse` 与内嵌 `BasicOCSPResponse`（区分 `byName [1]` / `byKey [2]` ResponderID、迭代 `good`/`revoked`/`unknown` 三态 `SingleResponse`、暴露 `tbs_raw_der` 供直接验签），并支持 `OCSPRequest` 多 `CertID` 零分配迭代。

---

## 一、导入方式

```cp
import {
    OID_OCSP_BASIC,
    OcspResponseStatus,
    ocspResponseStatusFromU8,
    OcspCertStatus,
    OcspCertId,
    OcspSingleResponse,
    OcspResponse,
    OcspSingleIter,
    OcspRequest,
    OcspReqIter,
    parseOcspResponseDer,
    parseOcspRequestDer,
} from "cap:auth";
```

---

## 二、核心常量、枚举与数据结构

| 符号 | 值 / 字段 | 说明 |
| :--- | :--- | :--- |
| `OID_OCSP_BASIC` | `1.3.6.1.5.5.7.48.1.1` | `id-pkix-ocsp-basic` DER OID 字节 |
| `OcspResponseStatus: u8` | `successful=0`, `malformed_request=1`, `internal_error=2`, `try_later=3`, `sig_required=5`, `unauthorized=6`, `unknown=255` | OCSP 顶层响应状态 |
| `OcspCertStatus: u8` | `good=0`, `revoked=1`, `unknown_status=2` | 证书吊销三态 |
| `OcspCertId` | `hash_alg_oid`, `issuer_name_hash`, `issuer_key_hash`, `serial_bytes` | 证书标识（自动剥离序列号前导 `0x00`） |
| `OcspResponse` | `status`, `is_basic`, `tbs_raw_der`, `produced_at`, `responder_is_by_key`, `responder_id`, `responses_der`, `sig_alg_oid`, `signature`, `certs_der`; `singleResponses()` | 零拷贝 OCSP 响应视图 |
| `OcspSingleResponse` / `OcspSingleIter` | `cert_id`, `cert_status`, `revocation_time`, `revocation_reason`, `this_update`, `next_update` | 零分配迭代每条证书状态响应 |
| `OcspRequest` / `OcspReqIter` | `version`, `request_list_der`, `has_signature`; `requests()` | 零分配解析并迭代 `OCSPRequest` 内全部 `CertID` |

---

## 三、使用示例

```cp
import { parseOcspResponseDer, OcspCertStatus } from "cap:auth";

fn inspectOcsp(der: []const u8) {
    if (resp := parseOcspResponseDer(der)) {
        if (resp.is_basic) {
            let it = resp.singleResponses();
            while (sr := it.next()) {
                if (sr.cert_status == OcspCertStatus.revoked) {
                    * = sr.revocation_time;
                    * = sr.revocation_reason;
                }
            }
        }
    }
}
```
