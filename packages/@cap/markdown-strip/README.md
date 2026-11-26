# @cap/markdown-strip

零堆分配的 CP Markdown 转纯文本与摘要截断引擎 — 洁净室 MIT 实现，对标 npm `remove-markdown` / `strip-markdown`，专为全文搜索索引、摘要生成与 LLM 上下文瘦身打造。

Zero-allocation Markdown to plain-text stripper and UTF-8 safe excerpt generator for CP — clean-room MIT parity with npm `remove-markdown` and `strip-markdown`.

## 特性 / Features

- **标题与分隔线清理 / Headings & Rules**：剥离 ATX 标题（`#`..`######`）、Setext 下划线（`===` / `---`）与水平分隔线（`---` / `***` / `___`）。
- **行内样式剥离 / Inline Formatting**：去除 `**bold**`、`__bold__`、`*italic*`、`_italic_`、`~~strike~~`、`` `code` ``，同时智能保护 `snake_case` 变量下划线与 `a < b`、`2 * 3` 运算符。
- **链接、图片与代码块 / Links, Images & Fences**：支持 `[text](url "title")`、`[text][ref]`、`![alt](url)`（可配置 `keep_img_alt`）、自动跳过 `[ref]: url` 定义行，剥离 ```` ``` ```` / `~~~` 围栏行（可配置 `keep_code_block_text`）。
- **引用、列表与 HTML / Quotes, Lists & HTML**：剥离 `>` 引用、`-`/`*`/`+`/`1.`/`2)` 列表、`- [x]` 任务列表及 `<tag>` HTML 标签。
- **单行索引与安全摘要 / Single-line & UTF-8 Safe Excerpt**：提供 `stripToSingleLine` 与不截断多字节 UTF-8 字符的 `excerpt(md, max_bytes, out)`。

## 安装 / Install

```bash
cap add @cap/markdown-strip
```

## API 参考 / API Reference

| 符号 / Symbol | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `StripOptions` | `{ keep_img_alt, keep_code_block_text, strip_html, collapse_whitespace }` | 剥离选项，内置 `StripOptions.defaults()` |
| `stripMarkdown` | `(md: string, out: []u8) -> string` | 默认选项转干净多行纯文本 |
| `stripMarkdownWith` | `(md: string, opts: StripOptions, out: []u8) -> string` | 自定义选项转纯文本 |
| `stripToSingleLine` | `(md: string, out: []u8) -> string` | 转为紧凑单行纯文本（适合搜索索引） |
| `excerpt` | `(md: string, max_bytes: usize, out: []u8) -> string` | UTF-8 安全的单行摘要截断（超出追加 `...`） |

## 使用示例 / Usage

```cp
import { stripMarkdown, stripToSingleLine, excerpt } from "@cap/markdown-strip";

let buf: [1024]u8 = undefined;
const plain = stripMarkdown("# Hello **CP**\n\nVisit [CapDux](https://capdux.org)!", buf[0..1024]);
// => "Hello CP\n\nVisit CapDux!"

const line = stripToSingleLine("## Title\n- [x] **Fast** & `zero-alloc`", buf[0..1024]);
// => "Title Fast & zero-alloc"

const summary = excerpt("# Long Article\n\nThis is a **very long** markdown document.", 24 as usize, buf[0..1024]);
// => "Long Article This is..."
```

## License

MIT
