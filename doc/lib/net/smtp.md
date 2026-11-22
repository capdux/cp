# `cap:smtp` — RFC 5321/5322 SMTP 线协议编解码与 MIME 邮件构建库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

> **模块路径**: `lib/smtp/`  
> **导入协议**: `import { SmtpReply, SmtpCommandCodec, MailBuilder, SmtpAttachment, SmtpError, parseReply, dotStuff, dotUnstuff } from "cap:net";`  
> **动态挂载**: `import cap from "cap";` -> `cap.smtp.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:smtp` 是纯 CP 实现、零 C/POSIX/Bun 外部依赖（无 `std.posix` 或 `@import("bun")`）的无状态 SMTP 线协议编解码与 MIME 邮件构建库：

1. **SMTP 响应解析（RFC 5321 & RFC 2034）**：
   - `parseReply(allocator, raw)` 完整支持单行响应（如 `250 OK`）、多行续行响应（如 `250-PIPELINING\r\n250 8BITMIME\r\n250 AUTH PLAIN LOGIN`）与增强状态码（如 `2.1.0`、`5.1.1`）。
   - `SmtpReply` 提供 `isSuccess()`（2xx）、`isIntermediate()`（3xx）、`isTransientError()`（4xx）、`isPermanentError()`（5xx）以及 `deinit(allocator)` 确定性释放。
2. **SMTP 命令编码（RFC 5321 & RFC 4616）**：
   - `SmtpCommandCodec.ehlo`、`authPlain`（`\x00user\x00pass` Base64）、`mailFrom`、`rcptTo`、`dataStart`、`quit`。
   - 严格拦截地址与参数中的 `\r` / `\n` / `\0` / `<` / `>` 注入，返回 `SmtpError.HeaderInjection` 或 `SmtpError.InvalidAddress`。
3. **透明点号转义（RFC 5321 §4.5.2）**：
   - `dotStuff(allocator, raw_mail)` 将行首 `.` 转义为 `..`，并在末尾追加 `\r\n.\r\n` 结束符。
   - `dotUnstuff(allocator, stuffed_wire)` 剥离 `\r\n.\r\n` 结束符并还原行首 `..` 为 `.`，支持精确字节往返。
4. **MIME 邮件构建（RFC 5322 / 2045 / 2046 / 2047）**：
   - `MailBuilder` 支持多 `To` / `Cc` 收件人、`text/plain` 与 `text/html` 的 `multipart/alternative` 嵌套、带二进制 Base64（76 列换行）附件的 `multipart/mixed` 嵌套、非 ASCII `Subject` 自动 RFC 2047 `=?UTF-8?B?...?=` 编码，以及发件人/收件人/主题/文件名头注入拦截。

---

## 二、 核心 API 速查表 (`lib/smtp/smtp.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `SmtpError` | `InvalidAddress`, `HeaderInjection`, `InvalidReply`, `InvalidAttachment`, `TruncatedLine`, `OutOfMemory` |
| `SmtpReply` | `{ code: u16, enhanced: ([]const u8)?, lines: [][]const u8 }`；方法：`deinit(*_, allocator)`, `isSuccess()`, `isIntermediate()`, `isTransientError()`, `isPermanentError()` |
| `parseReply(allocator, raw)` | 解析单行/多行 SMTP 响应与增强状态码（调用方通过 `reply.deinit(allocator)` 释放） |
| `SmtpCommandCodec.ehlo(allocator, domain)` | 编码 `EHLO <domain>\r\n` |
| `SmtpCommandCodec.authPlain(allocator, user, pass)` | 编码 RFC 4616 `AUTH PLAIN <base64(\0user\0pass)>\r\n` |
| `SmtpCommandCodec.mailFrom(allocator, addr)` | 编码 `MAIL FROM:<addr>\r\n`（支持空串 `""` 生成退信路径 `MAIL FROM:<>\r\n`） |
| `SmtpCommandCodec.rcptTo(allocator, addr)` | 编码 `RCPT TO:<addr>\r\n` |
| `SmtpCommandCodec.dataStart()` | 返回静态命令 `"DATA\r\n"` |
| `SmtpCommandCodec.quit()` | 返回静态命令 `"QUIT\r\n"` |
| `dotStuff(allocator, raw_mail)` | RFC 5321 §4.5.2 行首 `.` 转义并追加 `\r\n.\r\n` |
| `dotUnstuff(allocator, stuffed_wire)` | 剥离 `\r\n.\r\n` 并还原行首 `..` |
| `SmtpAttachment` | `{ filename: []const u8, content_type: []const u8, data: []const u8 }` |
| `MailBuilder` | `init(allocator, from_addr, subject)`, `deinit(*_)`, `addTo(*_, addr)`, `addCc(*_, addr)`, `setTextBody(*_, text)`, `setHtmlBody(*_, html)`, `addAttachment(*_, att)`, `build(allocator, boundary) -> []u8` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    SmtpCommandCodec,
    MailBuilder,
    SmtpAttachment,
    parseReply,
    dotStuff,
    dotUnstuff,
} from "cap:net";

fn demo(alloc: std.mem.Allocator) !void {
    let reply = parseReply(alloc, "250-PIPELINING\r\n250 2.0.0 OK\r\n")?;
    defer reply.deinit(alloc);

    const ehlo = SmtpCommandCodec.ehlo(alloc, "mail.cp.dev")?;
    defer alloc.free(ehlo);

    let mb = MailBuilder.init(alloc, "sender@cp.dev", "测试主题");
    defer mb.deinit();
    mb.addTo("alice@cp.dev")?;
    mb.setTextBody("Hello CP");
    mb.setHtmlBody("<p>Hello CP</p>");
    mb.addAttachment(SmtpAttachment{
        filename: "note.txt",
        content_type: "text/plain",
        data: "attachment payload",
    })?;

    const raw_mail = mb.build(alloc, "bound-001")?;
    defer alloc.free(raw_mail);

    const wire = dotStuff(alloc, raw_mail)?;
    defer alloc.free(wire);
    const restored = dotUnstuff(alloc, wire)?;
    defer alloc.free(restored);
}
```
