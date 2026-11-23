# `cap:markdown` — 纯 CP CommonMark 0.31.2 / GFM 行分析、链接解析与安全 HTML 渲染引擎

> 参考来源：`/data/dev/bun/src/md/{line_analysis,links,ref_defs}.cp`, [`pulldown-cmark/pulldown-cmark`](https://github.com/pulldown-cmark/pulldown-cmark) (MIT) | 加固日期：2026-11-23

## 概述

`cap:markdown` 是 CP 官方内置的纯原生 CommonMark 0.31.2 + GFM Markdown 解析与安全 HTML 渲染引擎，对照 Rust 工业级标杆仓 **`pulldown-cmark/pulldown-cmark` (MIT)** 完成边界加固，具备 **零外部依赖、零 `zig{}` 块、`cap:simd` + 16-lane `@Vector(16, u8)` SIMD 热路径加速、递归深度硬上限（`MD_MAX_NESTING_DEPTH = 64`）、线性时间防二次回溯 DoS 机制与安全 URL 过滤**。

## 子模块拓扑

| 文件 | 谱系与标杆注释 | 职责 |
| :--- | :--- | :--- |
| `lib/markdown/line.cp` | `// Reference-hardened against pulldown-cmark/pulldown-cmark (MIT)` | ATX/Setext 标题、分隔线、围栏代码块、HTML 块起止条件、GFM 表格分隔行分析（`cap:simd` 行尾加速） |
| `lib/markdown/links.cp` | `// Reference-hardened against pulldown-cmark/pulldown-cmark (MIT)` | CommonMark §6.3–6.5 内联链接/图片/自动链接解析、`percentEncodeUrl`、安全 URL 过滤 `isSafeUrl` 与括号深度上限防护 |
| `lib/markdown/refdefs.cp` | `// Source: Absorbed from /data/dev/bun/src/md/ref_defs.cp` | 链接引用定义解析、标签规范化（`normalizeLabel`/`labelsEqual`）与引用定义表 `RefDefTable` |
| `lib/markdown/renderer.cp` | `// Reference-hardened against pulldown-cmark/pulldown-cmark (MIT)` | CommonMark 0.31.2 + GFM 线性时间解析与安全 HTML 渲染引擎、`cap:simd` / 16-lane SIMD 行内触发与 HTML 转义原语 |

## 核心 API 摘要

- **安全 HTML 渲染与 SIMD 原语 (`renderer.cp`)**：
  - `MD_MAX_NESTING_DEPTH`（默认 64）、`RenderOptions`（`safe_urls` / `gfm_tables` / `gfm_tasklist` / `gfm_strikethrough` / `max_depth`）
  - `renderToHtml(allocator, md): ([]u8)!`、`renderToHtmlWithOptions(allocator, md, opts): ([]u8)!`
  - `simdFindLineEnd(text)`、`simdIndexOfInlineSpecial(text)`、`simdIndexOfHtmlEscape(text)`、`escapeHtmlSimd(out, allocator, text)`
- **行分析 (`line.cp`)**：`SetextResult`, `AtxHeaderResult`, `CodeFenceResult`, `TableAlign`, `TableUnderlineResult`, `isSetextUnderline`, `isHrLine`, `isAtxHeaderLine`, `isOpeningCodeFence`, `isClosingCodeFence`, `isHtmlBlockStartCondition`, `isHtmlBlockEndCondition`, `matchHtmlTag`, `isBlockLevelHtmlTag`, `isCompleteHtmlTag`, `isTableUnderline`
- **链接解析与安全过滤 (`links.cp`)**：`LinkKind`, `ParsedLink`, `parseAutolink`, `parseInlineLinkOrImage`, `percentEncodeUrl`, `isSafeUrl`
- **引用定义 (`refdefs.cp`)**：`RefDef`, `ParseRefDefResult`, `ParseDestResult`, `ParseTitleResult`, `normalizeLabel`, `labelsEqual`, `skipRefDefWhitespace`, `parseRefDefDest`, `parseRefDefTitle`, `parseRefDef`, `RefDefTable`

## 示例

```cp
const std = @import("std");
import { renderToHtml, isAtxHeaderLine, isSafeUrl } from "cap:markdown";

fn example(): !void {
    const html = try renderToHtml(std.heap.page_allocator, "# Hello\n\n- [x] **CP** ~~old~~ `code`");
    defer std.heap.page_allocator.free(html);
}
```

## CommonMark 0.31.2 / GFM 工业级安全边界

1. **块级严格边界与递归深度上限 (`line.cp` / `renderer.cp`)**：
   - 行首缩进 `> 3` 空格不触发 ATX 标题或围栏代码块；ATX `#` 后必须为空白或行尾；反引号围栏的 info string 禁止含 `` ` ``；闭合围栏必须同字符且长度 `>=` 开围栏。
   - 嵌套块引用（`> > > ...`）与行内嵌套严格受 `MD_MAX_NESTING_DEPTH = 64` 硬上限保护，超深嵌套安全降级为转义纯文本，杜绝栈溢出。
2. **线性时间防二次回溯 DoS (`renderer.cp` / `links.cp`)**：
   - 对未闭合反引号采用 64-bit 长度未命中位图缓存（`no_close_tick_mask`），对未闭合 `[`、`*`、`_`、`~~`、`<` 采用单次未命中闩锁与线性扫描预算（`scan_budget`），确保 2,000+ 字符病理嵌套在毫秒级线性完成。
3. **安全 URL 过滤与 HTML 转义 (`links.cp` / `renderer.cp`)**：
   - `isSafeUrl` 自动剥离混淆空白与控制字符，大小写不敏感拦截 `javascript:`、`vbscript:`、`data:text/html`、`data:application/xhtml`；`escapeHtmlSimd` 使用 `Simd.indexOfAny4('<', '>', '&', '"')` 批量安全转义。
