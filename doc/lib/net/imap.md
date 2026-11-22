# `cap:imap` — RFC 3501 / RFC 9051 IMAP4rev1/2 线协议编解码与增量解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

> **模块路径**: `lib/imap/`  
> **导入协议**: `import { ImapStreamParser, ImapCommandCodec, ImapStatus, ImapResponseKind, ImapError, parseCapability, parseSearchUids, parseExistsCount, parseFetchFlags } from "cap:net";`  
> **动态挂载**: `import cap from "cap";` -> `cap.imap.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:imap` 是纯 CP 实现、零 `std.posix` / `@import("bun")` 依赖的无状态 IMAP4rev1/2 线协议编解码与增量响应解析引擎：

1. **增量分块响应解析（`ImapStreamParser`）**：
   - 支持任意分块边界（单字节或跨包拆帧）的 `\r\n` 行定界解析。
   - 完整支持同步 `{N}\r\n` 与非同步 `{N+}\r\n` Literal 二进制块跨包读取（Literal 内部允许包含任意 `\r\n` 与二进制字节），以及单条响应内包含多个 Literal 的拆帧重组。
   - 识别四类标准响应（`ImapResponseKind`）：
     - `.tagged`：`A0001 OK [READ-WRITE] SELECT completed`
     - `.untagged_status`：`* OK [CAPABILITY IMAP4rev1 STARTTLS] Ready`
     - `.untagged_data`：`* 42 EXISTS`、`* SEARCH 1 5 42`、`* 12 FETCH (...)`（附带有序 `literals` 切片）
     - `.continuation`：`+ Ready for additional command text`
   - 严格校验 Literal 长度与截断拦截：调用 `markEof()` 或 `finish()` 后若存在未完整接收的 Literal，立即返回 `ImapError.TruncatedLiteral`；若存在未闭合行则返回 `ImapError.UnexpectedEof`。
2. **专用语义解析辅助**：
   - `parseCapability(allocator, line)`：解析 `* CAPABILITY ...` 或 `[CAPABILITY ...]` 能力列表。
   - `parseSearchUids(allocator, line)`：解析 `* SEARCH 1 5 42 108` 为 `[]u32`。
   - `parseExistsCount(line)`：解析 `* 23 EXISTS` / `* 2 RECENT` 返回 `u32?`。
   - `parseFetchFlags(allocator, line)`：从 `FETCH` 响应中提取 `FLAGS (\Seen \Flagged ...)` 标志列表。
3. **命令编码与注入防护（`ImapCommandCodec`）**：
   - 内部维护单调递增标签（`A0001`, `A0002`, ...），支持 `login`、`select`、`examine`、`search`、`fetch`、`store`、`expunge`、`logout`、`noop`。
   - 对用户名、密码、邮箱名执行严格引号转义（转义 `"` 与 `\`），并拦截所有参数中的 `\r` / `\n` / `\0` 注入，返回 `ImapError.CommandInjection`。

---

## 二、 核心 API 速查表 (`lib/imap/imap.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `ImapError` | `InvalidTag`, `InvalidResponse`, `UnexpectedEof`, `TruncatedLiteral`, `CommandInjection`, `InvalidNumber`, `OutOfMemory` |
| `ImapStatus` | `Ok`, `No`, `Bad`, `Preauth`, `Bye`；方法：`fromBytes(s)`, `toString()` |
| `ImapResponseKind` | 带标签联合：`tagged`, `untagged_status`, `untagged_data`, `continuation`；方法：`deinit(*_, allocator)` |
| `ImapStreamParser` | `init(allocator)`, `deinit(*_)`, `feed(*_, chunk)`, `markEof(*_)`, `finish(*_)`, `nextResponse(*_) -> (ImapResponseKind?) ! ImapError` |
| `parseCapability` | `parseCapability(allocator, line) -> ([][]const u8) ! ImapError` |
| `parseSearchUids` | `parseSearchUids(allocator, line) -> ([]u32) ! ImapError` |
| `parseExistsCount` | `parseExistsCount(line) -> u32?` |
| `parseFetchFlags` | `parseFetchFlags(allocator, line) -> ([][]const u8) ! ImapError` |
| `ImapCommandCodec` | `init()`, `login`, `select`, `examine`, `search`, `fetch`, `store`, `expunge`, `logout`, `noop` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    ImapStreamParser,
    ImapCommandCodec,
    parseCapability,
    parseExistsCount,
    parseSearchUids,
    parseFetchFlags,
} from "cap:net";

fn demo(alloc: std.mem.Allocator) !void {
    let codec = ImapCommandCodec.init();
    const cmd1 = codec.login(alloc, "alice@cp.dev", "p@ss\"word\\1")?;
    defer alloc.free(cmd1); // A0001 LOGIN "alice@cp.dev" "p@ss\"word\\1"\r\n

    const cmd2 = codec.select(alloc, "INBOX")?;
    defer alloc.free(cmd2); // A0002 SELECT "INBOX"\r\n

    let parser = ImapStreamParser.init(alloc);
    defer parser.deinit();

    parser.feed("* OK [CAPABILITY IMAP4rev1 AUTH=PLAIN] Ready\r\n")?;
    if (resp := parser.nextResponse()?) {
        let mut_resp = resp;
        defer mut_resp.deinit(alloc);
        switch (mut_resp) {
            .untagged_status u => {
                if (code := u.code) {
                    const caps = parseCapability(alloc, code)?;
                    defer {
                        for (caps) |c| alloc.free(c);
                        alloc.free(caps);
                    }
                }
            },
            else => {},
        }
    }
}
```
