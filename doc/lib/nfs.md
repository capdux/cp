# `cap:nfs` — RFC 5531 ONC RPC v2 & RFC 1813 NFSv3 零分配线协议解析库

> **模块路径**: `lib/nfs/`  
> **导入协议**: `import { NFS_PORT, RPC_PROG_NFS, NFS_V3, NFS3_GETATTR, parseRpcRecordFrag, parseRpcCall, parseRpcReply, encodeRpcCallHeader, parseNfs3FileHandle, parseNfs3Fattr } from "cap:nfs";`  
> **动态挂载**: `import cap from "cap";` -> `cap.nfs.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:nfs` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 RFC 5531 ONC RPC v2 + RFC 1813 NFSv3 线协议解析与编码库：

1. **TCP Record Marking (`parseRpcRecordFrag` / `encodeRpcRecordMark`)**：
   - 完整解析 RFC 5531 §11 4 字节流式分片标记（高 1 位 `is_last` + 低 31 位长度），零拷贝切分分片载荷。
2. **ONC RPC v2 Call / Reply (`parseRpcCall` / `parseRpcReply` / `encodeRpcCallHeader`)**：
   - 严格校验 `rpcvers == 2`、`cred`/`verf` 长度不超过 400B 且满足 XDR 4B 零填充对齐。
   - 同时支持 `MSG_ACCEPTED` 与 `MSG_DENIED` 响应解析，附 `RpcReply.isSuccess()` 便捷方法。
3. **NFSv3 File Handle & `fattr3` (`parseNfs3FileHandle` / `parseNfs3Fattr`)**：
   - 严格校验 `nfs_fh3` 长度 `<= 64` 字节与 XDR 对齐填充。
   - 完整解析 84 字节 `fattr3` 元数据（文件类型、权限、大小、fsid、fileid、atime/mtime/ctime）。

---

## 二、 核心 API 速查表 (`lib/nfs/nfs.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| RPC / NFS 常量 | `NFS_PORT (2049)`, `RPC_VERSION (2)`, `RPC_CALL (0)`, `RPC_REPLY (1)`, `RPC_PROG_PORTMAP/NFS/MOUNT`, `NFS_V3/V4`, `AUTH_NONE/SYS/SHORT/GSS`, `MSG_ACCEPTED/DENIED`, `ACCEPT_*`, `NFS3_*` 过程号/状态码, `FTYPE_*` |
| `RpcRecordFrag` | `parseRpcRecordFrag(data)`、`encodeRpcRecordMark(is_last, payload_len, out)` |
| `RpcAuth` / `RpcCall` / `RpcReply` | `parseRpcCall(data)`、`parseRpcReply(data)`（含 `isSuccess()`）、`encodeRpcCallHeader(xid, prog, vers, proc, out)` |
| `Nfs3Fattr` | `parseNfs3FileHandle(data): ([]const u8)?`、`parseNfs3Fattr(data): Nfs3Fattr?` |

---

## 三、 使用示例

```typescript
import { RPC_PROG_NFS, NFS_V3, NFS3_GETATTR, encodeRpcCallHeader, parseRpcCall } from "cap:nfs";

fn demo() {
    let hdr_buf: [40]u8 = [0; 40];
    if (n := encodeRpcCallHeader(0x10203040, RPC_PROG_NFS, NFS_V3, NFS3_GETATTR, hdr_buf[0..])) {
        if (call := parseRpcCall(hdr_buf[0..n])) {
            * = call.xid;
        }
    }
}
```
