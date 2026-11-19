# CP 标准库：HTML 实体编解码、CSS 选择器与流式重写器手册 (`cap:html`)

## 一、 架构总览与设计准则

`cap:html` 是 CP 语言官方内置的 HTML 文本处理与流式重写标准库，吸收自 Bun 的 HTMLRewriter 绑定（`/data/dev/bun/src/lolhtml_sys/lol_html.cp`）与实体转义子系统（`/data/dev/bun/src/md/entity.cp`），为 Web 服务端渲染（SSR）、模板内容防注入过滤、网页爬虫文本抽取与边缘节点 HTML 改写提供零堆分配原语。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├───────────────────────────────────┬────────────────────────────────────┤
│     HTML 实体转义与还原            │   CSS 选择器解析与流式标签重写      │
│  HtmlEscape.needsEscape()         │  CssSelector.parse()               │
│  HtmlEscape.escapeToBuf()         │  CssSelector.matchesTag()          │
│  HtmlEscape.unescapeToBuf()       │  HtmlRewriter.stripTags()          │
│                                   │  HtmlRewriter.renameTag()          │
├───────────────────────────────────┴────────────────────────────────────┤
│      零拷贝快路径扫描 (Zero-Copy Fast Path) + 栈缓冲输出 ([]u8)         │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/html/index.cp` | `// Source: /data/dev/bun/src/lolhtml_sys/lol_html.cp` | `cap:html` 统一公开导出入口 |
| `lib/html/selector.cp` | `// Source: /data/dev/bun/src/lolhtml_sys/lol_html.cp` | `CssSelector` 复合选择器解析器与 `HtmlRewriter` 流式标签剥离/重命名器 |
| `lib/html/escape.cp` | `// Source: /data/dev/bun/src/md/entity.cp` | `HtmlEscape` 零拷贝快路径 HTML 实体转义与反转义器 |

### 2. 核心设计特性

1. **零拷贝快路径转义（Fast-Path Escape）**：
   - `HtmlEscape.escapeToBuf` 先执行极速单遍扫描 `needsEscape`；当输入不含 `&`、`<`、`>`、`"`、`'` 时跳过逐字符实体替换分支，直接拷贝或返回原切片。
2. **标准 HTML5 核心实体全覆盖**：
   - 支持 `&amp;`、`&lt;`、`&gt;`、`&quot;`、`&#39;` 以及 XML 兼容实体 `&apos;` 的双向转义与还原，有效防护模板注入与畸形标签闭合问题。
3. **复合 CSS 选择器解析与流式 HTML 改写**：
   - `CssSelector.parse` 支持标签（`div`）、通配符（`*`）、ID（`a#main`）、类（`span.highlight`）与属性表达式（`input.field[type="email"]`）的组合拆解；`HtmlRewriter` 提供单遍流式 `stripTags` 与 `renameTag`，无需构建整棵 DOM 树。

---

## 二、 核心 API 详解

### 1. `HtmlEscape` 实体编解码器

| API 签名 | 说明 |
| :--- | :--- |
| `HtmlEscape.needsEscape(input): bool` | 检测输入切片是否包含任意需转义字符（`&`、`<`、`>`、`"`、`'`） |
| `HtmlEscape.escapeToBuf(input, out_buf): []const u8` | 将特殊字符转义为标准 HTML 实体写入 `out_buf` 并返回有效切片 |
| `HtmlEscape.unescapeToBuf(input, out_buf): []const u8` | 将 `&amp;`、`&lt;`、`&gt;`、`&quot;`、`&#39;`、`&apos;` 还原为原始字符 |

### 2. `CssSelector` 选择器结构

字段：`tag_name: []const u8`、`id_name: []const u8`、`class_name: []const u8`、`attr_name: []const u8`、`attr_val: []const u8`。

| API 签名 | 说明 |
| :--- | :--- |
| `CssSelector.parse(sel): CssSelector` | 解析简单或复合 CSS 选择器（支持 `#id`、`.class`、`[attr]`、`[attr="val"]`） |
| `selector.matchesTag(tag): bool` | 判断给定标签名是否与当前选择器匹配（空标签或 `*` 匹配任意标签） |

### 3. `HtmlRewriter` 流式重写器

| API 签名 | 说明 |
| :--- | :--- |
| `HtmlRewriter.stripTags(html, out_buf): []const u8` | 流式剥离所有 `<...>` HTML 标签，提取纯文本内容到 `out_buf` |
| `HtmlRewriter.renameTag(html, old_tag, new_tag, out_buf): []const u8` | 流式将匹配的 `<old_tag ...>` 与 `</old_tag>` 重命名为 `<new_tag ...>` 与 `</new_tag>`（保留原属性） |

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { HtmlEscape, CssSelector, HtmlRewriter } from "cap:html";

pub fn demoHtml() {
    // 1. HTML 实体转义与还原
    const unsafe_html = "<div class=\"card\">Tom & 'Jerry'</div>";
    let esc_buf: [256]u8 = [0; 256];
    const escaped = HtmlEscape.escapeToBuf(unsafe_html, esc_buf[0..256]);
    // => "&lt;div class=&quot;card&quot;&gt;Tom &amp; &#39;Jerry&#39;&lt;/div&gt;"

    let unesc_buf: [256]u8 = [0; 256];
    const restored = HtmlRewriter.stripTags(
        HtmlEscape.unescapeToBuf(escaped, unesc_buf[0..256]),
        esc_buf[0..256],
    );
    // => "Tom & 'Jerry'"
    _ = restored;

    // 2. 复合 CSS 选择器解析与标签匹配
    const sel = CssSelector.parse("input.search[type=\"text\"]");
    const is_input = sel.matchesTag("input"); // true
    // sel.class_name == "search", sel.attr_name == "type", sel.attr_val == "text"
    _ = is_input;

    // 3. 流式标签重命名（保留属性）
    let out_buf: [256]u8 = [0; 256];
    const rewritten = HtmlRewriter.renameTag(
        "<div id=\"app\">Hello CP</div>",
        "div",
        "section",
        out_buf[0..256],
    );
    // => "<section id=\"app\">Hello CP</section>"
    _ = rewritten;
}
```

---

## 四、 与 Bun `src/lolhtml_sys/` & `Bun.escapeHTML` 对照表

| 功能分类 | Bun (`src/lolhtml_sys/` & `src/md/entity.cp`) | CP 标准库 (`cap:html`) |
| :--- | :--- | :--- |
| **HTML 实体转义** | `Bun.escapeHTML(str)` / `src/md/entity.cp` | `HtmlEscape.escapeToBuf(input, out_buf)` |
| **转义快路径检测** | 内部 SIMD 特殊字符探测 | `HtmlEscape.needsEscape(input)` |
| **HTML 实体还原** | `src/md/entity.cp` | `HtmlEscape.unescapeToBuf(input, out_buf)` |
| **CSS 选择器解析** | `lol_html_selector_parse` (`src/lolhtml_sys/lol_html.cp`) | `CssSelector.parse(sel)` / `matchesTag(tag)` |
| **HTML 标签重命名** | `new HTMLRewriter().on(sel, { element(e) { e.tagName = ... } })` | `HtmlRewriter.renameTag(html, old_tag, new_tag, out_buf)` |
| **HTML 纯文本抽取** | `HTMLRewriter` 文本节点收集 | `HtmlRewriter.stripTags(html, out_buf)` |
