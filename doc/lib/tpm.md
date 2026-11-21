# `cap:tpm` — TCG TPM 2.0 报头、远程证明、Quote 与 Public 零分配解析编码库

纯 CP、零堆分配的 TCG TPM 2.0 二进制结构解析与编码库，支持 10B 命令/响应报头往返编解码、`TPMS_ATTEST` 远程证明结构、`TPMS_QUOTE_INFO` / `TPML_PCR_SELECTION` 位图检测以及 `TPMT_PUBLIC` 公钥头与属性标志位解析。

## 导入

```cp
import {
    TPM_HEADER_SIZE,
    TPM_GENERATED_VALUE,
    TPM_ST_NO_SESSIONS,
    TPM_ST_SESSIONS,
    TPM_ST_ATTEST_CERTIFY,
    TPM_ST_ATTEST_QUOTE,
    TPM_ST_ATTEST_TIME,
    TPM_ST_ATTEST_CREATION,
    TPM_ALG_RSA,
    TPM_ALG_SHA1,
    TPM_ALG_KEYEDHASH,
    TPM_ALG_SHA256,
    TPM_ALG_SHA384,
    TPM_ALG_SHA512,
    TPM_ALG_ECC,
    TPM_ALG_SYMCIPHER,
    TPM_CC_STARTUP,
    TPM_CC_QUOTE,
    TPM_CC_GET_CAPABILITY,
    TPM_CC_GET_RANDOM,
    TPM_CC_PCR_READ,
    TPM_RC_SUCCESS,
    ATTR_FIXED_TPM,
    ATTR_FIXED_PARENT,
    ATTR_SENSITIVE_DATA_ORIGIN,
    ATTR_USER_WITH_AUTH,
    ATTR_RESTRICTED,
    ATTR_DECRYPT,
    ATTR_SIGN_ENCRYPT,
    TpmHeader,
    parseTpmHeader,
    encodeTpmHeader,
    TpmClockInfo,
    TpmAttest,
    isTpmAttest,
    parseTpmAttest,
    TpmPcrSelection,
    TpmPcrSelectIter,
    TpmQuoteInfo,
    parseTpmQuoteInfo,
    TpmPublicHeader,
    parseTpmPublicHeader,
} from "cap:tpm";
```

## 核心 API

- **命令/响应报头**：
  - `parseTpmHeader(data: []const u8): TpmHeader?` — 校验 `tag`（`0x8001` / `0x8002`）与 `10 <= size <= data.len`，附 `hasSessions()`、`isSuccess()`。
  - `encodeTpmHeader(tag: u16, code: u32, body_len: usize, out: []u8): usize?` — 零分配编码 10B 大端报头。
- **远程证明 (`TPMS_ATTEST`) 与 Quote**：
  - `isTpmAttest(data: []const u8): bool`
  - `parseTpmAttest(data: []const u8): TpmAttest?` — 严格校验 `0xFF544347` 魔数、2B `TPM2B` 长度前缀与 17B `TPMS_CLOCK_INFO`，附 `isQuote()`、`isCertify()`。
  - `parseTpmQuoteInfo(attested_body: []const u8): TpmQuoteInfo?` — 解析 `TPML_PCR_SELECTION` 与 `pcrDigest`，附 `pcrSelections(): TpmPcrSelectIter` 与 `TpmPcrSelection.isPcrSelected(pcr_idx)`。
- **公钥头 (`TPMT_PUBLIC`)**：
  - `parseTpmPublicHeader(data: []const u8): TpmPublicHeader?` — 解析 `alg_type`、`name_alg`、`object_attributes`、`auth_policy` 与 `params_and_unique`，附 `isRestricted()`、`isFixedTpm()`、`isSigningKey()`。

## 示例

```cp
import { parseTpmAttest, parseTpmQuoteInfo } from "cap:tpm";

fn inspectQuote(attest_bytes: []const u8) {
    if (att := parseTpmAttest(attest_bytes)) {
        if (att.isQuote()) {
            if (q := parseTpmQuoteInfo(att.attested_body)) {
                let it = q.pcrSelections();
                while (true) {
                    if (sel := it.next()) {
                        * = sel.isPcrSelected(7);
                    } else {
                        break;
                    }
                }
            }
        }
    }
}
```
