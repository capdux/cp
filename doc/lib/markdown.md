# `cap:markdown` — 纯 CP CommonMark / GFM 行分析、内联链接与引用定义解析库

## 概述

`cap:markdown` 吸收自 Bun `src/md/` 核心模块（`line_analysis.cp`、`links.cp`、`ref_defs.cp`），纯 CP 零外部依赖，提供 CommonMark / GFM 块级行分析、内联链接/图片/自动链接解析与 URL 百分号编码，以及链接引用定义表（`RefDefTable`）的完整实现。

## 子模块拓扑

| 文件 | 职责 |
| :--- | :--- |
| `lib/markdown/line.cp` | ATX/Setext 标题、分隔线、围栏代码块、HTML 块起止条件、GFM 表格分隔行分析 |
| `lib/markdown/links.cp` | CommonMark §6.3–6.5 内联链接、图片、自动链接解析与 `percentEncodeUrl` |
| `lib/markdown/refdefs.cp` | 链接引用定义解析、标签规范化（`normalizeLabel`/`labelsEqual`）与引用定义表 `RefDefTable` |

## 核心 API 摘要

- **行分析 (`line.cp`)**：`SetextResult`, `AtxHeaderResult`, `CodeFenceResult`, `TableAlign`, `TableUnderlineResult`, `isSetextUnderline`, `isHrLine`, `isAtxHeaderLine`, `isOpeningCodeFence`, `isClosingCodeFence`, `isHtmlBlockStartCondition`, `isHtmlBlockEndCondition`, `matchHtmlTag`, `isBlockLevelHtmlTag`, `isCompleteHtmlTag`, `isTableUnderline`
- **链接解析 (`links.cp`)**：`LinkKind`, `ParsedLink`, `parseAutolink`, `parseInlineLinkOrImage`, `percentEncodeUrl`
- **引用定义 (`refdefs.cp`)**：`RefDef`, `ParseRefDefResult`, `ParseDestResult`, `ParseTitleResult`, `normalizeLabel`, `labelsEqual`, `skipRefDefWhitespace`, `parseRefDefDest`, `parseRefDefTitle`, `parseRefDef`, `RefDefTable`

## 示例

```cp
import { isAtxHeaderLine, parseInlineLinkOrImage, parseRefDef } from "cap:markdown";

fn example(): bool {
    if (hdr := isAtxHeaderLine("## Hello")) {
        return hdr.level == 2;
    }
    return false;
}
```
