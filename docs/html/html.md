# CP 标准库：HTML5 Tokenizer、实体编解码、选择器与 HTMLRewriter 手册 (`cap:html`)

## 一、 架构总览与设计准则

`cap:html` 是 CP 语言官方内置的 HTML5 解析与流式重写标准库，对标 Bun 的 `HTMLRewriter`（`/data/dev/bun/src/lolhtml_sys/lol_html.cp`）与实体转义子系统（`/data/dev/bun/src/md/entity.cp`），包含完整的 HTML5 Tokenizer、70+ 命名及十进制/十六进制数值实体编解码器、复合 CSS 选择器引擎以及支持元素增删改的流式 `HtmlRewriter`。

```text
┌───────────────────────────────────────────────────────────────────────────┐
│                         CP 用户层应用 (Application)                        │
├──────────────────────────┬─────────────────────────┬──────────────────────┤
│   HTML5 词法分词器        │   复合 CSS 选择器引擎     │   HTMLRewriter       │
│  HtmlTokenizer           │  CssSelector.parse()    │  on(sel, handler)    │
│  StartTag / EndTag       │  tag#id.c1.c2[a=v]      │  HtmlElement API:    │
│  SelfClosing / Comment   │  [a^=v] [a$=v] [a*=v]   │  setTagName / attr   │
│  Doctype / CDATA         │  matchesToken()         │  before/after/prepend│
│  RawText (<script/style>)│  matchesAttributes()    │  append/replaceWith  │
├──────────────────────────┴─────────────────────────┴──────────────────────┤
│        HtmlEscape (快路径转义 + 86 个 HTML5 命名实体 + &#NNN; / &#xHHH;)     │
└───────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/html/index.cp` | `// Source: /data/dev/bun/src/lolhtml_sys/lol_html.cp` | `cap:html` 统一公开导出入口 |
| `lib/html/tokenizer.cp` | `// Source: /data/dev/bun/src/lolhtml_sys/lol_html.cp` | `HtmlTokenizer`、`HtmlToken`、`HtmlTokenKind`、`HtmlAttribute` HTML5 词法引擎 |
| `lib/html/selector.cp` | `// Source: /data/dev/bun/src/lolhtml_sys/lol_html.cp` | `CssSelector` 复合选择器、`HtmlElement` 变更句柄与 `HtmlRewriter` 流式重写器 |
| `lib/html/escape.cp` | `// Source: /data/dev/bun/src/md/entity.cp` | `HtmlEscape` 快路径转义与 HTML5 命名/数值实体编解码 |

---

## 二、 核心 API 详解

### 1. `HtmlTokenizer` 与 `HtmlToken` (HTML5 Tokenizer)

- `HtmlTokenKind`：`StartTag`、`EndTag`、`SelfClosingTag`、`Text`、`Comment`、`Doctype`、`Cdata`。
- 正确解析双引号 `attr="v"`、单引号 `attr='v'`、无引号 `attr=v` 及布尔属性 `disabled`；
- 对 `<script>`、`<style>`、`<textarea>`、`<title>` 进入 `RawText` 模式，确保内部 JS/CSS 代码中的 `<` / `>`（如 `if (a < b)`）不会被误拆分为 HTML 标签；
- 自动识别 HTML5 Void 元素（`br`、`img`、`input`、`meta`、`link`、`hr` 等）。

| API 签名 | 说明 |
| :--- | :--- |
| `HtmlTokenizer.init(html): HtmlTokenizer` | 创建流式 HTML5 分词器 |
| `tok.next(allocator): (HtmlToken?)!` | 读取下一个 `HtmlToken`（结束返回 `null`） |
| `HtmlTokenizer.tokenizeAll(allocator, html): ([]HtmlToken)!` | 一次性分词为完整 Token 切片 |
| `token.getAttribute(name)` / `hasAttribute(name)` | 大小写不敏感查询标签属性 |

### 2. `HtmlEscape` 实体编解码器

- 支持 `&amp;`、`&lt;`、`&gt;`、`&quot;`、`&apos;`、`&nbsp;`、`&copy;`、`&reg;`、`&trade;`、`&mdash;`、`&ndash;`、`&hellip;`、`&euro;`、希腊字母、箭头、数学符号等 **86 个高频 HTML5 命名实体**（二分查找）；
- 支持十进制（`&#65;`）与十六进制（`&#x1F600;` -> 4 字节 UTF-8 Emoji）数值实体解码，并按 HTML5 规范将非法代理项/越界码点替换为 `U+FFFD`。

| API 签名 | 说明 |
| :--- | :--- |
| `HtmlEscape.needsEscape(input): bool` | 快速检测是否包含需转义字符 |
| `HtmlEscape.escapeToBuf(input, out_buf)` / `escapeAlloc(alloc, input)` | HTML 特殊字符转义 |
| `HtmlEscape.unescapeToBuf(input, out_buf)` / `unescapeAlloc(alloc, input)` | 命名 + 十进制 + 十六进制实体解码 |

### 3. `CssSelector`、`HtmlElement` 与 `HtmlRewriter`

- `CssSelector.parse(sel)` 支持标签（`a`）、通配符（`*`）、ID（`#main`）、多 Class（`.btn.primary`）、复合组合（`div.card#hero`）与多属性匹配（`[href]`、`[type="text"]`、`[href^="https"]`、`[href$=".pdf"]`、`[href*="cp"]`）。
- `HtmlElement` 回调方法（在 `rw.on(selector, handler)` 中使用）：
  - 属性与标签：`tagName()`、`setTagName(name)`、`getAttribute(name)`、`hasAttribute(name)`、`setAttribute(name, val)`、`removeAttribute(name)`；
  - 内容与节点改写：`before(content, is_html)`、`after(content, is_html)`、`prepend(content, is_html)`、`append(content, is_html)`、`setInnerContent(content, is_html)`、`replaceWith(content, is_html)`（另有单参 `replaceHtml` / `replaceText`）、`remove()`、`removeAndKeepContent()`。
- 静态零堆便捷工具保留：`HtmlRewriter.stripTags(html, out_buf)` 与 `HtmlRewriter.renameTag(html, old_tag, new_tag, out_buf)`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { HtmlEscape, HtmlElement, HtmlRewriter } from "cap:html";

fn enhanceLink(el: *HtmlElement) {
    el.setAttribute("rel", "noopener noreferrer");
    el.setTagName("a");
    el.prepend("[Link] ", false);
}

pub fn demoHtmlRewriter(): !void {
    const allocator = std.heap.page_allocator;

    let rw = HtmlRewriter.init(allocator);
    defer rw.deinit();
    try rw.on("a.external[href^=\"https://\"]", enhanceLink);

    const out = try rw.transform("<p><a class=\"external\" href=\"https://cp-lang.org\">CP</a></p>");
    defer allocator.free(out);
    // => <p><a class="external" href="https://cp-lang.org" rel="noopener noreferrer">[Link] CP</a></p>

    let buf: [64]u8 = [0; 64];
    const emoji = HtmlEscape.unescapeToBuf("Hi &#x1F600; &amp; &copy;", &buf);
    * = emoji;
}
```
