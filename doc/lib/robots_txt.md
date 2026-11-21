# `cap:robots_txt` — RFC 9309 `robots.txt` 解析与 URL 匹配引擎

> 来源：吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9309 `robots.txt` Parser & Matcher）

`cap:robots_txt` 提供纯 CP、零外部依赖、零堆分配的 RFC 9309 `robots.txt` 规则解析、通配符/锚点路径匹配、`Crawl-delay` 提取与 `Sitemap` 迭代能力。

## 核心函数与类型

### `matchRobotsPattern`

```cp
pub fn matchRobotsPattern(pattern: []const u8, path: []const u8): usize?
```

- 支持 `*` 任意子串通配符与结尾 `$` 精确锚点（线性双指针回溯，无递归与栈溢出风险）。
- 匹配成功返回规则特异度长度 `pattern.len`，否则返回 `null`（空规则返回 `null`）。

### `isAllowed`

```cp
pub fn isAllowed(robots_txt: []const u8, user_agent: []const u8, path: []const u8): bool
```

- 自动剥离 UTF-8 BOM 并忽略 `#` 注释行及行尾注释。
- 按 RFC 9309 大小写不敏感匹配最具体 `User-agent` 组（如 `Googlebot-Image` > `Googlebot` > `*`）。
- 组内遵循最长匹配规则胜出；同长度冲突时 `Allow` 优先于 `Disallow`。
- 空 `Disallow:` 表示全部允许；未命中任何组默认允许。

### `crawlDelaySec`

```cp
pub fn crawlDelaySec(robots_txt: []const u8, user_agent: []const u8): u64?
```

- 提取最具体匹配组的 `Crawl-delay` 秒数。

### `SitemapIter`

```cp
pub struct SitemapIter {
    static fn init(robots_txt: []const u8): SitemapIter
    fn next(*_): ([]const u8)?
}
```

- 零拷贝迭代文档中全部 `Sitemap:` URL（大小写不敏感，自动剥离行尾 `#` 注释）。

## 使用示例

```cp
import { isAllowed, crawlDelaySec, SitemapIter, matchRobotsPattern } from "cap:robots_txt";

const txt = "User-agent: *\nDisallow: /admin/\nAllow: /admin/public\nCrawl-delay: 5\nSitemap: https://example.com/sitemap.xml\n";
// isAllowed(txt, "CapBot", "/admin/secret") == false
// isAllowed(txt, "CapBot", "/admin/public") == true
// crawlDelaySec(txt, "CapBot").? == 5
```
