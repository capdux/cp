# CP 标准库：RFC 9309 `robots.txt` 与 XML Sitemap 0.9 引擎 (`cap:sitemap`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:web`，导入请使用 `import { ... } from "cap:web"`。

## 一、 架构总览与设计准则

`cap:sitemap` 是 CP 语言官方内置的纯 CP 网页采集与站点地图标准库，**零外部 C 依赖、零 `std.posix`、零 `@import("bun")`**，基于 `cap:xml` 构建，为网页采集器、AI Agent 搜索爬虫与静态站点生成器提供工业级 **RFC 9309 `robots.txt` 解析/最长前缀匹配引擎** 与 **XML Sitemap 0.9 / Sitemap Index 解析与生成器**。既可通过 `import { ... } from "cap:web"` 导入，也支持 `import cap from "cap"; cap.sitemap.*` 动态懒挂载调用。

```text
┌──────────────────────────────────────────────────────────────────────────┐
│               CP 统一爬虫规则与 Sitemap 引擎 (cap:sitemap)               │
├──────────────────────────────┬───────────────────────────────────────────┤
│ RFC 9309 规则引擎 (sitemap.cp)│ Sitemap 0.9 编解码器 (sitemap.cp)         │
│ RobotsRules.parse (Arena)    │ SitemapCodec.detectKind (UrlSet / Index)  │
│ RobotsRules.isAllowed        │ SitemapCodec.parseUrlSet / parseIndex     │
│ RobotsRules.crawlDelayMs     │ SitemapCodec.buildUrlSet / buildIndex     │
│ RobotsRules.sitemaps         │ 50,000 URL 上限校验 & XML 安全实体转义     │
└──────────────────────────────┴───────────────────────────────────────────┘
```

### 1. 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sitemap/sitemap.cp` | `// Source: cp/lib/sitemap/sitemap.cp (RFC 9309 robots.txt & XML Sitemap 0.9 Codec)` | `SitemapError`、`ChangeFreq`、`SitemapKind`、`RobotsRules`、`SitemapUrl`、`SitemapIndexItem`、`SitemapCodec` |
| `lib/sitemap/index.cp` | `// Source: cp/lib/sitemap/index.cp (Unified Exports for cap:sitemap)` | `cap:sitemap` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与枚举

- **`SitemapError`**：`InvalidXml`、`InvalidSitemap`、`InvalidPriority`、`MaxUrlsExceeded`、`OutOfMemory`。
- **`ChangeFreq`**：`Always`、`Hourly`、`Daily`、`Weekly`、`Monthly`、`Yearly`、`Never`。
  - `toString(): []const u8`：返回小写规范字符串（如 `"daily"`）。
  - `ChangeFreq.parse(s: []const u8): ChangeFreq?`：大小写不敏感解析。
- **`SitemapKind`**：`UrlSet`、 `Index`。

### 2. RFC 9309 `robots.txt` 引擎（`RobotsRules`）

- `RobotsRules.parse(alloc: std.mem.Allocator, txt: []const u8): RobotsRules ! SitemapError`
  - 自动剥离 UTF-8 BOM 与 `#` 行注释，支持多组 `User-agent`（连续声明共享规则组）、`Allow`、`Disallow`（空值不禁用任何路径）、`Crawl-delay`（支持小数秒）及全局 `Sitemap:` 声明。
- `rules.deinit()`：$O(1)$ 释放 Arena 托管的规则与字符串内存。
- `rules.isAllowed(user_agent: []const u8, path: []const u8): bool`
  - 优先精确匹配 `user_agent`（大小写不敏感），无精确匹配时回退 `*` 通配组；
  - 组内按 **最长路径模式匹配长度** 胜出，长度相同时 `Allow` 优于 `Disallow`；
  - 原生支持 `*` 任意多字符通配符与 `$` 行尾锚点。
- `rules.crawlDelayMs(user_agent: []const u8): u64?`：返回匹配组或 `*` 回退组的抓取延迟（毫秒）。
- `rules.sitemaps(): []const []const u8`：返回全部 `Sitemap:` URL 列表。

### 3. Sitemap 0.9 / Sitemap Index 数据模型与编解码器（`SitemapCodec`）

- **`SitemapUrl`**：`loc: []const u8`、`lastmod: []const u8`、`changefreq: ChangeFreq?`、`priority: f64?`（`0.0..1.0`）。
- **`SitemapIndexItem`**：`loc: []const u8`、`lastmod: []const u8`。
- **`SitemapCodec`**：
  - `MAX_URLS: usize = 50000`
  - `detectKind(xml_text: []const u8): SitemapKind ! SitemapError`
  - `parseUrlSet(alloc, xml_text): ([]SitemapUrl) ! SitemapError`（配合 `SitemapCodec.freeUrlSet(alloc, urls)` 释放）
  - `parseIndex(alloc, xml_text): ([]SitemapIndexItem) ! SitemapError`（配合 `SitemapCodec.freeIndex(alloc, items)` 释放）
  - `buildUrlSet(alloc, urls: []const SitemapUrl): ([]u8) ! SitemapError`（校验 `priority` 在 `0.0..1.0`、`MAX_URLS` 上限及 XML 实体转义）
  - `buildIndex(alloc, items: []const SitemapIndexItem): ([]u8) ! SitemapError`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { RobotsRules, SitemapCodec, SitemapUrl, ChangeFreq } from "cap:web";
import cap from "cap";

pub fn demoSitemapPipeline(): ! {
    const alloc = std.heap.page_allocator;
    const robots_txt =
        "User-agent: *\n" ++
        "Disallow: /admin/\n" ++
        "Allow: /admin/public/\n" ++
        "Crawl-delay: 1.5\n" ++
        "Sitemap: https://example.com/sitemap.xml\n";

    let rules = RobotsRules.parse(alloc, robots_txt)?;
    defer rules.deinit();

    const ok = rules.isAllowed("MyAgent", "/admin/public/info.html"); // true
    const blocked = rules.isAllowed("MyAgent", "/admin/secret");      // false
    * = ok;
    * = blocked;

    const urls = [
        SitemapUrl{
            loc: "https://example.com/search?q=cp&lang=zh",
            lastmod: "2026-11-20",
            changefreq: .Daily,
            priority: 0.8,
        },
    ];
    const xml = cap.sitemap.SitemapCodec.buildUrlSet(alloc, &urls)?;
    defer alloc.free(xml);
}
```
