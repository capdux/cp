# `cap:autolink` — GFM 宽松自动链接（URL / WWW / Email）扫描器

源自 Bun `md/autolinks.cp`，纯 CP 零依赖实现 GitHub Flavored Markdown 宽松自动链接检测（`http://`/`https://`/`ftp://`、`www.` 与邮箱地址），内置边界检查、尾部不配对右括号 `)` 裁剪及尾部 HTML 实体 `&name;` 裁剪。

## 导出符号

- `AutolinkKind`（`url` / `www` / `email`）、`AutolinkSpan`（`beg`/`end`/`kind`/`slice`/`len`）、`ScanUrlResult`
- 核心扫描：`findPermissiveAutolink`、`findNextAutolink`、`scanUrlComponent`、`postProcessAutolinkEnd`
- 边界与列表辅助：`checkLeftBoundary`、`checkRightBoundary`、`isInSet`、`isListBullet`、`isListItemMark`
