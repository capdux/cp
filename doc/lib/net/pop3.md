# `cap:pop3` — RFC 1939 / RFC 2449 POP3 邮局协议 v3 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP、零堆分配（zero-alloc）的 Post Office Protocol version 3（POP3）线协议编解码库，支持 RFC 1939 状态行 / `STAT` / `LIST` / `UIDL` / 命令解析与编码、RFC 2449 `[RESP-CODE]` 响应码解析、零拷贝多行响应迭代与字节解填充（byte-unstuffing）及发送端 dot-stuffing。

## 导入

```cp
import {
    POP3_TCP_PORT,
    POP3S_TCP_PORT,
    POP3_MAX_CMD_LEN,
    POP3_MAX_LINE_LEN,
    isPop3ResponsePrefix,
    Pop3Response,
    parsePop3Response,
    Pop3Stat,
    parsePop3Stat,
    Pop3ListItem,
    parsePop3ListLine,
    Pop3UidlItem,
    parsePop3UidlLine,
    Pop3Command,
    parsePop3Command,
    Pop3MultiLineIter,
    encodePop3Command,
    pop3DotStuff,
} from "cap:net";
```

## 核心 API

- **协议常量**：
  - `POP3_TCP_PORT = 110`、`POP3S_TCP_PORT = 995`
  - `POP3_MAX_CMD_LEN = 255`、`POP3_MAX_LINE_LEN = 512`
- **状态响应解析**：
  - `isPop3ResponsePrefix(data): bool` — 快速嗅探 `+OK` / `-ERR` 响应前缀。
  - `parsePop3Response(line): Pop3Response?` — 严格校验 `+OK` 或 `-ERR` 前缀（后接空格或行尾），零拷贝提取 RFC 2449 `[RESP-CODE]`（如 `IN-USE`、`LOGIN-DELAY`、`SYS/TEMP`）与消息正文。`Pop3Response` 提供 `isOk()`、`isErr()`、`hasRespCode()`。
- **邮箱与报文列表解析**：
  - `parsePop3Stat(resp_msg): Pop3Stat?` — 解析 `STAT` 响应载荷（`message_count`, `total_octets`），严格校验十进制无符号整数与溢出。
  - `parsePop3ListLine(line): Pop3ListItem?` — 解析 `LIST` 扫描行（`msg_num >= 1`, `octets`）。
  - `parsePop3UidlLine(line): Pop3UidlItem?` — 解析 `UIDL` 唯一标识行（`msg_num >= 1`, `uid` 长度 `1..=70` 且位于 `0x21..=0x7E`）。
- **命令解析与零分配编码**：
  - `parsePop3Command(line): Pop3Command?` — 解析 `VERB [arg1 [arg2]]`（对 `PASS` 保留含空格密码作为 `arg1`），`Pop3Command` 提供大小写不敏感的 `verbEq(target)`。
  - `encodePop3Command(verb, arg1, arg2, out: []u8): usize?` — 零分配编码为 `VERB [arg1 [arg2]]\r\n`，拦截 CRLF/NUL 注入、超长命令（> 255 字节）与输出缓冲不足。
- **多行响应迭代与 Dot-Stuffing**：
  - `Pop3MultiLineIter` — 零拷贝按 `\r\n` 迭代多行响应，遇独立 `.\r\n` 正常结束（`isComplete()`），自动对 `..` 开头行剥离首个 `.`（byte-unstuffing），遇截断/未终止设置 `hasError()`，并通过 `remaining()` 返回后续流水线数据。
  - `pop3DotStuff(src, out: []u8): usize?` — 零分配将原始正文编码为 RFC 1939 dot-stuffed 多行线协议格式（行首 `.` 补齐 `..`、规范化 `\r\n` 并附加终止行 `.\r\n`）。

## 示例

```cp
import std from "std";
import {
    parsePop3Response,
    parsePop3Stat,
    Pop3MultiLineIter,
    encodePop3Command,
    pop3DotStuff,
} from "cap:net";

fn example() {
    if (resp := parsePop3Response("+OK 2 320\r\n")) {
        if (resp.isOk()) {
            if (st := parsePop3Stat(resp.message)) {
                * = st.message_count;
                * = st.total_octets;
            }
        }
    }

    let cmd_buf: [64]u8 = std.mem.zeroes([64]u8);
    if (n := encodePop3Command("TOP", "1", "10", cmd_buf[0..])) {
        * = cmd_buf[0..n];
    }

    let wire_buf: [128]u8 = std.mem.zeroes([128]u8);
    if (wn := pop3DotStuff("Subject: Hi\r\n.dot line\r\nBody", wire_buf[0..])) {
        let it = Pop3MultiLineIter.init(wire_buf[0..wn]);
        while (true) {
            if (line := it.next()) {
                * = line;
            } else {
                break;
            }
        }
    }
}
```
