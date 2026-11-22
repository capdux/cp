# `cap:autolink` — GFM 宽松自动链接（URL / WWW / Email）扫描器

> 参考来源：/data/dev/bun/src/md/autolinks.cp | 吸收日期：2026-11-22

## 概述

`cap:autolink` 吸收自 Bun 原生 Markdown 解析器（`src/md/autolinks.cp`，沿袭 md4c 的 GitHub Flavored Markdown 扩展自动链接算法），纯 CP 零依赖、零堆分配实现 GFM 宽松自动链接检测，覆盖协议 URL（`http://` / `https://` / `ftp://`）、无协议 `www.` 域名以及邮箱地址（Email）。

## 上游规范与边界处理契约

- **三类自动链接识别（`AutolinkKind`）**：
  - `url`：以 `http://`、`https://` 或 `ftp://` 开头的完整链接；
  - `www`：以 `www.` 开头且包含至少两个合法域名分段的裸链接；
  - `email`：形如 `local@domain.tld` 的合法邮箱地址。
- **严格左右边界约束（`checkLeftBoundary` / `checkRightBoundary`）**：
  - 左边界仅允许位于文本起点、空白字符或合法前导标点（如 `(`、`[`、`{`、`*`、`_`、`~` 等）之后，避免在普通单词或标识符中间误触发；
  - 兼容 Markdown 列表项符号（`isListBullet`：`-`/`+`/`*`；`isListItemMark`：额外含 `.`/`)`）与强调定界符边界。
- **GFM 尾部标点与括号平衡裁剪（`postProcessAutolinkEnd`）**：
  - 自动剥离末尾句末标点（如 `?`、`!`、`.`、`,`、`:`、`*`、`_`、`~`）；
  - **括号配对保护**：当链接以 `)` 结尾时，仅当链接内部 `)` 数量多于 `(` 时才裁剪多余的右括号，从而正确保留维基百科式 URL（如 `https://en.wikipedia.org/wiki/Foo_(bar)`）同时剥离外层句子括号（如 `(https://example.com)`）；
  - **末尾 HTML 实体裁剪**：若链接末尾紧跟形如 `&amp;` 或 `&gt;` 的实体引用且以 `;` 结尾，自动将其移出链接范围。

## 导出符号与 API

### 类型

- `AutolinkKind`：`url` / `www` / `email`
- `AutolinkSpan`：`{ beg: usize, end: usize, kind: AutolinkKind }`，提供 `slice(input)` 与 `len()`
- `ScanUrlResult`：`{ end: usize, ok: bool }`

### 核心扫描与辅助函数

| 函数 | 说明 |
| :--- | :--- |
| `findPermissiveAutolink(content, pos, allow_emph)` | 在指定位置检测是否构成合法 GFM 宽松自动链接 |
| `findNextAutolink(content, start_pos)` | 从 `start_pos` 起向后扫描并返回下一个 `AutolinkSpan?` |
| `scanUrlComponent(content, start, start_char, delim_char, allowed_nonalnum, min_components, optional_end_char)` | 按 md4c URL 组件规则扫描主机/路径/查询/片段 |
| `postProcessAutolinkEnd(content, beg, end)` | 执行 GFM 尾部标点、多余右括号与尾部 HTML 实体裁剪 |
| `checkLeftBoundary` / `checkRightBoundary` | 校验自动链接起止边界合法性 |
| `isInSet` / `isListBullet` / `isListItemMark` | 字符集合与 Markdown 列表标记判定 |

## 边界注意事项

1. **零拷贝切片**：`AutolinkSpan` 仅记录原文本的字节区间 `[beg, end)`，无任何堆分配；多轮迭代扫描全篇文本的时间复杂度接近线性。
2. **输入长度与越界安全**：所有扫描函数均对 `start` / `pos` 进行边界守卫，越界或空输入安全返回 `null` / `{ ok: false }`。

## 使用示例

```cp
import { findNextAutolink, AutolinkKind } from "cap:autolink";

const text = "See (https://en.wikipedia.org/wiki/Foo_(bar)), www.example.com, or hi@example.org.";
let pos: usize = 0;
while (span := findNextAutolink(text, pos)) {
    const matched = span.slice(text);
    // 依次匹配:
    // 1. "https://en.wikipedia.org/wiki/Foo_(bar)" (AutolinkKind.url)
    // 2. "www.example.com"                         (AutolinkKind.www)
    // 3. "hi@example.org"                          (AutolinkKind.email)
    * = matched;
    * = AutolinkKind.url;
    pos = span.end;
}
```
