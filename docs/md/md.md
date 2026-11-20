# CP 标准库：MD4C 级工业 Markdown 解析与渲染手册 (`cap:md`)

## 一、 架构总览与设计准则

`cap:md` 是 CP 语言官方内置的高完整度 CommonMark + GitHub Flavored Markdown (GFM) 解析与多目标渲染标准库，直接原版移植自 Bun 生产级 MD4C 架构引擎（`/data/dev/bun/src/md/`，21 个文件 11,735 行），同时 100% 保留原有 `Markdown.parse` / `Markdown.toHtml` / `MdTable` 结构化 AST 与表格 API 的向后兼容性。

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│                          CP 用户层应用 (Application)                           │
├──────────────────────────────┬────────────────────────┬──────────────────────┤
│   MD4C 双遍流式引擎 (原版直迁) │   兼容 AST 与表格引擎   │   HTML5 实体与 Unicode │
│  Markdown.toHtmlEngine()     │  Markdown.parse()      │  2,125 W3C 命名实体   │
│  Markdown.toAnsi()           │  Markdown.toHtml()     │  lookupEntity()      │
│  renderWithRenderer()        │  MdTable / MdDocument  │  caseFold() (1/2/3码点)│
│  HtmlRenderer / AnsiRenderer │  MdRenderer 遍历接口    │  HeadingIdTracker    │
└──────────────────────────────┴────────────────────────┴──────────────────────┘
```

### 1. 模块文件拓扑与谱系来源（共 21 文件，11,735 行）

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/md/index.cp` | `// Source: /data/dev/bun/src/md/root.cp` | `cap:md` 统一公开导出入口 |
| `lib/md/types.cp` | `// Source: /data/dev/bun/src/md/types.cp` | MD4C `Flags`、`BlockType`、`SpanType`、`RendererVTable`、`Md4cParserState`、`BitSet256` 标记映射 |
| `lib/md/entity.cp` | `// Source: /data/dev/bun/src/md/entity.cp` | 完整 2,125 条 W3C HTML5 命名字符实体表与二分查找 `lookup()` |
| `lib/md/unicode.cp` | `// Source: /data/dev/bun/src/md/unicode.cp` | 完整 MD4C Unicode Case-Folding 映射表（1/2/3 码点 `fold_map_1/2/3`）与 `caseFold()` |
| `lib/md/helpers.cp` | `// Source: /data/dev/bun/src/md/helpers.cp` | 335 组 Unicode 标点范围、UTF-8 编解码、数字/命名实体转 UTF-8、`HeadingIdTracker` |
| `lib/md/autolinks.cp` | `// Source: /data/dev/bun/src/md/autolinks.cp` | GFM 宽松 URL/Email/www 自动链接扫描与括号/尾部标点配平裁剪 |
| `lib/md/line_analysis.cp` | `// Source: /data/dev/bun/src/md/line_analysis.cp` | ATX/Setext 标题、围栏/缩进代码块、HR、HTML 块（1-7 类）、GFM 表格下划线与容器行分析 |
| `lib/md/ref_defs.cp` | `// Source: /data/dev/bun/src/md/ref_defs.cp` | CommonMark 链接引用定义解析、Unicode 归一化与哈希查找 `lookupRefDef()` |
| `lib/md/inline_core.cp` | `// Source: /data/dev/bun/src/md/inlines.cp` | 内联 Span 调度、反引号代码跨度、左右侧翼规则、HTML 标签扫描 |
| `lib/md/links.cp` | `// Source: /data/dev/bun/src/md/links.cp` | 行内链接、引用/简写链接、图片、Wiki 链接与尖括号自动链接解析 |
| `lib/md/inlines.cp` | `// Source: /data/dev/bun/src/md/inlines.cp` | CommonMark $O(N)$ 强调分隔符栈匹配算法、删除线与全行内元素分发 |
| `lib/md/render_blocks.cp` | `// Source: /data/dev/bun/src/md/render_blocks.cp` | 代码块、HTML 块、GFM 表格行/单元格渲染分发 |
| `lib/md/containers.cp` | `// Source: /data/dev/bun/src/md/containers.cp` | 嵌套容器栈、紧缩/松散列表回溯修补与块字节流遍历循环 |
| `lib/md/blocks.cp` | `// Source: /data/dev/bun/src/md/blocks.cp` | MD4C 双遍块级解析状态机（`processDoc` / `analyzeLine` / `processLine`） |
| `lib/md/html_renderer.cp` | `// Source: /data/dev/bun/src/md/html_renderer.cp` | MD4C `HtmlRenderer`（标题锚点、自链接标题、GFM 标签过滤、`sanitize_html` XSS 防护） |
| `lib/md/ansi_renderer.cp` | `// Source: /data/dev/bun/src/md/ansi_renderer.cp` | 终端 `AnsiRenderer`（自动折行、东亚宽字符、Unicode 方框表格、OSC 8 超链接、Kitty 图片、`ImageUrlCollector`） |
| `lib/md/engine.cp` | `// Source: /data/dev/bun/src/md/root.cp` | `MdEngineOptions`、`renderToHtml`、`renderToHtmlWithOptions`、`renderWithRenderer`、`renderToAnsi` |
| `lib/md/markdown.cp` | `// Source: /data/dev/bun/src/md/root.cp` | `Markdown` 统一门面（含 `toHtmlEngine` / `toAnsi` / `lookupEntity` / `caseFold` 与兼容 API） |
| `lib/md/ast.cp` | `// Source: /data/dev/bun/src/md/types.cp` | 向后兼容：`MarkdownOptions`、`MdBlockKind`、`MdBlock`、`MdLinkDef`、`MdRenderer` |
| `lib/md/parser.cp` | `// Source: /data/dev/bun/src/md/parser.cp` | 向后兼容：`MdDocument` 与 `MdParser` |
| `lib/md/table.cp` | `// Source: /data/dev/bun/src/md/blocks.cp` | 向后兼容：`MdTableAlign` 与 `MdTable` |

---

## 二、 核心 API 详解

### 1. MD4C 工业流式引擎 (`MdEngineOptions` / `Markdown.toHtmlEngine` / `Markdown.toAnsi`)

| API / 选项 | 说明 |
| :--- | :--- |
| `MdEngineOptions` | 支持 `tables`、`strikethrough`、`tasklists`、`permissive_url_autolinks`、`permissive_email_autolinks`、`permissive_www_autolinks`、`heading_ids`、`autolink_headings`、`tag_filter`、`sanitize_html`、`wiki_links`、`latex_math`、`no_indented_code_blocks`、`hard_soft_breaks`、`underline` 等全套选项 |
| `Markdown.toHtmlEngine(allocator, md, opts): ([]u8)!` | 使用 MD4C 双遍引擎解析并渲染高质量 CommonMark + GFM HTML |
| `Markdown.toAnsi(allocator, md, opts, theme): ([]u8)!` | 使用 `AnsiTheme` 渲染终端 ANSI 彩色输出（支持自动折行、Unicode 表格边框、代码高亮、OSC 8 超链接） |
| `renderWithRenderer(md, allocator, opts, rend): !` | 使用自定义 `Renderer`（`RendererVTable`）流式接收块与行内事件（如 `ImageUrlCollector` 提取图片地址） |
| `Markdown.lookupEntity(name): ([2]u21)?` | 查找 2,125 个 W3C HTML5 命名实体（如 `&copy;`、`&mdash;`、`&AElig;`） |
| `Markdown.caseFold(codepoint): FoldInfo` | CommonMark 标准 Unicode 大小写折叠（如 `0x00DF` -> `"ss"`） |

### 2. 向后兼容 AST 与表格 API (`Markdown.parse` / `Markdown.toHtml` / `MdTable`)

保留全部既有兼容接口：`MarkdownOptions.commonmark()` / `.github()`、`Markdown.parse`、`Markdown.toHtml`、`Markdown.render`、`Markdown.renderWithRenderer`、`Markdown.renderHtml`、`Markdown.renderAnsi`、`MdTable.parseAlignSpec` / `splitRow` / `renderHtml`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    Markdown,
    MdEngineOptions,
    AnsiTheme,
    ImageUrlCollector,
    renderWithRenderer,
} from "cap:md";

pub fn demoMd4c(): ! {
    const allocator = std.heap.page_allocator;
    const src = "# Guide &copy;\n\n- [x] **Fast** ~~old~~ [Ref][r]\n\n[r]: https://example.com\n";

    // 1. MD4C HTML 渲染
    const opts = MdEngineOptions{
        heading_ids: true,
        autolink_headings: true,
        tag_filter: true,
        sanitize_html: true,
    };
    const html = Markdown.toHtmlEngine(allocator, src, opts)?;
    defer allocator.free(html);

    // 2. MD4C ANSI 终端渲染
    const ansi = Markdown.toAnsi(allocator, src, opts, AnsiTheme{ columns: 80, colors: true })?;
    defer allocator.free(ansi);

    // 3. 流式事件访问者收集图片 URL
    let collector = ImageUrlCollector.init(allocator);
    defer collector.deinit();
    renderWithRenderer("![logo](https://example.com/a.png)", allocator, opts, collector.renderer())?;
}
```
