# `cap:tacacs` — 零分配 RFC 8907 TACACS+ AAA 线协议解析库

> **模块路径**：`lib/tacacs/`（入口 `lib/tacacs/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 RFC 8907 TACACS+ 大端序线协议：支持 12 字节报头与标志位解析、Authentication START/REPLY、Authorization REQUEST/RESPONSE、Accounting REQUEST/REPLY 解码，以及零拷贝长度前缀表参数迭代器 `TacacsArgIter`。

---

## 一、导入方式

```cp
import {
    TACACS_TCP_PORT,
    TACACS_HEADER_SIZE,
    TACACS_MAJOR_VER,
    TACACS_MINOR_VER_DEFAULT,
    TACACS_MINOR_VER_ONE,
    TAC_PLUS_UNENCRYPTED_FLAG,
    TAC_PLUS_SINGLE_CONNECT_FLAG,
    TacacsType,
    TacacsAuthenStatus,
    TacacsAuthorStatus,
    TacacsAcctStatus,
    TacacsHeader,
    TacacsPacket,
    TacacsArgIter,
    TacacsAuthenStart,
    TacacsAuthenReply,
    TacacsAuthorRequest,
    TacacsAuthorResponse,
    TacacsAcctRequest,
    TacacsAcctReply,
    parseTacacs,
    isTacacs,
    parseAuthenStart,
    parseAuthenReply,
    parseAuthorRequest,
    parseAuthorResponse,
    parseAcctRequest,
    parseAcctReply,
} from "cap:tacacs";
```

---

## 二、核心常量与枚举

| 符号 | 说明 |
| :--- | :--- |
| 协议常量 | `TACACS_TCP_PORT (49)`, `TACACS_HEADER_SIZE (12)`, `TACACS_MAJOR_VER (0xC)`, `TACACS_MINOR_VER_DEFAULT (0)`, `TACACS_MINOR_VER_ONE (1)` |
| 报头标志 | `TAC_PLUS_UNENCRYPTED_FLAG (0x01)`, `TAC_PLUS_SINGLE_CONNECT_FLAG (0x04)` |
| `TacacsType: u8` | `authen (1)`, `author (2)`, `acct (3)` |
| `TacacsAuthenStatus: u8` | `pass (1)`, `fail (2)`, `getdata (3)`, `getuser (4)`, `getpass (5)`, `restart (6)`, `err (7)`, `follow (0x21)`, `unknown (0)` |
| `TacacsAuthorStatus: u8` | `pass_add (1)`, `pass_repl (2)`, `fail (0x10)`, `err (0x11)`, `follow (0x21)`, `unknown (0)` |
| `TacacsAcctStatus: u8` | `success (1)`, `err (2)`, `follow (0x21)`, `unknown (0)` |

---

## 三、核心 API

- `parseTacacs(data)` / `isTacacs(data)` — 严格校验 `major_version == 0xC`、`pkt_type in 1..3`、`seq_no >= 1` 与 `12 + length <= data.len`。
- `parseAuthenStart(body)` / `parseAuthenReply(body)` — 解析认证 START / REPLY 明文 Body。
- `parseAuthorRequest(body)` / `parseAuthorResponse(body)` — 解析授权 REQUEST / RESPONSE Body，附 `args(): TacacsArgIter`。
- `parseAcctRequest(body)` / `parseAcctReply(body)` — 解析计费 REQUEST / REPLY Body，附 `args(): TacacsArgIter`。
- `TacacsArgIter` — 零分配迭代 `arg_cnt` 个长度前缀参数切片。

---

## 四、使用示例

```cp
import { parseTacacs, parseAuthorRequest, TacacsType } from "cap:tacacs";

fn inspectAuthor(pkt_bytes: []const u8) {
    if (pkt := parseTacacs(pkt_bytes)) {
        if (pkt.header.pkt_type == TacacsType.author && pkt.header.isUnencrypted()) {
            if (req := parseAuthorRequest(pkt.body)) {
                let it = req.args();
                while (arg := it.next()) {
                    * = arg;
                }
            }
        }
    }
}
```
