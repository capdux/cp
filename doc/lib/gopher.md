# `cap:gopher` — RFC 1436 Gopher 协议与 Menu 目录零分配编解码库

纯 CP、零堆分配（zero-alloc）的 RFC 1436 Gopher 协议请求/响应与 Menu 目录条目编解码库，支持根目录/搜索请求解析、16 种条目类型常量、`h` 类型 `URL:` 剥离、零拷贝 Menu 迭代及防注入编码。

## 导入

```cp
import {
    GOPHER_TCP_PORT,
    TYPE_TEXT,
    TYPE_DIR,
    TYPE_CSO,
    TYPE_ERROR,
    TYPE_BINHEX,
    TYPE_DOS,
    TYPE_UUENCODED,
    TYPE_SEARCH,
    TYPE_TELNET,
    TYPE_BINARY,
    TYPE_MIRROR,
    TYPE_GIF,
    TYPE_IMAGE,
    TYPE_INFO,
    TYPE_HTML,
    TYPE_SOUND,
    GopherRequest,
    parseGopherRequest,
    encodeGopherRequest,
    GopherItem,
    parseGopherItem,
    encodeGopherItem,
    GopherMenuIter,
} from "cap:gopher";
```

## 核心 API

- **请求编解码**：`parseGopherRequest(line): GopherRequest?`（允许空 selector、按首个 `\t` 拆分搜索词）、`encodeGopherRequest(selector, search_query, out: []u8): usize?`（防 `\r`/`\n`/`\0` 注入）。
- **目录条目编解码**：`parseGopherItem(line): GopherItem?`（支持 `isInfo()`, `isDir()`, `isText()`, `isSearch()`, `isHtmlUrl()`, `htmlUrl()`）、`encodeGopherItem(item_type, display, selector, host, port, out: []u8): usize?`（防 `\t`/`\r`/`\n` 注入）。
- **Menu 流式迭代**：`GopherMenuIter`（`init`, `next`, `isTerminated`, `hasError`），按 `\r\n` 或 `\n` 逐行解析直至独立终止行 `.`。

## 示例

```cp
import std from "std";
import { TYPE_DIR, encodeGopherItem, GopherMenuIter } from "cap:gopher";

fn example() {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    if (n := encodeGopherItem(TYPE_DIR, "Docs", "/docs", "gopher.cp.dev", 70, buf[0..])) {
        * = buf[0..n];
    }

    let it = GopherMenuIter.init("iWelcome\t\terror.host\t1\r\n.\r\n");
    while (true) {
        if (item := it.next()) {
            * = item;
        } else {
            break;
        }
    }
}
```
