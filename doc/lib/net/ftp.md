# `cap:ftp` — RFC 959 / RFC 2428 / RFC 3659 FTP 控制通道与 MLSD 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP、零堆分配的 FTP 控制通道命令/单多行回复解析、PASV/EPSV 地址提取、PORT/EPRT 编码与 RFC 3659 MLSD/MLST Fact 迭代器。

## 导入

```cp
import {
    FTP_CONTROL_PORT,
    FTP_DATA_PORT,
    FTPS_IMPLICIT_PORT,
    RPL_READY,
    RPL_PASV,
    RPL_EPSV,
    FtpCommand,
    parseFtpCommand,
    FtpReplyLine,
    parseFtpReplyLine,
    FtpPasvAddr,
    parsePasvReply,
    parseEpsvPort,
    FtpMlsxFact,
    FtpMlsxFactIter,
    FtpMlsxEntry,
    parseMlsxLine,
    encodeFtpCommand,
    encodePortCommand,
    encodeEprtCommand,
} from "cap:net";
```

## 核心 API

- **命令与回复**：`parseFtpCommand(line)`、`parseFtpReplyLine(line)`（`is_continuation`, `isPreliminary()`, `isPositive()`, `isIntermediate()`, `isTransientError()`, `isPermanentError()`）。
- **被动模式地址**：`parsePasvReply(message): FtpPasvAddr?`、`parseEpsvPort(message): u16?`。
- **RFC 3659 MLSD/MLST**：`parseMlsxLine(line): FtpMlsxEntry?`（`facts()`, `findFact(name)` 忽略大小写、`size(): u64?`、`entryType(): ([]const u8)?`）。
- **零分配编码**：`encodeFtpCommand(verb, arg, out)`、`encodePortCommand(ip, port, out)`、`encodeEprtCommand(is_ipv6, addr, port, out)`。

## 示例

```cp
import { parsePasvReply, parseMlsxLine } from "cap:net";

fn example() {
    if (addr := parsePasvReply("Entering Passive Mode (192,168,1,10,19,136).")) {
        * = addr.port; // 5000
    }
    if (entry := parseMlsxLine("type=file;size=4096; readme.md\r\n")) {
        * = entry.size();
    }
}
```
