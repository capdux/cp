# CP 标准库：CommonMark / GFM Markdown 解析与渲染手册 (`cap:md`)

## 一、 架构总览与设计准则

`cap:md` 是 CP 语言官方内置的 CommonMark + GitHub Flavored Markdown (GFM) 完整解析与多目标渲染标准库，对标并吸收自 Bun 生产级 Markdown 引擎（`/data/dev/bun/src/md/`），提供结构化块级 AST、可定制渲染遍历接口、XSS 安全转义及零堆分配快路径接口。

```text
┌───────────────────────────────────────────────────────────────────────────┐
│                         CP 用户层应用 (Application)                        │
├─────────────────────────────┬──────────────────────┬──────────────────────┤
│   完整 CommonMark + GFM AST  │  GFM 表格引擎         │  零堆快路径与 ANSI    │
│  Markdown.parse()           │  MdTableAlign        │  Markdown.renderHtml │
│  Markdown.toHtml()          │  MdTable.splitRow()  │  Markdown.renderAnsi │
│  Markdown.render()          │  MdTable.renderHtml  │  renderInlineHtml()  │
│  Markdown.renderWithRenderer│  (支持 \| 与 code)    │                      │
├─────────────────────────────┴──────────────────────┴──────────────────────┤
│   MarkdownOptions (gfm/tables/strikethrough/tasklists/autolinks/sanitize) │
└───────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/md/index.cp` | `// Source: /data/dev/bun/src/md/root.cp` | `cap:md` 统一公开导出入口 |
| `lib/md/ast.cp` | `// Source: /data/dev/bun/src/md/types.cp` | `MarkdownOptions`、`MdBlockKind`、`MdBlock`、`MdLinkDef`、`MdRenderer` |
| `lib/md/parser.cp` | `// Source: /data/dev/bun/src/md/parser.cp` | `MdDocument` 与 `MdParser`（完整块级/行内 CommonMark + GFM 解析与 HTML 渲染） |
| `lib/md/markdown.cp` | `// Source: /data/dev/bun/src/md/root.cp` | `MdOptions` 兼容预设、`Markdown.parse` / `toHtml` / `render` / `renderHtml` / `renderAnsi` |
| `lib/md/table.cp` | `// Source: /data/dev/bun/src/md/blocks.cp` | `MdTableAlign` 对齐枚举与 `MdTable` GFM 表格解析与 `<table>` 渲染器 |

### 2. 支持的 CommonMark & GFM 完整语法矩阵

- **块级元素**：
  - ATX 标题（`#` ~ `######`，支持尾部闭合 `#` 序列裁剪）与 Setext 标题（`===` / `---`）；
  - 围栏代码块（`` ``` `` 与 `~~~`，支持语言 info string 提取及多反引号长度匹配）与缩进代码块（4 空格 / Tab）；
  - 水平分割线（`---`、`***`、`___`）；
  - 嵌套引用块（`>` / `> >`，支持惰性续行与内嵌块）；
  - 有序列表（保留 `start="N"` 起始编号，支持 `.` / `)`）与无序列表（`-` / `*` / `+`，支持多级缩进嵌套与紧缩/松散判定）；
  - GFM 表格（表头、`left`/`center`/`right` 对齐、短行补齐、单元格内行内样式与 `\|` 转义）；
  - GFM 任务列表（`- [x]` / `- [ ]` 输出 `task-list-item` 与禁用复选框）；
  - 链接引用定义（`[ref]: url "title"`）与 HTML 块。
- **行内元素与安全防护**：
  - 粗体 `**`/`__`、斜体 `*`/`_`（遵循 CommonMark 词内 `_` 不触发规则如 `foo_bar_baz`）、删除线 `~~`、硬/软换行；
  - 单/多反引号行内代码（`` `code` ``、```` ``a ` b`` ````）；
  - 行内链接 `[text](url "title")`、引用链接 `[text][ref]`、图片 `![alt](src "title")`；
  - 尖括号自动链接 `<https://...>` / `<user@example.com>` 与 GFM 宽松 URL 自动链接 `https://...` / `www....`；
  - 反斜杠转义（`\*`、`\\` 等）与 HTML 安全转义（防 XSS：转义 `&<>""`，`sanitizeHtml` 启用时转义原始 HTML 标签并拦截 `javascript:`/`vbscript:`/`data:text/html` 协议）。

---

## 二、 核心 API 详解

### 1. `MarkdownOptions` 与 `MdOptions`

| 字段 / 预设 | 类型 / 返回值 | 默认值 / 说明 |
| :--- | :--- | :--- |
| `gfm` | `bool` | `true`，是否启用 GFM 模式 |
| `tables` | `bool` | `true`，启用 GFM 表格 |
| `strikethrough` | `bool` | `true`，启用 `~~删除线~~` |
| `tasklists` | `bool` | `true`，启用 `- [x]` / `- [ ]` 任务列表 |
| `autolinks` | `bool` | `true`，启用 `https://` / `www.` 自动链接 |
| `sanitizeHtml` | `bool` | `false`（`github()` 为 `true`），转义原始 HTML 标签并拦截危险 URL 协议 |
| `headingIds` | `bool` | `false`（`github()` 为 `true`），自动生成去重标题锚点 `id="..."` |
| `MarkdownOptions.commonmark()` | `MarkdownOptions` | 严格 CommonMark 预设（全部 GFM 扩展关闭） |
| `MarkdownOptions.github()` | `MarkdownOptions` | 完整 GitHub Flavored Markdown 预设 |

### 2. `Markdown`、`MdDocument` 与 `MdRenderer`

| API 签名 | 说明 |
| :--- | :--- |
| `Markdown.parse(allocator, md_text, opts): MdDocument!` | 解析 Markdown 为结构化 `MdDocument` AST（需调用 `doc.deinit()` 释放） |
| `Markdown.toHtml(allocator, md_text, opts): ([]u8)!` | 一键解析并渲染完整 CommonMark / GFM HTML |
| `Markdown.render(allocator, md_text, opts): ([]u8)!` | `toHtml` 的便捷别名 |
| `Markdown.renderWithRenderer(allocator, md_text, opts, renderer): !` | 使用自定义 `MdRenderer`（`enterBlock` / `leaveBlock` / `onText`）遍历 AST |
| `doc.blocks(): []MdBlock` | 获取顶层块节点切片进行只读 AST 检查 |
| `doc.walk(renderer)` | 深度优先遍历整棵文档 AST |
| `Markdown.renderHtml(md_text, out_buf): []const u8` | 零堆快路径渲染简单标题/列表/段落到预分配缓冲区 |
| `Markdown.renderAnsi(md_text, out_buf): []const u8` | 渲染终端 ANSI 彩色输出 |

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { MarkdownOptions, MdBlock, MdRenderer, Markdown } from "cap:md";

pub fn demoMarkdown(): !void {
    const allocator = std.heap.page_allocator;
    const opts = MarkdownOptions.github();

    const doc_src =
        "# CP Guide\n\n" ++
        "> Nested\n> > Blockquote\n\n" ++
        "- [x] Complete `CommonMark`\n- [ ] Check ~~old~~ **new**\n\n" ++
        "| Feature | Status |\n| :--- | ---: |\n| Tables | **OK** |\n";

    // 1. 完整 CommonMark + GFM HTML 渲染（含 headingIds 与 XSS 过滤）
    const html = try Markdown.toHtml(allocator, doc_src, opts);
    defer allocator.free(html);

    // 2. 结构化 AST 检查
    let doc = try Markdown.parse(allocator, doc_src, opts);
    defer doc.deinit();
    const top_blocks = doc.blocks();
    * = top_blocks;
}
```
