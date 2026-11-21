# `cap:smb` — MS-SMB2 / SMB3 文件共享协议零分配线协议解析库

纯 CP、零堆分配的 Microsoft MS-SMB2 / SMB3 线协议解析库，支持 NetBIOS 4B 会话解封、64B 同步/异步 SMB2 报头解析、`SMB2 NEGOTIATE` 请求与响应（含多 Dialect 与 SPNEGO Security Buffer）、复合链式消息（Compound/Chained）零拷贝迭代以及 SMB3 Transform (`\xFDSMB`) 52B 加密头解析。

## 导入

```cp
import {
    SMB_TCP_PORT,
    SMB2_HEADER_SIZE,
    SMB3_TRANSFORM_HEADER_SIZE,
    NETBIOS_SESSION_HEADER_SIZE,
    SMB2_MAGIC,
    SMB3_TRANSFORM_MAGIC,
    CMD_NEGOTIATE,
    CMD_SESSION_SETUP,
    CMD_CREATE,
    CMD_READ,
    CMD_CLOSE,
    FLAG_SERVER_TO_REDIR,
    FLAG_ASYNC_COMMAND,
    FLAG_RELATED_OPERATIONS,
    FLAG_SIGNED,
    DIALECT_202,
    DIALECT_210,
    DIALECT_300,
    DIALECT_302,
    DIALECT_311,
    unwrapNetbiosSession,
    isSmb2,
    isSmb3Transform,
    Smb2Header,
    Smb2Packet,
    parseSmb2,
    Smb2ChainIter,
    iterSmb2Chain,
    Smb2NegotiateReq,
    parseSmb2NegotiateReq,
    Smb2NegotiateResp,
    parseSmb2NegotiateResp,
    Smb3TransformHeader,
    parseSmb3Transform,
} from "cap:smb";
```

## 核心 API

- **传输解封与识别**：`unwrapNetbiosSession(data): ([]const u8)?`、`isSmb2(data): bool`、`isSmb3Transform(data): bool`。
- **SMB2 报头与链式迭代**：`parseSmb2(data): Smb2Packet?`（`Smb2Header` 提供 `isResponse()`, `isSigned()`, `isRelated()`, `isChained()`）、`iterSmb2Chain(data): Smb2ChainIter`（按 8B 对齐 `next_command` 迭代，`isMalformed()` 检测链中畸形帧）。
- **Negotiate 请求与响应**：`parseSmb2NegotiateReq(pkt): Smb2NegotiateReq?`（附 `dialectAt(idx): u16?`）、`parseSmb2NegotiateResp(pkt): Smb2NegotiateResp?`（零拷贝提取 `security_buffer`）。
- **SMB3 加密头**：`parseSmb3Transform(data): Smb3TransformHeader?`（提取 `signature`, `nonce`, `orig_msg_size`, `flags`, `session_id`, `encrypted_data`）。

## 示例

```cp
import { unwrapNetbiosSession, parseSmb2, parseSmb2NegotiateResp } from "cap:smb";

fn example(tcp_frame: []const u8) {
    if (payload := unwrapNetbiosSession(tcp_frame)) {
        if (pkt := parseSmb2(payload)) {
            if (resp := parseSmb2NegotiateResp(pkt)) {
                * = resp.dialect_revision;
            }
        }
    }
}
```
