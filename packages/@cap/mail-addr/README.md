# @cap/mail-addr

Zero-allocation RFC 5322 email address and recipient list parser and formatter for CP — clean-room MIT parity with npm `addressparser` / `email-addresses` (10M/wk) and `nodemailer`.

纯 CP 零堆分配 RFC 5322 邮件地址与收件人列表解析/格式化引擎，全部零拷贝切片，支持最多 32 个逗号分隔收件人。

## Install / 安装

```bash
cap add @cap/mail-addr
```

## Features / 核心特性

- **Full RFC 5322 Formats**: Plain addresses (`alice@example.com`), angle-bracket format (`Alice Smith <alice@example.com>`), quoted display names with commas or escapes (`"Smith, Alice" <alice@example.com>`), and `(comment)` stripping.
- **Multi-Recipient Lists**: Quote-aware and comment-aware comma splitting up to 32 recipients with zero heap allocation.
- **Canonical Formatting**: `formatMailbox` automatically quotes and escapes display names containing RFC 5322 special characters (`,`, `;`, `"`, `\`, `<`, `>`, etc.).

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `MailAddress` | `struct { valid: bool, display_name: string, address: string, local: string, domain: string }` | Zero-copy parsed mailbox result |
| `AddressList` | `struct { items: [32]MailAddress, count: usize }` | Fixed-capacity recipient list |
| `parseMailbox` | `(raw: string) -> MailAddress` | Parse and validate a single RFC 5322 mailbox |
| `parseAddressList` | `(raw: string) -> AddressList` | Parse a comma-separated recipient list |
| `formatMailbox` | `(m: MailAddress, out: []u8) -> string` | Format a mailbox with automatic quoting/escaping |

## Usage / 使用示例

```cp
import {
    parseMailbox,
    parseAddressList,
    formatMailbox,
} from "@cap/mail-addr";

const list = parseAddressList("Alice <a@x.com>, \"Doe, John\" <j@y.org>, plain@z.net");
// list.count == 3

let out: [128]u8 = undefined;
const formatted = formatMailbox(list.items[1], out[0..128]);
// "\"Doe, John\" <j@y.org>"
```
