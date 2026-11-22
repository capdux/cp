# `cap:pptx` — 纯 CP OOXML PPTX PresentationML 与 DrawingML 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:doc`，导入请使用 `import { ... } from "cap:doc"`。

## 概述

`cap:pptx` 提供纯 CP、零外部依赖、零堆分配的 OOXML PresentationML（`.pptx`：`ppt/presentation.xml`、`ppt/slides/slide*.xml`、`ppt/notesSlides/notesSlide*.xml`）解析与文本提取能力：

- **格式判定**：精准识别 `<p:presentation>`、`<p:sld>`、`<p:notes>`（自动跳过 UTF-8 BOM、XML 声明与注释，且不误判 `<p:sldMaster>` / `<p:sldLayout>` / `<p:notesMaster>` 等同前缀标签）；
- **演示文稿清单与页面尺寸**：解析 `<p:sldSz cx="..." cy="..." type="..."/>` 并提供 `widthInches()` / `heightInches()` EMU（`914400.0`）英寸换算；零分配迭代 `<p:sldIdLst>` 内全部 `<p:sldId id="..." r:id="..."/>`；
- **幻灯片与备注页纯文本提取**：零分配迭代 `<a:p>`（精准区分 `<a:pPr>` / `<a:pBdr>`），提取 `<a:t>`、`<a:br/>` -> `'\n'`、`<a:tab/>` -> `'\t'`、段落间 `'\n'` 分隔，完整支持 XML 命名与数字实体 UTF-8 反转义，缓冲区不足安全返回 `null`；
- **幻灯片统计汇总**：精确统计形状（`<p:sp>`）、段落（`<a:p>`）、文本运行块（`<a:r>`）、表格（`<a:tbl>`）与图片（`<p:pic>`）数量。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `PPTX_MAIN_CONTENT_TYPE` / `EMU_PER_INCH` | PresentationML 主文档 MIME 类型与 EMU/英寸换算常量 (`914400.0`) |
| `isPresentationXml(xml)` / `isSlideXml(xml)` / `isNotesSlideXml(xml)` | 判定 `presentation.xml`、`slide*.xml`、`notesSlide*.xml` 根元素 |
| `PptxSlideSize` / `parsePptxSlideSize(pres_xml): PptxSlideSize?` | 页面尺寸 `cx`, `cy`, `size_type` 及 `widthInches()` / `heightInches()` |
| `PptxSlideRef` / `PptxSlideRefIter` | 零分配迭代 `<p:sldIdLst>` 中的 `<p:sldId id="..." r:id="..."/>` |
| `PptxParagraphIter` | 零分配迭代 `<a:p>...</a:p>` 段落切片 |
| `extractSlideText(slide_xml, out): usize?` | 提取单页幻灯片或演讲者备注纯文本，非幻灯片/备注或溢出返回 `null` |
| `PptxSlideStats` / `summarizeSlide(slide_xml): PptxSlideStats` | 统计 `shape_count`, `paragraph_count`, `text_run_count`, `table_count`, `pic_count` |

## 示例

```cp
import std from "std";
import {
    parsePptxSlideSize,
    PptxSlideRefIter,
    extractSlideText,
    summarizeSlide,
} from "cap:doc";

fn inspectDeck(pres_xml: []const u8, slide_xml: []const u8): usize {
    if (sz := parsePptxSlideSize(pres_xml)) {
        * = sz.widthInches();
        * = sz.heightInches();
    }

    let refs = PptxSlideRefIter.init(pres_xml);
    while (ref := refs.next()) {
        * = ref.id;
        * = ref.r_id;
    }

    const stats = summarizeSlide(slide_xml);
    * = stats.shape_count;

    let buf: [4096]u8 = std.mem.zeroes([4096]u8);
    return extractSlideText(slide_xml, buf[0..]) ?? 0;
}
```
