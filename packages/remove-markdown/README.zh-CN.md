[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# remove-markdown

纯 CP 零堆分配 Markdown 转纯文本与 UTF-8 安全摘要截断引擎 —— 洁净室 MIT 实现，对标 npm `remove-markdown` 与 `strip-markdown`。

精准剥离 ATX/Setext 标题、水平分隔线、行内样式（`**加粗**`、`_斜体_`、`~~删除线~~`、`` `代码` ``）、链接/图片、代码块围栏、引用、列表/任务列表与 HTML 标签，同时智能保护 `snake_case` 变量下划线与 `a < b`、`2 * 3` 运算符，专为全文检索、摘要预览与 LLM 上下文瘦身打造。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `remove-markdown`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/stiang/remove-markdown` |
| 许可证 | MIT |

从 TypeScript/Node.js 迁移至 CP 时无需改名：

```ts
// TypeScript / Node.js (npm: remove-markdown)
import removeMd from "remove-markdown";
const plain = removeMd("# Hello **CP**\n\nVisit [CapDux](https://capdux.org)!");
```

```cp
// CP (cap: remove-markdown) — 零堆分配纯文本剥离与摘要生成
import { stripMarkdown } from "remove-markdown";

let buf: [1024]u8 = undefined;
const plain = stripMarkdown("# Hello **CP**\n\nVisit [CapDux](https://capdux.org)!", buf[0..1024]);
```

## 安装

```bash
cap add remove-markdown
```

## 使用示例

### 基础用法：多行纯文本、单行索引与 UTF-8 安全摘要

```cp
import { stripMarkdown, stripToSingleLine, excerpt } from "remove-markdown";

fn demoStrip() {
    let b1: [1024]u8 = undefined;
    let b2: [1024]u8 = undefined;
    let b3: [128]u8 = undefined;

    const plain = stripMarkdown("# Hello **CP**\n\nVisit [CapDux](https://capdux.org)!", b1[0..1024]);
    // => "Hello CP\n\nVisit CapDux!"

    // 转为紧凑单行文本（适合全文搜索索引或向量嵌入）
    const line = stripToSingleLine("## Title\n- [x] **Fast** & `zero-alloc`", b2[0..1024]);
    // => "Title Fast & zero-alloc"

    // UTF-8 边界安全截断（绝不截断多字节中文/Emoji 字符）
    const summary = excerpt("# Long Article\n\nThis is a **very long** markdown document.", 24 as usize, b3[0..128]);
    // => "Long Article This is..."
    _ = plain;
    _ = line;
    _ = summary;
}
```

### 边界用法：自定义 `StripOptions`、代码块/图片控制与变量下划线保护

```cp
import { StripOptions, stripMarkdownWith } from "remove-markdown";

fn demoOptions() {
    let buf: [1024]u8 = undefined;

    let opts = StripOptions.defaults();
    opts.keep_img_alt = false;          // 彻底剔除 ![alt](url)
    opts.keep_code_block_text = false;  // 剔除围栏代码块正文
    opts.strip_html = true;             // 剥离 <tag> HTML 标签
    opts.collapse_whitespace = true;    // 合并连续空白

    const md = "Check `my_var_name` and ![logo](img.png) <span>ok</span>\n```cp\nlet x = 1;\n```";
    const out = stripMarkdownWith(md, opts, buf[0..1024]);
    // 保留 my_var_name 下划线，同时移除图片 alt 与代码块正文
    _ = out;
}
```

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `StripOptions` | `class { keep_img_alt: bool, keep_code_block_text: bool, strip_html: bool, collapse_whitespace: bool }` | 剥离配置，内置 `StripOptions.defaults()` |
| `stripMarkdown` | `fn(md: string, out: []u8): string` | 使用默认配置将 Markdown 转为干净多行纯文本 |
| `stripMarkdownWith` | `fn(md: string, opts: StripOptions, out: []u8): string` | 使用自定义 `StripOptions` 转为纯文本 |
| `stripToSingleLine` | `fn(md: string, out: []u8): string` | 转为紧凑单行纯文本（自动折叠换行与空白） |
| `excerpt` | `fn(md: string, max_bytes: usize, out: []u8): string` | UTF-8 安全的单行摘要截断（超出时追加 `...`） |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`isSpace`、`isAlpha`、`isAlnum`、`isDigit`、`trim`、`findByte`）加速行扫描与括号/分隔符定位。
- **零堆分配无正则**：单遍线性扫描直接写入调用方 `out: []u8`，免疫正则回溯（ReDoS）风险。
- **`undefined` 零清零栈缓冲**：支持传入 `undefined` 声明的栈缓冲，所有写入严格受 `out.len` 保护。
- **UTF-8 边界与递归安全**：`excerpt` 保证不劈裂多字节 UTF-8 字符，行内嵌套解析内置深度保护。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `remove-markdown`。
