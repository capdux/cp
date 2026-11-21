# `cap:irc` — RFC 1459 / RFC 2812 / IRCv3 线协议与 Message Tags 零分配编解码库

> **模块路径**: `lib/irc/`  
> **导入协议**: `import { parseIrcLine, parseIrcPrefix, unescapeTagValue, escapeTagValue, IrcMessage, IrcPrefix, IrcTag, IrcTagIter, IrcParamIter, IrcWriter, IRC_TCP_PORT, IRCS_TCP_PORT, RPL_WELCOME } from "cap:irc";`  
> **动态挂载**: `import cap from "cap";` -> `cap.irc.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:irc` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 RFC 1459 / RFC 2812 / IRCv3 线协议与 Message Tags 编解码库：

1. **零拷贝行解析 (`parseIrcLine` -> `IrcMessage`)**：
   - 自动剥离尾部 `\r\n` 或 `\n`。
   - 完整支持可选 IRCv3 `@tags`、可选 `:prefix`、必选 `command`（纯 ASCII 字母或 3 位数字）与可选参数段。
   - 严格拒收空行、空 `@` 标签、超长标签（`> 8191` 字节）、空 `:` 前缀、缺少命令及非法命令格式。
2. **IRCv3 Message Tags 迭代与转义 (`IrcTagIter` / `unescapeTagValue` / `escapeTagValue`)**：
   - 零拷贝切分 `;` 分隔的标签对（支持无值标签如 `k2`、空值标签 `k=` 与客户端专用标签 `+client/tag`）。
   - 完整实现 IRCv3 转义规则：`\:` <-> `;`、`\s` <-> ` `、`\\` <-> `\`、`\r` <-> CR、`\n` <-> LF。
3. **前缀与参数视图 (`parseIrcPrefix` / `IrcParamIter`)**：
   - `parseIrcPrefix` 零拷贝切分 `nick!user@host`、`nick@host`、`nick!user` 与纯 `servername`。
   - `IrcParamIter` 按空格切分参数，遇到参数首字节 `:` 时将剩余整段（含空格，允许空串）作为最终 trailing 参数。
4. **零分配线缆编码器 (`IrcWriter`)**：
   - 直接写入调用方缓冲区，按 `@tags` -> `:prefix` -> `command` / 3 位数字回复 -> `params` -> `:trailing\r\n` 顺序编码，并对标签值自动执行 IRCv3 转义；对乱序调用、非法字符与缓冲区溢出严格防护。

---

## 二、 核心 API 速查表 (`lib/irc/irc.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `IRC_TCP_PORT (6667)`, `IRCS_TCP_PORT (6697)`, `IRC_MAX_LINE_LEN (512)`, `IRCV3_MAX_TAGS_LEN (8191)` |
| 数字回复码常量 | `RPL_WELCOME (1)`, `RPL_YOURHOST (2)`, `RPL_CREATED (3)`, `RPL_MYINFO (4)`, `RPL_ISUPPORT (5)`, `RPL_TOPIC (332)`, `RPL_NAMREPLY (353)`, `RPL_ENDOFNAMES (366)`, `RPL_MOTD (372)`, `RPL_ENDOFMOTD (376)`, `ERR_NOSUCHNICK (401)`, `ERR_NOSUCHCHANNEL (403)`, `ERR_NICKNAMEINUSE (433)` |
| `IrcTag` | `{ key: []const u8, value: []const u8, has_value: bool }`；方法 `isClientOnly(): bool` |
| `IrcTagIter` | `init(tags_raw: []const u8)`、`next(*_): IrcTag?` |
| `unescapeTagValue(raw, out)` | IRCv3 标签值反转义写入 `out: []u8`，返回写入长度或 `null`（缓冲区不足） |
| `escapeTagValue(raw, out)` | IRCv3 标签值转义写入 `out: []u8`，返回写入长度或 `null`（缓冲区不足） |
| `IrcPrefix` | `{ raw, nick, user, host: []const u8 }`；方法 `isServer(): bool`、`isUser(): bool` |
| `parseIrcPrefix(raw)` | 切分 `nick!user@host` 或纯 `servername` |
| `IrcParamIter` | `init(params_raw: []const u8)`、`next(*_): ([]const u8)?` |
| `IrcMessage` | `{ tags_raw, prefix_raw, command, params_raw: []const u8 }`；方法 `hasTags()`, `hasPrefix()`, `tags()`, `prefix()`, `params()`, `numericCode(): u16?`, `isNumeric()`, `trailing()`, `findTag(key)` |
| `parseIrcLine(line)` | 解析单行 IRC / IRCv3 报文，返回 `IrcMessage?` |
| `IrcWriter` | `init(buf: []u8)`、`writeTag(key, value)`、`writePrefix(prefix)`、`writeCommand(command)`、`writeNumeric(code)`、`writeParam(param)`、`writeTrailing(text)`、`finish(): ([]const u8)?`、`hasError()`、`written()` |

---

## 三、 使用示例

```typescript
import { parseIrcLine, unescapeTagValue, IrcWriter } from "cap:irc";

fn demo() {
    const line = "@time=2026-01-01T00:00:00Z;+draft/reply=a\\sb\\:c :alice!u@host PRIVMSG #cp :hello irc!\r\n";
    if (msg := parseIrcLine(line)) {
        // msg.command == "PRIVMSG"
        // msg.trailing() == "hello irc!"
        if (tag := msg.findTag("+draft/reply")) {
            let decoded_buf: [64]u8 = [0; 64];
            if (n := unescapeTagValue(tag.value, decoded_buf[0..])) {
                // decoded_buf[0..n] == "a b;c"
                * = n;
            }
        }
    }

    let wire_buf: [512]u8 = [0; 512];
    let w = IrcWriter.init(wire_buf[0..]);
    * = w.writeTag("msgid", "42");
    * = w.writePrefix("alice!u@host");
    * = w.writeCommand("PRIVMSG");
    * = w.writeParam("#cp");
    * = w.writeTrailing("hello irc!");
    if (encoded := w.finish()) {
        // encoded == "@msgid=42 :alice!u@host PRIVMSG #cp :hello irc!\r\n"
        * = encoded;
    }
}
```
