# `cap:tftp` — RFC 1350 / RFC 2347-2349 TFTP 零分配编解码库

纯 CP、零堆分配的 Trivial File Transfer Protocol（TFTP）全套报文（RRQ / WRQ / DATA / ACK / ERROR / OACK）与 RFC 2347 选项协商（`blksize`/`timeout`/`tsize`）编解码库。

## 导入

```cp
import {
    TFTP_UDP_PORT,
    TFTP_DEFAULT_BLKSIZE,
    TFTP_DATA_HEADER_SIZE,
    OP_RRQ,
    OP_WRQ,
    OP_DATA,
    OP_ACK,
    OP_ERROR,
    OP_OACK,
    ERR_NOT_DEFINED,
    ERR_FILE_NOT_FOUND,
    ERR_ACCESS_VIOLATION,
    ERR_DISK_FULL,
    ERR_ILLEGAL_OP,
    ERR_UNKNOWN_TID,
    ERR_FILE_EXISTS,
    ERR_NO_SUCH_USER,
    ERR_OPT_NEGOTIATION,
    TftpOption,
    TftpOptIter,
    TftpRequest,
    TftpData,
    TftpAck,
    TftpError,
    TftpOack,
    peekTftpOpcode,
    isTftp,
    parseTftpRequest,
    parseTftpData,
    parseTftpAck,
    parseTftpError,
    parseTftpOack,
    encodeTftpRequest,
    encodeTftpData,
    encodeTftpAck,
    encodeTftpError,
    TftpOptWriter,
} from "cap:tftp";
```

## 核心 API

- **快速识别**：`peekTftpOpcode(data): u16?`、`isTftp(data): bool`。
- **零拷贝解析**：`parseTftpRequest`、`parseTftpData`（含 `isLastBlock(blksize)`）、`parseTftpAck`（严格 `len == 4`）、`parseTftpError`（严格校验尾 NUL）、`parseTftpOack`。
- **选项迭代**：`TftpOptIter`、`findOption(name)`（ASCII 大小写不敏感）。
- **零分配编码**：`encodeTftpRequest`、`encodeTftpData`、`encodeTftpAck`、`encodeTftpError`、`TftpOptWriter`（`initRequest`, `initOack`, `writeOption`, `finish`）。

## 示例

```cp
import std from "std";
import { TftpOptWriter, parseTftpRequest } from "cap:tftp";

fn example() {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = TftpOptWriter.initRequest(buf[0..], false, "pxelinux.0", "octet");
    * = w.writeOption("blksize", "1468");
    * = w.writeOption("tsize", "0");
    if (wire := w.finish()) {
        if (req := parseTftpRequest(wire)) {
            * = req.findOption("BLKSIZE");
        }
    }
}
```
