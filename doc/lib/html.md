# CP 标准库：HTML5 解析、实体编解码、白名单清洗与 AI 网页提取手册 (`cap:html`)

> 参考来源：`/data/dev/bun/docs/runtime/html-rewriter.mdx`, `/data/dev/bun/docs/guides/util/escape-html.mdx`, `/data/dev/bun/src/lolhtml_sys/lol_html.cp` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:html` 是 CP 语言官方内置的工业生产级 HTML5 解析、内容安全清洗与 AI Agent 网页语义提取标准库，对标 Bun 的 `HTMLRewriter`（`/data/dev/bun/src/lolhtml_sys/lol_html.cp`）与 WHATWG HTML5 字符引用 / 安全清洗规范，提供纯 CP 零外部依赖的完整工具链：

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 / AI Agent RAG 管线                       │
├───────────────────┬──────────────────┬──────────────────┬────────────────────┤
│  HTML5 词法与重写   │ WHATWG 实体编解码  │  白名单内容清洗器   │  AI 网页语义提取器   │
│  HtmlTokenizer    │  HtmlEntities    │  HtmlSanitizer   │  HtmlExtractor     │
│  CssSelector      │  encodeText/Attr │  SanitizerPolicy │  HtmlDocumentSummary│
│  HtmlRewriter     │  decode (C1/FFFD)│  isSafeUrl       │  title/meta/og/link│
└───────────────────┴──────────────────┴──────────────────┴────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/html/index.cp` | `// Source: /data/dev/bun/src/lolhtml_sys/lol_html.cp` | `cap:html` 统一公开导出入口 |
| `lib/html/tokenizer.cp` | `// Source: /data/dev/bun/src/lolhtml_sys/lol_html.cp` | `HtmlTokenizer`、`HtmlToken`、`HtmlTokenKind`、`HtmlAttribute` HTML5 词法引擎 |
| `lib/html/selector.cp` | `// Source: /data/dev/bun/src/lolhtml_sys/lol_html.cp` | `CssSelector` 复合选择器、`HtmlElement` 变更句柄与 `HtmlRewriter` 流式重写器 |
| `lib/html/escape.cp` | `// Source: /data/dev/bun/src/md/entity.cp` | `HtmlEscape` 零拷贝快路径转义与实体解码 |
| `lib/html/entities.cp` | `// Source: cp/lib/html/entities.cp (WHATWG HTML5 Character Reference Codec)` | `HtmlEntities` WHATWG HTML5 206+ 命名实体与十进制/十六进制/C1 替换编解码 |
| `lib/html/sanitizer.cp` | `// Source: cp/lib/html/sanitizer.cp (Allowlist HTML5 Content Sanitizer)` | `SanitizerPolicy` 与 `HtmlSanitizer` 白名单流式清洗与 URL 协议防护 |
| `lib/html/extractor.cp` | `// Source: cp/lib/html/extractor.cp (AI Agent HTML Visible Text & Metadata Extractor)` | `HtmlExtractor`、`HtmlDocumentSummary` 单遍网页元数据与 RAG 规范化纯文本提取 |

---

## 二、 核心 API 详解

### 1. `HtmlEntities` (WHATWG HTML5 实体编解码器)

- `HtmlEntities.encodeText(alloc, text): ([]u8)!`：转义文本节点中的 `&`、`&lt;`/`<`、`>`（输出 `&amp;`、`&lt;`、`&gt;`），调用方负责释放返回切片。
- `HtmlEntities.encodeAttr(alloc, text): ([]u8)!`：额外转义属性值中的 `"`（`&quot;`）与 `'`（`&#39;`）。
- `HtmlEntities.decode(alloc, html): ([]u8)!`：
  - 支持 **206 个高频 HTML5 命名实体**（`&amp;`、`&lt;`、`&gt;`、`&quot;`、`&apos;`、`&nbsp;`、`&copy;`、`&reg;`、`&mdash;`、`&ndash;`、`&hellip;`、`&euro;`、`&yen;`、`&times;`、`&divide;`、`&rarr;`、`&larr;`、希腊字母与常用拉丁重音字符等，O(log N) 二分查找）；
  - 支持十进制 `&#NNN;` 与十六进制 `&#xHHH;` / `&#XHHH;` 数字引用；
  - 严格遵循 WHATWG HTML5 规范处理 C1 控制区（`0x80..0x9F` Windows-1252 映射表，如 `&#x80;` -> `€`、`&#151;` -> `—`、`&#x99;` -> `™`）以及零码点/UTF-16 代理区（`0xD800..0xDFFF`）/越界码点替换为 `U+FFFD`。

### 2. `SanitizerPolicy` 与 `HtmlSanitizer` (白名单 HTML5 内容清洗)

- `SanitizerPolicy`：
  - `SanitizerPolicy.defaultSafe()`：允许安全语义与排版标签（`p`、`div`、`span`、`h1`..`h6`、`a`、`img`、`ul`/`ol`/`li`、`table`、`pre`/`code`、`blockquote` 等）与安全属性（`href`/`src`/`alt`/`title`/`class`/`id` 等），限制最大嵌套深度（默认 `256`）。
  - `SanitizerPolicy.strictTextOnly()`：剥离全部标签并输出安全实体转义后的纯文本。
  - `SanitizerPolicy.custom(tags, attrs, url_attrs)`：自定义允许标签与安全属性白名单。
- 危险容器彻底清除：始终阻止 `<script>`、`<style>`、`<iframe>`、`<object>`、`<embed>`、`<svg>`、`<math>`、`<applet>`、`<base>`、`<frame>`、`<frameset>`、`<noscript>`、`<template>` 及其内部载荷。
- `HtmlSanitizer.isSafeUrl(url): bool`：过滤首尾空白并拒绝内嵌控制字符（`\t`/`\n`/`\r`/`\x00` 等绕过），仅放行 `http://`、`https://`、`mailto:`、`#` 锚点及相对路径；严格拦截 `javascript:`、`vbscript:`、`data:text/html` 等危险协议。
- `HtmlSanitizer.sanitize(alloc, html, policy): ([]u8)!`：流式状态机清洗，剔除 `on*` 事件属性与危险协议属性（含实体编码绕过解码检测），移除 HTML 注释/DOCTYPE/CDATA，自动闭合未闭合允许标签，并对输出文本和属性进行安全实体转义。

### 3. `HtmlExtractor` 与 `HtmlDocumentSummary` (AI Agent 网页语义提取)

- 数据结构：
  - `HtmlLink`：`href: []const u8`、`text: []const u8`
  - `HtmlMeta`：`name: []const u8`、`content: []const u8`
  - `HtmlHeading`：`level: u8`、`text: []const u8`
  - `HtmlDocumentSummary`：`title`、`description`、`canonical_url`、`og_title`、`og_image`、`links`、`headings`、`metas`、`plain_text`，提供 `deinit(*_)` 确定性释放全部分配内存。
- `HtmlExtractor.extract(alloc, html): HtmlDocumentSummary!`：
  - 单遍流式解析：自动跳过 `<script>`、`<style>`、`<noscript>`、`<template>` 与注释；
  - 自动解码实体并提取 `<title>`、`<meta name="description">`、`<meta property="og:title">`、`<meta property="og:image">`、`<link rel="canonical">`、`<h1>`..`<h6>` 与 `<a>` 链接；
  - 将可见正文在块级标签处折叠空白为单空格或单换行，生成紧凑规范的 `plain_text`，直接服务于 AI Agent 网页浏览与 RAG 管线。

### 4. `HtmlTokenizer`、`CssSelector`、`HtmlEscape` 与 `HtmlRewriter`

- 保持原有全部 API 100% 向后兼容：`HtmlTokenizer.init` / `next` / `tokenizeAll`、`CssSelector.parse` / `matchesToken`、`HtmlRewriter`、`HtmlEscape`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    HtmlEntities,
    SanitizerPolicy,
    HtmlSanitizer,
    HtmlExtractor,
} from "cap:html";

pub fn demoHtmlSecurityAndExtract(): !void {
    const allocator = std.heap.page_allocator;

    // 1. WHATWG 实体解码（含 C1 控制区 Windows-1252 映射）
    const decoded = try HtmlEntities.decode(allocator, "Price: &euro;99 &#x97; &#x1F600;");
    defer allocator.free(decoded);

    // 2. 白名单 HTML 清洗（防 XSS 与协议绕过）
    const dirty = "<p onclick=\"alert(1)\">Hello <a href=\"javascript:evil()\">bad</a> <a href=\"https://cp-lang.org\">CP</a></p><script>steal()</script>";
    const clean = try HtmlSanitizer.sanitize(allocator, dirty, SanitizerPolicy.defaultSafe());
    defer allocator.free(clean);

    // 3. AI Agent 网页结构化提取
    let summary = try HtmlExtractor.extract(
        allocator,
        "<html><head><title>CP Docs</title><meta name=\"description\" content=\"Fast &amp; Safe\"></head><body><h1>Intro</h1><p>Welcome</p></body></html>",
    );
    defer summary.deinit();
    * = summary.plain_text;
}
```

---

## 零分配实体转义快捷函数（`esc.cp`，收敛自 `html_escape`）

- `needsHtmlEscape(src: []const u8): bool`
- `escapeHtml(src: []const u8, out: []u8): ([]const u8)?`
- `unescapeHtml(src: []const u8, out: []u8): ([]const u8)?`

---

## 上游 Bun 行为契约与安全防御注意事项

1. **流式重写与零 DOM 开销（参考 `/data/dev/bun/docs/runtime/html-rewriter.mdx`）**：
   - `HtmlRewriter` 对标 Cloudflare `lol-html` / Bun `HTMLRewriter` 架构：基于单遍 `HtmlTokenizer` 流式匹配 `CssSelector` 并即时改写标签属性或注入前后内容，无需在内存中构建完整 DOM 树，适合反向代理、边缘计算与高频 HTML 改写。
2. **零拷贝快速转义路径（`needsHtmlEscape` / `escapeHtml`）**：
   - 对标 `Bun.escapeHTML`：先用 `needsHtmlEscape` 扫描输入是否包含 `&`、`<`、`>`、`"`、`'` 五类危险字符，无特殊字符时可零拷贝直接透传原文，显著提升模板渲染吞吐。
3. **白名单清洗与 URL 协议防线（`HtmlSanitizer`）**：
   - 渲染用户提交或第三方抓取的富文本 HTML 时，使用 `SanitizerPolicy.defaultSafe()` 自动移除 `<script>`、`<iframe>`、`on*` 事件属性，并通过 `isSafeUrl` 阻断 `javascript:` / `vbscript:` / 畸形 `data:` URI。
4. **WHATWG C1 控制区与非法码点替换（`HtmlEntities`）**：
   - 解码数字字符引用（`&#...;` / `&#x...;`）时严格遵循 WHATWG HTML5 规范：自动映射 Windows-1252 C1 替换表（`0x80..0x9F`）并将 `0x00`、孤立代理项与超界码点替换为 `U+FFFD`。
