# `cap:markdown` — 纯 CP CommonMark / GFM 行分析、内联链接与引用定义解析库

> 参考来源：/data/dev/bun/src/md/{line_analysis,links,ref_defs}.cp | 吸收日期：2026-11-22

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

## 上游行为契约与 CommonMark/GFM 解析边界

1. **CommonMark 围栏与 ATX 标题严格边界 (`line.cp`)**：
   - 遵循 CommonMark 规则：行首缩进超过 3 个空格不触发 ATX 标题或围栏代码块；反引号围栏（```` ``` ````）的 info string 中禁止包含反引号，闭合围栏长度必须 `>=` 开围栏长度且仅含尾随空白。
2. **链接安全编码与括号平衡 (`links.cp`)**：
   - `parseInlineLinkOrImage` 支持无尖括号目标中的平衡嵌套圆括号（如 Wikipedia 链接）及尖括号 `<...>` 含空格目标；`percentEncodeUrl` 对非法/空白字符执行百分号编码并保留既有合法 `%XX` 序列防双重编码。
3. **引用标签规范化 (`refdefs.cp`)**：
   - `normalizeLabel` 与 `labelsEqual` 折叠连续空白并执行大小写无关比较，同一文档出现重复引用定义时遵循 CommonMark「首次定义优先」契约。
