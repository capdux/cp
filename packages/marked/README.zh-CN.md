[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `marked`

面向 CP 语言的零堆分配 CommonMark / GFM Markdown 转 HTML 编译器与纯文本提取器——完整对标 **npm `marked` v14**，内置基于 `escape-html` 的安全 HTML 转义与链接协议校验。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `npm:marked` ([markedjs/marked](https://github.com/markedjs/marked), v14 对齐) |
| **CP 包名与版本** | `marked@1.0.0` (`MIT`) |
| **依赖** | `escape-html@1.0.0` |

保持原包名导入，零改名成本直接迁移：

```ts
// TypeScript (npm marked)
import { marked } from "marked";
const html = marked.parse("# Hello **CP**");
```

```cp
// CP — 保持原包名，零堆分配调用方缓冲
import { parseMarkdown } from "marked";

let out: [512]u8 = undefined;
const n = parseMarkdown("# Hello **CP**\n", out[0..]);
// out[0..n] == "<h1>Hello <strong>CP</strong></h1>\n"
```

## 架构特性与零堆设计

- **零动态内存分配**：单遍块级与行内递归编译器直接写入调用方 `out: []u8` 缓冲区，内置递归深度上限保护与编译期 256 项 ASCII 查找表（`MKD_PUNCT_ESCAPE_LUT`、`MKD_INLINE_SPECIAL_LUT`）。
- **默认安全转义**：通过 `escape-html` 转义 HTML 特殊字符，并在 `sanitize_links: true`（默认开启）时自动拦截不安全链接协议（`javascript:`、`vbscript:`、`data:`）。
- **完整 CommonMark + GFM 子集**：支持 ATX 标题（`#`..`######`）、围栏代码块与语言标识、引用块、有序/无序列表、分隔线、斜体/粗体、GFM 删除线（`~~`）、行内代码、链接/图片及纯文本剥离。

## 实战示例

### 1. 基础 Markdown 转 HTML（`parseMarkdown` 与 `parseInlineMarkdown`）

```cp
import { parseMarkdown, parseInlineMarkdown } from "marked";

fn demoBasic() {
    const md = "# Title\n\nHello **bold**, *italic*, ~~old~~, and `code`.\n\n- item 1\n- item 2\n";
    let html_buf: [1024]u8 = undefined;
    const n = parseMarkdown(md, html_buf[0..]);

    // 仅渲染行内元素（不包裹外层 <p>...</p>）
    let inline_buf: [256]u8 = undefined;
    const in_len = parseInlineMarkdown("Visit [CP](https://cp-lang.org)", inline_buf[0..]);
    _ = n; _ = in_len;
}
```

### 2. 自定义选项（`MarkedOptions`：`gfm`、`sanitize_links`、`breaks`）

```cp
import { MarkedOptions, defaultMarkedOptions, parseMarkdownWithOptions } from "marked";

fn demoOptions() {
    let opts = defaultMarkedOptions();
    opts.breaks = true;         // 单个换行符渲染为 <br>
    opts.gfm = true;            // 启用 GFM 删除线 ~~text~~
    opts.sanitize_links = true; // 过滤不安全链接协议

    let out: [512]u8 = undefined;
    const n = parseMarkdownWithOptions("line1\nline2\n\n[link](javascript:void(0))", opts, out[0..]);
    _ = n;
}
```

### 3. 纯文本提取（`stripMarkdown`）与缓冲区容量保护

```cp
import { stripMarkdown, parseMarkdown } from "marked";

fn demoStripAndSizing() {
    const doc = "## Release Notes\n\nSee **[docs](https://cp-lang.org)** and `cap build`.\n";

    // 提取无格式纯文本（适用于全文检索索引、摘要与 AI Agent 上下文压缩）
    let text_buf: [512]u8 = undefined;
    const tn = stripMarkdown(doc, text_buf[0..]);

    // 建议为 HTML 输出预留约 2x–4x 输入长度；
    // 即使缓冲区偏小也会安全截断到 out.len，绝不越界
    let small_buf: [32]u8 = undefined;
    const clamped = parseMarkdown(doc, small_buf[0..]);
    _ = tn; _ = clamped;
}
```

## 核心 API 速览

- `MarkedOptions`（`gfm: bool`、`sanitize_links: bool`、`breaks: bool`）
- `defaultMarkedOptions(): MarkedOptions`（默认 `gfm: true, sanitize_links: true, breaks: false`）
- `parseMarkdown(md: string, out: []u8): usize`
- `parseMarkdownWithOptions(md: string, opts: MarkedOptions, out: []u8): usize`
- `parseInlineMarkdown(src: string, out: []u8): usize`
- `stripMarkdown(md: string, out: []u8): usize`

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
