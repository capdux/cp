# cap:http_early_hints

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 8297 HTTP `103 Early Hints` 与 RFC 8288 `Link` 头部规范），提供纯 CP、零外部依赖、零堆分配的预加载 `Link` 头流式解析与 `103 Early Hints` 报文块序列化库。

## 导入方式

```cp
import {
    EarlyHintRel,
    EarlyHintLink,
    EarlyHintIter,
    formatEarlyHintLink,
    formatEarlyHints103Block,
} from "cap:http_early_hints";
```

## 核心类型与 API

- `EarlyHintRel`：`preload`、`preconnect`、`modulepreload`、`dns_prefetch`、`other`。
- `EarlyHintLink`：`uri: []const u8`、`rel: EarlyHintRel`、`rel_raw: []const u8`、`as_type: ([]const u8)?`、`crossorigin: ([]const u8)?`、`fetchpriority: ([]const u8)?`、`nopush: bool`：
  - `isPreloadable(): bool`：判定是否为 `preload`/`preconnect`/`modulepreload`/`dns-prefetch`。
- `EarlyHintIter`：`init(link_header: []const u8)` / `next(*_): EarlyHintLink?`（逗号分隔多条目，尖括号与引号感知，自动跳过未闭合 `<...>` 或缺失 `rel` 的畸形项）。
- `formatEarlyHintLink(link: EarlyHintLink, out: []u8): ([]const u8)?`：规范化输出单条 RFC 8288 `Link` 值（防 CRLF 注入，缓冲不足返回 `null`）。
- `formatEarlyHints103Block(links: []const EarlyHintLink, out: []u8): ([]const u8)?`：格式化完整 HTTP/1.1 `HTTP/1.1 103 Early Hints\r\nLink: ...\r\n\r\n` 报文块。

## 使用示例

```cp
import { EarlyHintIter, formatEarlyHints103Block } from "cap:http_early_hints";

fn example() {
    let it = EarlyHintIter.init("</app.css>; rel=preload; as=style; nopush, <https://cdn.cp.dev>; rel=preconnect");
    if (l1 := it.next()) {
        if (l2 := it.next()) {
            const links: [2]EarlyHintLink = [l1, l2];
            let buf: [256]u8 = undefined;
            if (block := formatEarlyHints103Block(links[0..2], &buf)) {
                * = block;
            }
        }
    }
}
```
