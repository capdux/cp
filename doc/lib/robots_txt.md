# `cap:robots_txt` — RFC 9309 `robots.txt` 解析与 URL 匹配引擎

> 参考来源：/data/dev/bun/src/http/http.cp | 吸收日期：2026-11-22

## 概述

`cap:robots_txt` 吸收自 Bun HTTP 协议工具集（`src/http/http.cp`），提供纯 CP、零外部依赖、零堆分配的 RFC 9309 `robots.txt` 规则解析、通配符/锚点路径匹配、`Crawl-delay` 提取与 `Sitemap` 零拷贝迭代能力，适用于合规爬虫、检索代理与边缘缓存策略判断。

## 规范契约与安全边界

- **无递归线性通配符匹配（防 ReDoS / 栈溢出）**：`matchRobotsPattern` 使用双指针线性回溯算法实现 `*` 任意子串通配符与末尾 `$` 精确锚点匹配，面对恶意构造的大量连续 `*` 规则也不会出现指数级回溯或栈溢出。
- **RFC 9309 组选择与最长匹配胜出**：
  - 自动剥离 UTF-8 BOM 并忽略 `#` 注释行及行尾注释；
  - 大小写不敏感选择最具体的 `User-agent` 组（如 `Googlebot-Image` 优先于 `Googlebot`，再优先于通配 `*`）；
  - 组内遵循最长规则（按 `pattern.len`）胜出原则；当 `Allow` 与 `Disallow` 长度相同时，按 RFC 9309 规定 **`Allow` 优先于 `Disallow`**；
  - 空 `Disallow:` 表示全部允许；未匹配任何组默认允许。

## 核心函数与类型

### `matchRobotsPattern`

```cp
pub fn matchRobotsPattern(pattern: []const u8, path: []const u8): usize?
```

- 支持 `*` 任意子串通配符与结尾 `$` 精确锚点；匹配成功返回规则特异度长度 `pattern.len`，否则返回 `null`（空规则返回 `null`）。

### `isAllowed`

```cp
pub fn isAllowed(robots_txt: []const u8, user_agent: []const u8, path: []const u8): bool
```

### `crawlDelaySec`

```cp
pub fn crawlDelaySec(robots_txt: []const u8, user_agent: []const u8): u64?
```

- 提取最具体匹配组的 `Crawl-delay` 秒数，未声明或数值非法返回 `null`。

### `SitemapIter`

```cp
pub struct SitemapIter {
    static fn init(robots_txt: []const u8): SitemapIter
    fn next(*_): ([]const u8)?
}
```

- 零拷贝迭代文档中全部 `Sitemap:` URL（大小写不敏感，自动剥离行尾 `#` 注释与首尾空白）。

## 使用示例

```cp
import { isAllowed, crawlDelaySec, SitemapIter, matchRobotsPattern } from "cap:robots_txt";

const txt = "User-agent: *\nDisallow: /admin/\nAllow: /admin/public\nCrawl-delay: 5\nSitemap: https://example.com/sitemap.xml\n";
const ok1 = isAllowed(txt, "CapBot", "/admin/secret"); // false
const ok2 = isAllowed(txt, "CapBot", "/admin/public"); // true
const delay = crawlDelaySec(txt, "CapBot");            // 5
let sm = SitemapIter.init(txt);
while (url := sm.next()) { * = url; }
* = ok1; * = ok2; * = delay; * = matchRobotsPattern;
```
