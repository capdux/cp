# `cap:ntlm` — MS-NLMP NTLM v1/v2 认证线协议零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:auth`，导入请使用 `import { ... } from "cap:auth"`。

纯 CP、零堆分配的 Microsoft MS-NLMP（NTLM v1/v2）认证消息线协议解析库，覆盖 Type 1 `NEGOTIATE`、Type 2 `CHALLENGE`（含 8B Server Challenge 与 `AV_PAIR` TLV 零拷贝迭代）以及 Type 3 `AUTHENTICATE`（全部 6 个 Security Buffer 与 NTLMv2 判定），并严格执行 8 字节 `"NTLMSSP\0"` 签名、小端 Security Buffer 边界与 `VERSION` 标志位校验。

## 导入

```cp
import {
    NTLM_SIGNATURE,
    NTLM_MSG_NEGOTIATE,
    NTLM_MSG_CHALLENGE,
    NTLM_MSG_AUTHENTICATE,
    NTLMSSP_NEGOTIATE_UNICODE,
    NTLMSSP_NEGOTIATE_OEM,
    NTLMSSP_REQUEST_TARGET,
    NTLMSSP_NEGOTIATE_SIGN,
    NTLMSSP_NEGOTIATE_SEAL,
    NTLMSSP_NEGOTIATE_NTLM,
    NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY,
    NTLMSSP_NEGOTIATE_TARGET_INFO,
    NTLMSSP_NEGOTIATE_VERSION,
    NTLMSSP_NEGOTIATE_128,
    NTLMSSP_NEGOTIATE_56,
    MsvAvEOL,
    MsvAvNbComputerName,
    MsvAvNbDomainName,
    MsvAvDnsComputerName,
    MsvAvDnsDomainName,
    MsvAvDnsTreeName,
    MsvAvFlags,
    MsvAvTimestamp,
    MsvAvSingleHost,
    MsvAvTargetName,
    MsvAvChannelBindings,
    NtlmVersion,
    NtlmAvPair,
    NtlmAvPairIter,
    NtlmNegotiate,
    NtlmChallenge,
    NtlmAuthenticate,
    peekNtlmType,
    isNtlm,
    parseNtlmNegotiate,
    parseNtlmChallenge,
    parseNtlmAuthenticate,
} from "cap:auth";
```

## 核心 API

- **快速识别**：`peekNtlmType(data): u32?`、`isNtlm(data): bool`。
- **Type 1 Negotiate**：`parseNtlmNegotiate(data): NtlmNegotiate?`（`flags`, `domain`, `workstation`, `version: NtlmVersion?`）。
- **Type 2 Challenge**：`parseNtlmChallenge(data): NtlmChallenge?`（`flags`, `server_challenge: [8]u8`, `target_name`, `target_info`, `version: NtlmVersion?`，附 `avPairs(): NtlmAvPairIter` 与 `findAvPair(id): ([]const u8)?`）。
- **AV_PAIR 迭代器**：`NtlmAvPairIter.init(target_info)` 按 `4 + av_len` 零拷贝迭代至 `MsvAvEOL(0)`，并提供 `sawEol()` 与 `isTruncated()`。
- **Type 3 Authenticate**：`parseNtlmAuthenticate(data): NtlmAuthenticate?`（`flags`, `lm_response`, `nt_response`, `domain`, `user`, `workstation`, `session_key`, `version: NtlmVersion?`，附 `isNtlmV2(): bool`）。

## 示例

```cp
import { peekNtlmType, NTLM_MSG_CHALLENGE, parseNtlmChallenge, MsvAvDnsDomainName } from "cap:auth";

fn example(wire: []const u8) {
    if (t := peekNtlmType(wire)) {
        if (t == NTLM_MSG_CHALLENGE) {
            if (ch := parseNtlmChallenge(wire)) {
                * = ch.findAvPair(MsvAvDnsDomainName);
            }
        }
    }
}
```
