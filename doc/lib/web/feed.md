# CP 标准库：RSS 2.0 / Atom 1.0 / JSON Feed 1.1 统一解析与生成手册 (`cap:feed`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:web`，导入请使用 `import { ... } from "cap:web"`。

## 一、 架构总览与设计准则

`cap:feed` 是 CP 语言官方内置的纯 CP 聚合订阅源解析与生成标准库，**零外部 C 依赖、零 `std.posix`、零 `@import("bun")`**，基于 `cap:xml` 与 `cap:json` 构建，专为 AI Agent 资讯监测、知识库增量同步与静态站点聚合器设计。既可通过 `import { ... } from "cap:web"` 导入，也支持 `import cap from "cap"; cap.feed.*` 动态懒挂载调用。

```text
┌──────────────────────────────────────────────────────────────────────────┐
│                 CP 统一 Feed 引擎 (cap:feed / cap.feed)                   │
├──────────────────────────────┬───────────────────────────────────────────┤
│ 格式识别与统一解析 (feed.cp)  │ 跨格式规范生成 (feed.cp)                   │
│ FeedCodec.detectFormat       │ FeedCodec.toRss2  (RSS 2.0 + CDATA)       │
│ FeedCodec.parse (Arena 托管) │ FeedCodec.toAtom1 (Atom 1.0 XML)          │
│ RSS 2.0 / Atom 1.0 / JF 1.1  │ FeedCodec.toJsonFeed (JSON Feed 1.1)      │
└──────────────────────────────┴───────────────────────────────────────────┘
```

### 1. 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/feed/feed.cp` | `// Source: cp/lib/feed/feed.cp (RSS 2.0 / Atom 1.0 / JSON Feed 1.1 Parser & Codec)` | `FeedError`、`FeedFormat`、`FeedItem`、`Feed`、`FeedCodec` |
| `lib/feed/index.cp` | `// Source: cp/lib/feed/index.cp (Unified Exports for cap:feed)` | `cap:feed` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与格式枚举

- **`FeedError`**：`UnsupportedFormat`、`InvalidXml`、`InvalidJson`、`MissingTitle`、`OutOfMemory`。
- **`FeedFormat`**：`Rss2`、`Atom1`、`JsonFeed1`。

### 2. 统一数据模型（`FeedItem` / `Feed`）

- **`FeedItem`**：
  - `id: []const u8`（RSS `guid` / Atom `id` / JSON Feed `id`）
  - `title: []const u8`
  - `link: []const u8`（RSS `link` 或 `enclosure@url` / Atom `link[rel=alternate]@href` / JSON Feed `url`）
  - `summary: []const u8`（RSS `description` / Atom `summary` / JSON Feed `summary`）
  - `content_html: []const u8`（RSS `content:encoded` / Atom `content` / JSON Feed `content_html`）
  - `author: []const u8`（RSS `author` 或 `dc:creator` / Atom `author/name` / JSON Feed `authors[0].name`）
  - `published: []const u8`（RSS `pubDate` / Atom `published` 或 `updated` / JSON Feed `date_published`）
  - `tags: [][]const u8`（RSS `<category>` / Atom `<category term="..."/>` / JSON Feed `tags`）
- **`Feed`**：
  - `format: FeedFormat`、`title: []const u8`、`link: []const u8`、`description: []const u8`、`language: []const u8`、`updated: []const u8`、`items: []FeedItem`
  - `Feed.init(format)`：创建空 `Feed`；
  - `feed.deinit()`：$O(1)$ 释放由 `FeedCodec.parse` 分配的 Arena 内存（手动构造且未挂载 Arena 时安全空操作）。

### 3. 统一编解码器（`FeedCodec`）

- `FeedCodec.detectFormat(raw: []const u8): FeedFormat ! FeedError`
  - 自动剥离 UTF-8 BOM 与 XML 声明/注释/DOCTYPE 前导，精准识别 `<rss` / `<rdf:RDF`（`Rss2`）、`<feed`（`Atom1`）以及含 `https://jsonfeed.org/version/` 的 JSON 对象（`JsonFeed1`），非 Feed 内容返回 `FeedError.UnsupportedFormat`。
- `FeedCodec.parse(alloc: std.mem.Allocator, raw: []const u8): Feed ! FeedError`
  - 自动检测格式并解析为统一 `Feed`（调用方需在使用完毕后调用 `feed.deinit()`）。
- `FeedCodec.toRss2(alloc: std.mem.Allocator, feed: Feed): ([]u8) ! FeedError`
  - 生成合规 RSS 2.0 XML（含 `<?xml ...?>` 声明、自动实体转义与含标记内容 CDATA 封装）。
- `FeedCodec.toAtom1(alloc: std.mem.Allocator, feed: Feed): ([]u8) ! FeedError`
  - 生成合规 Atom 1.0 XML（含 `xmlns="http://www.w3.org/2005/Atom"`、`<link rel="alternate" href="..."/>`、`<author><name>` 与 `<category term="..."/>`）。
- `FeedCodec.toJsonFeed(alloc: std.mem.Allocator, feed: Feed): ([]u8) ! FeedError`
  - 生成合规 JSON Feed 1.1（`version: "https://jsonfeed.org/version/1.1"`、`authors` 数组、`tags` 数组）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { FeedCodec, FeedFormat } from "cap:web";
import cap from "cap";

pub fn demoFeedPipeline(): ! {
    const alloc = std.heap.page_allocator;
    const raw_atom =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<feed xmlns=\"http://www.w3.org/2005/Atom\">\n" ++
        "  <title>CP Agent Updates</title>\n" ++
        "  <id>https://cp-lang.org/feed</id>\n" ++
        "  <link rel=\"alternate\" href=\"https://cp-lang.org\"/>\n" ++
        "  <entry>\n" ++
        "    <id>urn:cp:1</id>\n" ++
        "    <title>Feed Engine Ready</title>\n" ++
        "    <link rel=\"alternate\" href=\"https://cp-lang.org/blog/feed\"/>\n" ++
        "    <author><name>Core Team</name></author>\n" ++
        "    <category term=\"release\"/>\n" ++
        "  </entry>\n" ++
        "</feed>";

    let feed = FeedCodec.parse(alloc, raw_atom)?;
    defer feed.deinit();

    // 转换为 JSON Feed 1.1
    const jf_bytes = FeedCodec.toJsonFeed(alloc, feed)?;
    defer alloc.free(jf_bytes);

    // 动态命名空间 cap.feed
    const fmt = cap.feed.FeedCodec.detectFormat(jf_bytes)?;
    * = fmt;
}
```
