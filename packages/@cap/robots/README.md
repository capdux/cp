# @cap/robots

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:robots-parser` ([https://github.com/samclarke/robots-parser](https://github.com/samclarke/robots-parser))（原库名 `robots-parser` → CP 包名 `@cap/robots`）

Zero-allocation RFC 9309 `robots.txt` rule parser and URL compliance engine for CP.

## Features

- **Zero heap allocation**: single-pass streaming evaluation directly over `[]const u8`.
- **RFC 9309 compliant**:
  - Case-insensitive `User-agent` matching with most-specific group precedence and `*` wildcard fallback.
  - Longest-match rule resolution with `Allow` winning ties over `Disallow`.
  - Wildcard `*` and end-of-path `$` anchor support.
  - Empty `Disallow:` permits all paths; `#` comments and UTF-8 BOM stripped automatically.
- **`Crawl-delay` & `Sitemap` extraction**: zero-copy `firstSitemap` and per-agent `crawlDelay`.

## Usage

```cp
import { isAllowed, crawlDelay, firstSitemap } from "@cap/robots";

const rules = "User-agent: *\nDisallow: /admin/\nAllow: /admin/public\nCrawl-delay: 5\nSitemap: https://example.com/sitemap.xml\n";
const ok = isAllowed(rules, "CapBot/1.0", "/admin/public/page"); // true
const delay = crawlDelay(rules, "CapBot/1.0"); // 5
const sm = firstSitemap(rules); // "https://example.com/sitemap.xml"
```
