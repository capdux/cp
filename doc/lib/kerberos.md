# `cap:kerberos` — RFC 4120 Kerberos v5 ASN.1 DER 线协议零分配解析库

纯 CP、零堆分配的 RFC 4120 Kerberos v5 ASN.1 DER 线协议解析库，覆盖 TCP 4B Record Mark 解封、`APPLICATION [1]` Ticket（含 `PrincipalName` 多组件迭代与 `EncryptedData`）、`APPLICATION [30]` `KRB-ERROR` 以及 `APPLICATION [10..30]` 通用消息头探测，严格校验 v5 版本与 DER 最小化编码边界。

## 导入

```cp
import {
    KRB_PORT,
    KRB_VERSION,
    MSG_AS_REQ,
    MSG_AS_REP,
    MSG_TGS_REQ,
    MSG_TGS_REP,
    MSG_AP_REQ,
    MSG_AP_REP,
    MSG_KRB_SAFE,
    MSG_KRB_PRIV,
    MSG_KRB_CRED,
    MSG_KRB_ERROR,
    ETYPE_AES128_CTS_HMAC_SHA1,
    ETYPE_AES256_CTS_HMAC_SHA1,
    ETYPE_RC4_HMAC,
    NT_UNKNOWN,
    NT_PRINCIPAL,
    NT_SRV_INST,
    NT_SRV_HST,
    NT_ENTERPRISE,
    KDC_ERR_NONE,
    KDC_ERR_C_PRINCIPAL_UNKNOWN,
    KDC_ERR_S_PRINCIPAL_UNKNOWN,
    KDC_ERR_PREAUTH_FAILED,
    KDC_ERR_PREAUTH_REQUIRED,
    KRB_AP_ERR_TKT_EXPIRED,
    KRB_AP_ERR_SKEW,
    KrbStringIter,
    KrbPrincipalName,
    KrbEncryptedData,
    KrbTicket,
    KrbMessageInfo,
    KrbError,
    unwrapKrbTcpRecord,
    peekKrbAppTag,
    isKerberos,
    parseKrbMessageInfo,
    parseKrbTicket,
    parseKrbError,
} from "cap:kerberos";
```

## 核心 API

- **传输解封与识别**：`unwrapKrbTcpRecord(data): ([]const u8)?`、`peekKrbAppTag(data): u8?`、`isKerberos(data): bool`。
- **通用消息头**：`parseKrbMessageInfo(data): KrbMessageInfo?`（解析 `APPLICATION [10..30]` 外层、`SEQUENCE` 内 `pvno == 5` 与 `msg_type`）。
- **Ticket 解析**：`parseKrbTicket(data): KrbTicket?`（解析 `[APPLICATION 1]`，提供 `realm`、`sname.names()` 零拷贝迭代器及 `enc_part` 含可选 `kvno`）。
- **KRB-ERROR 解析**：`parseKrbError(data): KrbError?`（解析 `[APPLICATION 30]`，支持全部可选字段 `ctime`/`cusec`/`crealm`/`cname`/`e-text`/`e-data`）。

## 示例

```cp
import { unwrapKrbTcpRecord, parseKrbTicket } from "cap:kerberos";

fn example(tcp_bytes: []const u8) {
    if (der := unwrapKrbTcpRecord(tcp_bytes)) {
        if (tkt := parseKrbTicket(der)) {
            let it = tkt.sname.names();
            while (true) {
                if (part := it.next()) {
                    * = part;
                } else {
                    break;
                }
            }
        }
    }
}
```
