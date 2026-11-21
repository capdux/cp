# `cap:md_line_analysis`

纯 CP 零堆分配 CommonMark / GFM 块级行分析器，吸收自 `/data/dev/bun/src/md/line_analysis.cp`。

## 导出函数与类型

- `isSetextUnderline(text, off)` -> `SetextResult`（识别 `===` H1 / `---` H2）
- `isHrLine(text, off)` -> `bool`（识别 `---` / `***` / `___` 水平分割线）
- `isAtxHeaderLine(text, off, permissive)` -> `AtxHeaderResult`（识别 `#..######` 标题与剥离可选尾 `#` 序列）
- `isOpeningCodeFence(text, off)` / `isClosingCodeFence(text, off, fence_char, min_count)`（识别 ```` ``` ```` 与 `~~~` 围栏代码块与 info string）
- `isHtmlBlockStartCondition(text, off)` / `isHtmlBlockEndCondition(text, off, block_type)`（完整 CommonMark §4.6 Type 1–7 HTML 块起始/终止检测，内置 62 个块级标签表）
- `isTableUnderline(text, off)` -> `TableUnderlineResult`（识别 GFM 表格对齐分隔行 `left`/`center`/`right`/`none`）
