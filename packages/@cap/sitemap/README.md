# `@cap/sitemap`

Zero-heap Sitemaps.org 0.9 XML sitemap (`<urlset>`) and sitemap index (`<sitemapindex>`) generator in pure CP, modeled after npm `sitemap` (8.82M/wk). Features strict XML entity escaping, optional `<lastmod>`, `<changefreq>`, `<priority>`, and overflow-safe streaming writes.

零堆分配纯 CP Sitemaps.org 0.9 XML 站点地图生成器，对标 npm `sitemap`（882 万/周下载），支持 `<urlset>` 与 `<sitemapindex>` 生成、严格 XML 实体转义（防注入）与缓冲区溢出保护。

## Installation

```bash
cap add @cap/sitemap
```

## API

| Type / Method | Description |
| --- | --- |
| `SitemapWriter.init(buf)` | Start a `<urlset>` XML document in caller buffer |
| `addUrl(entry: SitemapUrl)` | Append `<url>` with escaped `<loc>`, optional `<lastmod>`, `<changefreq>`, `<priority>` |
| `finish()` | Close `</urlset>` and return total bytes written (`-1` on overflow) |
| `SitemapIndexWriter.init(buf)` | Start a `<sitemapindex>` XML document |
| `addSitemap(loc, lastmod)` | Append a `<sitemap>` entry |

## License

MIT
