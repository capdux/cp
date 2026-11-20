# CP 标准库：电子邮件 MIME 与 SMTP 协议引擎 (`cap:email` / `cap.email`)

## 一、 架构总览

`cap:email` 是 CP 语言官方内置的零外部依赖电子邮件协议核心库，完整覆盖 **RFC 5322** 邮件地址解析与 CRLF 头注入防护、**RFC 2045 / RFC 2047** MIME 邮件构建与递归解析（含 Quoted-Printable 与 76 列 Base64 附件），以及 **RFC 5321 / RFC 4616** SMTP 线缆命令编解码、点填充（Dot-Stuffing）与多行响应解析。全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:email"` 与 `import cap from "cap"; cap.email.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/email/address.cp` | `// Source: cp/lib/email/address.cp (RFC 5322 Email Address Parser & Header Injection Guard)` | `EmailError`、`EmailAddress` 解析/校验/格式化与 `containsHeaderInjection` 防护 |
| `lib/email/mime.cp` | `// Source: cp/lib/email/mime.cp (RFC 5322 / RFC 2045 MIME Composer, Quoted-Printable & Parser)` | `QuotedPrintable`、`EmailAttachment`、`EmailBuilder`、`ParsedEmail`、`MimeParser` |
| `lib/email/smtp.cp` | `// Source: cp/lib/email/smtp.cp (RFC 5321 SMTP Command Encoder, Dot-Stuffing & Reply Parser)` | `SmtpReply` 与 `SmtpCodec`（`EHLO`/`MAIL FROM`/`RCPT TO`/`AUTH PLAIN`/Dot-Stuffing/多行响应） |
| `lib/email/index.cp` | `// Source: cp/lib/email/index.cp (Unified Exports for cap:email)` | `cap:email` 与 `cap.email` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. RFC 5322 邮件地址与注入防护（`address.cp`）

- **`EmailError`**：`InvalidAddress`、`HeaderInjection`、`InvalidMime`、`InvalidSmtpReply`、`OutOfMemory`
- **`containsHeaderInjection(s: string): bool`**：检测 `\r`、`\n`、`\x00` 字节，阻断邮件头与 SMTP 命令注入
- **`EmailAddress`**（字段：`display_name: string`、`local_part: string`、`domain: string`）：
  - `EmailAddress.parse(raw: string): EmailAddress ! EmailError`：支持 `"Alice <alice@example.com>"` 与 `"bob@sub.example.org"`；严格校验局部长度 `<= 64`、总长 `<= 254`、禁止连续 `..`、空格与 CRLF 注入，要求合法域名标签与非数字顶级域
  - `EmailAddress.isValid(raw: string): bool`：无分配快速合法性判定
  - `emailOnly(alloc): ([]u8) ! EmailError`：输出裸地址 `local@domain`
  - `format(alloc): ([]u8) ! EmailError`：输出 RFC 5322 格式化地址（含特殊字符自动加引号转义）

### 2. MIME 编解码、构建与解析（`mime.cp`）

- **`QuotedPrintable`**：
  - `encode(alloc, input): ([]u8) ! EmailError`：RFC 2045 §6.7 编码，保留 `\r\n`，对行尾空白与非 ASCII/特殊字节执行 `=XX` 十六进制编码，并在 76 列处自动插入 `=\r\n` 软换行
  - `decode(alloc, input): ([]u8) ! EmailError`：剥离 `=\r\n` / `=\n` 软换行并解码 `=XX` 十六进制字节
- **`EmailAttachment`**：`filename: string`、`content_type: string`、`data: []const u8`（提供 `EmailAttachment.init(...)`）
- **`EmailBuilder`**：
  - `init(alloc)` / `deinit(*_)`
  - `setFrom(*_, addr)` / `addTo(*_, addr)` / `addCc(*_, addr)`：强制经过 `EmailAddress.parse` 校验
  - `setSubject(*_, subject)`：阻断 CRLF 注入；`build` 时含非 ASCII 字符自动拆分为 RFC 2047 `=?UTF-8?B?...?=` 编码字
  - `setTextBody(*_, body)` / `setHtmlBody(*_, body)` / `addAttachment(*_, attachment)` / `addHeader(*_, name, value)`
  - `build(alloc): ([]u8) ! EmailError`：自动按内容拓扑生成单文本、`multipart/alternative` 或 `multipart/mixed`（内嵌 `multipart/alternative` + 76 列折行 Base64 附件）
- **`ParsedEmail` & `MimeParser`**：
  - `MimeParser.parse(alloc, raw_rfc5322): ParsedEmail ! EmailError`：自动展开折叠头（Header Unfolding）、解码 RFC 2047 `=?UTF-8?B?...?=` / `=?UTF-8?Q?...?=` 主题，递归解析 `multipart/mixed` 与 `multipart/alternative`，自动解码 `quoted-printable` 与 `base64` 正文及附件
  - `ParsedEmail` 提供 `from`、`to`、`cc`、`subject`、`raw_subject`、`content_type`、`text_body`、`html_body`、`attachments`、`parts`、`getHeader(name)` 与 `deinit(*_)`

### 3. RFC 5321 SMTP 线缆协议引擎（`smtp.cp`）

- **`SmtpReply`**（`code: u16`、`is_multiline: bool`、`message: string`）：
  - `isPositive(): bool`（`2xx`/`3xx`）、`isPositiveCompletion(): bool`（`2xx`）、`isPositiveIntermediate(): bool`（`3xx`）、`isTransientNegative(): bool`（`4xx`）、`isPermanentNegative(): bool`（`5xx`）
- **`SmtpCodec`**：
  - `encodeEhlo(alloc, domain)` -> `EHLO <domain>\r\n`
  - `encodeMailFrom(alloc, reverse_path)` -> `MAIL FROM:<local@domain>\r\n`（支持空串 `""` 生成退信空反向路径 `MAIL FROM:<>\r\n`）
  - `encodeRcptTo(alloc, forward_path)` -> `RCPT TO:<local@domain>\r\n`
  - `encodeAuthPlain(alloc, user, pass)` -> `AUTH PLAIN <base64(\0user\0pass)>\r\n`（RFC 4616）
  - `dotStuffBody(alloc, raw_msg)`：RFC 5321 §4.5.2 行首 `.` 翻倍并附加 `\r\n.\r\n` 结束符
  - `dotUnstuffBody(alloc, stuffed)`：剥离 `\r\n.\r\n` 并还原行首 `..` 为 `.`
  - `parseReply(raw_reply): SmtpReply ! EmailError`：解析单行 `250 OK\r\n` 及多行 `250-...\r\n250 ...\r\n` 响应并校验状态码一致性

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { EmailBuilder, MimeParser, SmtpCodec } from "cap:email";

pub fn demoEmail(): !void {
    const alloc = std.heap.page_allocator;

    let builder = EmailBuilder.init(alloc);
    defer builder.deinit();
    builder.setFrom("Alice <alice@example.com>")?;
    builder.addTo("bob@sub.example.org")?;
    builder.setSubject("月度报告 📊 Report")?;
    builder.setTextBody("Hello Bob");
    builder.setHtmlBody("<p>Hello <b>Bob</b></p>");

    const wire_msg = builder.build(alloc)?;
    defer alloc.free(wire_msg);

    let parsed = MimeParser.parse(alloc, wire_msg)?;
    defer parsed.deinit();

    const stuffed = SmtpCodec.dotStuffBody(alloc, wire_msg)?;
    defer alloc.free(stuffed);

    const ok = cap.email.EmailAddress.isValid("dev@cp-lang.org");
    * = ok;
}
```
