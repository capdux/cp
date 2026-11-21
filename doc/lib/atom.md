# `cap:atom` — 纯 CP RFC 4287 Atom 1.0 XML Feed 零分配解析与序列化库

## 概述

`cap:atom` 提供纯 CP、零外部依赖、零堆分配的 RFC 4287 Atom 1.0 Feed 解析器、流式 `<entry>` 迭代器与 XML 转义序列化器，与 `cap:rss` 配套补齐 Syndication Feed 标准库生态。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `AtomFeed` | Feed 顶层元数据：`id`, `title`, `updated`, `link_href`, `subtitle` |
| `AtomEntry` | Entry 条目元数据：`id`, `title`, `updated`, `link_href`, `summary`, `author_name` |
| `isAtomFeed(xml)` | 快速校验文档是否包含合法 `<feed>...</feed>` 结构 |
| `parseAtomFeed(xml)` | 解析 `<feed>` 顶层元数据，自动跳过 `<entry>` 内部同名子标签，支持 CDATA 剥离与 `<link rel="alternate" href="..."/>` 优先匹配 |
| `AtomEntryIter` | 零分配流式 `<entry>` 迭代器：`init(xml)`、`next(*_): AtomEntry?`（支持 `<author><name>...</name></author>` 与自闭合 `<link/>`） |
| `formatAtomEntry(entry, out)` | 规范化 XML 转义 `&`/`<`/`>`/`"`/`'` 并序列化 `<entry>...</entry>`，缓冲不足返回 `null` |

## 示例

```cp
import { parseAtomFeed, AtomEntryIter, formatAtomEntry } from "cap:atom";

fn example(xml: []const u8): bool {
    if (feed := parseAtomFeed(xml)) {
        let it = AtomEntryIter.init(xml);
        if (entry := it.next()) {
            let buf: [512]u8 = undefined;
            return formatAtomEntry(entry, buf[0..]) != null && feed.title.len > 0;
        }
    }
    return false;
}
```
