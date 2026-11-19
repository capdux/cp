# CP 标准库：CommonMark / GFM Markdown 解析与渲染手册 (`cap:md`)

## 一、 架构总览与设计准则

`cap:md` 是 CP 语言官方内置的零堆分配 Markdown 解析与多目标渲染标准库，吸收自 Bun 生产级 Markdown 引擎（`/data/dev/bun/src/md/`），面向静态站点生成、文档管线、AI Agent 输出渲染与 CLI 终端富文本展示。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├───────────────────────────────────┬────────────────────────────────────┤
│     核心文档解析与渲染器           │       GFM 表格专用引擎             │
│  MdOptions (commonmark / github)  │  MdTableAlign (None/Left/Center/R) │
│  Markdown.renderHtml()            │  MdTable.parseAlignSpec()          │
│  Markdown.renderInlineHtml()      │  MdTable.splitRow()                │
│  Markdown.renderAnsi()            │  MdTable.parseDelimiterRow()       │
│                                   │  MdTable.renderHtml()              │
├───────────────────────────────────┴────────────────────────────────────┤
│              调用方预分配缓冲输出 (Zero-Heap []u8 Buffer)               │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/md/index.cp` | `// Source: /data/dev/bun/src/md/root.cp` | `cap:md` 统一公开导出入口 |
| `lib/md/markdown.cp` | `// Source: /data/dev/bun/src/md/root.cp` | `MdOptions` 配置预设、`Markdown` HTML/行内元素/终端 ANSI 渲染器 |
| `lib/md/table.cp` | `// Source: /data/dev/bun/src/md/blocks.cp` | `MdTableAlign` 对齐枚举与 `MdTable` GFM 表格解析与 `<table>` HTML 渲染器 |

### 2. 核心设计特性

1. **零堆分配缓冲写入（Caller-Owned Buffer）**：
   - 所有渲染函数（`Markdown.renderHtml`、`Markdown.renderAnsi`、`MdTable.renderHtml`）直接向调用方提供的 `[]u8` 缓冲区写入并返回精确有效切片 `out_buf[0..pos]`，全程不触发堆分配，天然适配高并发 Actor 工作负载。
2. **预设方言切换（CommonMark vs GitHub Flavored Markdown）**：
   - `MdOptions.commonmark()` 严格关闭扩展项，`MdOptions.github()` 一键启用 GFM 表格、删除线、任务列表、自动链接、标签过滤与标题 ID。
3. **双渲染后端（Web HTML + Terminal ANSI）**：
   - 同一套 Markdown 源码既可输出语义化 HTML5 标签（`<h1>`..`<h6>`、`<li>`、`<p>`、`<strong>`、`<code>`、`<table>`），也可直接输出带 ANSI 转义色彩控制码的终端文本。

---

## 二、 核心 API 详解

### 1. `MdOptions` 配置结构

| 字段 / 方法 | 类型 / 返回值 | 默认值 / 说明 |
| :--- | :--- | :--- |
| `tables` | `bool` | `true`，是否启用 GFM 表格 |
| `strikethrough` | `bool` | `true`，是否启用删除线 |
| `tasklists` | `bool` | `true`，是否启用任务列表 |
| `autolinks` | `bool` | `true`，是否启用自动链接 |
| `tag_filter` | `bool` | `false`，是否启用 GFM HTML 标签过滤 |
| `heading_ids` | `bool` | `false`，是否为标题生成锚点 ID |
| `MdOptions.commonmark()` | `MdOptions` | 严格 CommonMark 预设（全部扩展关闭） |
| `MdOptions.github()` | `MdOptions` | 完整 GFM 预设（全部扩展开启） |

### 2. `Markdown` 渲染引擎

| API 签名 | 说明 |
| :--- | :--- |
| `Markdown.renderHtml(md_text, out_buf): []const u8` | 将 ATX 标题（`#`..`######`）、无序列表（`- ` / `* `）、段落及行内样式渲染为 HTML |
| `Markdown.renderInlineHtml(line, out, start_pos): usize` | 渲染单行中的行内代码（`` `code` ``）与加粗（`**bold**`）到 `out`，返回最新写入偏移 |
| `Markdown.renderAnsi(md_text, out_buf): []const u8` | 将 Markdown 标题与正文渲染为终端 ANSI 高亮文本（一级标题高亮青色 `\x1b[1;36m`） |

### 3. `MdTableAlign` 与 `MdTable`

- `MdTableAlign` 枚举：`None = 0`、`Left = 1`、`Center = 2`、`Right = 3`。

| API 签名 | 说明 |
| :--- | :--- |
| `MdTable.parseAlignSpec(spec): MdTableAlign?` | 解析单个分隔单元格（`---`、`:---`、`:---:`、`---:`），非法格式返回 `null` |
| `MdTable.splitRow(line, out_cells): usize` | 按 `|` 切分表格行并去除首尾管道符与空白，返回单元格数量 |
| `MdTable.parseDelimiterRow(line, out_aligns): usize` | 解析整行对齐声明，全部合法时返回列数，否则返回 `0` |
| `MdTable.alignAttr(align_val): []const u8` | 将对齐枚举映射为 HTML 属性片段（如 ` align="center"`） |
| `MdTable.renderHtml(md_table, out_buf): []const u8` | 将多行 GFM 表格完整渲染为 `<table><thead>...</thead><tbody>...</tbody></table>`（最多支持 16 列） |

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { MdOptions, Markdown, MdTableAlign, MdTable } from "cap:md";

pub fn demoMarkdown() {
    // 1. 使用方言配置预设
    const gfm_opts = MdOptions.github();
    _ = gfm_opts;

    // 2. Markdown 渲染为 HTML
    const doc = "# Hello **CP**\n- Fast `zero-heap` parser\nNative **Markdown** renderer\n";
    let html_buf: [512]u8 = [0; 512];
    const html = Markdown.renderHtml(doc, html_buf[0..512]);
    // 结果:
    // <h1>Hello <strong>CP</strong></h1>
    // <li>Fast <code>zero-heap</code> parser</li>
    // <p>Native <strong>Markdown</strong> renderer</p>
    _ = html;

    // 3. Markdown 渲染为终端 ANSI 输出
    let ansi_buf: [256]u8 = [0; 256];
    const ansi = Markdown.renderAnsi("# CLI Header\nStatus: OK", ansi_buf[0..256]);
    _ = ansi;

    // 4. GFM 表格解析与 HTML <table> 渲染
    const md_table = "| Module | Mode | Align |\n| :--- | :---: | ---: |\n| cap:md | GFM | 100% |\n";
    let table_buf: [512]u8 = [0; 512];
    const table_html = MdTable.renderHtml(md_table, table_buf[0..512]);
    _ = table_html;
}
```

---

## 四、 与 Bun `src/md/` 对照表

| 功能分类 | Bun (`src/md/` & `Bun.markdown`) | CP 标准库 (`cap:md`) |
| :--- | :--- | :--- |
| **方言选项配置** | `src/md/types.cp` (`Options`) | `MdOptions` (`commonmark()` / `github()`) |
| **HTML 块级渲染** | `src/md/html_renderer.cp` / `Bun.markdown.html()` | `Markdown.renderHtml(md_text, out_buf)` |
| **行内元素渲染** | `src/md/inlines.cp` | `Markdown.renderInlineHtml(line, out, start_pos)` |
| **终端 ANSI 渲染** | `src/md/ansi_renderer.cp` / `Bun.markdown.render()` | `Markdown.renderAnsi(md_text, out_buf)` |
| **GFM 表格对齐解析** | `src/md/blocks.cp` (`TableAlign`) | `MdTableAlign` / `MdTable.parseAlignSpec()` / `parseDelimiterRow()` |
| **GFM 表格 HTML 输出** | `src/md/render_blocks.cp` | `MdTable.renderHtml(md_table, out_buf)` |
