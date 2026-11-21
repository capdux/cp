# `cap:crl` — 零分配 RFC 5280 X.509 证书吊销列表 (CRL) DER 解析库

> **模块路径**：`lib/crl/`（入口 `lib/crl/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 RFC 5280 Section 5 X.509 v1/v2 CRL ASN.1 DER 规范：暴露完整 `TBSCertList` 原始切片供直接验签，兼容 `UTCTime` 与 `GeneralizedTime`，自动剥离序列号前导 `0x00` 符号位并解析 `OID_CRL_REASON` 吊销原因码，提供零分配 O(N) 序列号吊销查验与单遍摘要统计。

---

## 一、导入方式

```cp
import {
    OID_CRL_NUMBER,
    OID_CRL_REASON,
    CrlReason,
    crlReasonFromU8,
    CrlRevokedEntry,
    CrlParsed,
    CrlStats,
    CrlRevokedIter,
    parseCrlDer,
    isCrlDer,
    summarizeCrl,
} from "cap:crl";
```

---

## 二、核心常量、枚举与数据结构

| 符号 | 值 / 字段 | 说明 |
| :--- | :--- | :--- |
| OID 常量 | `OID_CRL_NUMBER` (`2.5.29.20`), `OID_CRL_REASON` (`2.5.29.21`) | CRL 编号与条目吊销原因扩展 OID |
| `CrlReason: u8` | `unspecified=0`, `key_compromise=1`, `ca_compromise=2`, `affiliation_changed=3`, `superseded=4`, `cessation_of_operation=5`, `certificate_hold=6`, `remove_from_crl=8`, `privilege_withdrawn=9`, `aa_compromise=10`, `none=255` | RFC 5280 Section 5.3.1 吊销原因码 |
| `CrlParsed` | `tbs_raw_der`, `version`, `tbs_sig_alg_oid`, `issuer_der`, `this_update`, `next_update`, `revoked_list_der`, `extensions_der`, `sig_alg_oid`, `signature`; `revoked()`, `isSerialRevoked(serial_bytes)` | 零拷贝 X.509 v1/v2 CRL 视图 |
| `CrlRevokedEntry` / `CrlRevokedIter` | `serial_bytes`, `revocation_date`, `reason: CrlReason` | 零分配迭代每条吊销记录并自动提取吊销原因 |
| `CrlStats` / `summarizeCrl(der_data)` | `version`, `revoked_count`, `key_compromise_count`, `has_next_update` | 单遍统计 CRL 摘要 |

---

## 三、使用示例

```cp
import { parseCrlDer, summarizeCrl } from "cap:crl";

fn checkRevocation(der: []const u8, serial: []const u8): bool {
    if (stats := summarizeCrl(der)) {
        * = stats.revoked_count;
    }
    if (crl := parseCrlDer(der)) {
        return crl.isSerialRevoked(serial);
    }
    return false;
}
```
