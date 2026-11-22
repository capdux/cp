# `cap:mail` — 电子邮件地址、RFC 2047 编码词与 Quoted-Printable 工具库

> 参考来源：/data/dev/bun/src/http/http.cp | 吸收日期：2026-11-22

## 概述

`cap:mail` 吸收自 Bun HTTP/MIME 协议解析体系（`src/http/http.cp`），聚合收敛了 RFC 5322 邮件地址解析、RFC 2047 MIME 头部 Encoded-Word 编解码以及 RFC 2045 Quoted-Printable 传输编解码三大能力，全程纯 CP、零外部依赖、零堆分配。

## 规范契约与安全边界

- **RFC 5322 邮箱与地址列表解析（`address.cp`）**：
  - 同时支持裸地址（`alice@example.com`）与带显示名的尖括号格式（`"Alice, Jr." <alice@example.com>`）；
  - `MailboxListIter` 在逗号分隔多地址列表时严格保护引号显示名内部的逗号，避免错误切分；
  - `isValidAddrSpec` 校验本地部分与域名结构，拒绝空段、连续点号 `..`、控制字符与非法空白，防范 SMTP 头部注入。
- **RFC 2047 MIME Encoded-Word（`rfc2047.cp`）**：
  - 解析 `=?charset?B|Q?encoded-text?=` 结构，大小写不敏感识别 `B`（Base64）与 `Q`（Quoted-Printable，下划线 `_` 还原为空格 `0x20`）；
  - `decodeHeaderValue` 按 RFC 2047 §6.2 规范自动折叠相邻两个 Encoded-Word 之间的纯线性空白（LWS），而保留普通文本与 Encoded-Word 之间的空白。
- **RFC 2045 Quoted-Printable（`qp.cp`）**：
  - 支持软换行（`=\r\n` / `=\n`）折叠与 `=XX` 十六进制字节还原；输出缓冲不足或遇到畸形编码时安全返回 `null`。

## 导出清单

### RFC 5322 邮箱地址 (`./address.cp`)

- `isValidAddrSpec(addr: []const u8): bool`
- `Mailbox`：`{ display_name, local_part, domain }` + `Mailbox.parse(...)`
- `MailboxListIter`：零拷贝逗号分隔邮箱列表迭代器

### RFC 2047 MIME Encoded-Word (`./rfc2047.cp`)

- `EncodedWordEncoding`：`base64` / `quoted_printable`
- `EncodedWord`
- `parseEncodedWord(src: []const u8): EncodedWord?`
- `decodeEncodedWord(src: []const u8, out: []u8): ([]const u8)?`
- `decodeHeaderValue(src: []const u8, out: []u8): ([]const u8)?`
- `encodeUtf8Base64Word(src: []const u8, out: []u8): ([]const u8)?`

### RFC 2045 Quoted-Printable (`./qp.cp`)

- `encodeQuotedPrintable(src: []const u8, out: []u8): ([]const u8)?`
- `decodeQuotedPrintable(src: []const u8, out: []u8): ([]const u8)?`

## 使用示例

```cp
import { Mailbox, decodeHeaderValue, decodeQuotedPrintable } from "cap:mail";

fn demo() {
    if (mb := Mailbox.parse("\"Alice\" <alice@example.com>")) {
        * = mb.local_part; // "alice"
        * = mb.domain;     // "example.com"
    }
    let buf: [128]u8 = undefined;
    const subj = decodeHeaderValue("=?UTF-8?B?SGVsbG8=?= World", buf[0..]);
    * = subj;
    * = decodeQuotedPrintable;
}
```
