# `cap:doc` — 统一文档、出版、日程、联系人、订阅与流式表格标准库

> 聚合路径：`lib/doc/index.cp`（`import { ... } from "cap:doc"` / `import cap from "cap"; cap.doc.*`）

`cap:doc` 是 CP 标准库的统一文档与出版格式主包，零外部 C 依赖，涵盖 **办公文档（PDF / OOXML / ODF / RTF）**、**电子书（EPUB）**、**学术引用（BibTeX）**、**日程与联系人（iCalendar / vCard）**、**订阅源（RSS 2.0 / Atom 1.0）** 与 **零堆分配流式 CSV/TSV**。

---

## 模块架构总览

| 分类 | 子模块文件 | 协议 / 标准 | 核心能力 |
| --- | --- | --- | --- |
| 办公与电子书 | `pdf_writer.cp` / `pdf_inspector.cp` | PDF 1.4 | 多页文档生成、Base14 字体、矢量绘制、精确 10 位 xref、结构与元数据解析 |
| 办公与电子书 | `docx.cp` | OOXML WordprocessingML | 零堆分配 `.docx` 段落迭代、纯文本提取、CoreProps 元数据与文档统计 |
| 办公与电子书 | `xlsx.cp` | OOXML SpreadsheetML | A1 单元格引用编解码、工作表迭代、SharedStrings 迭代、单元格流式遍历 |
| 办公与电子书 | `pptx.cp` | OOXML PresentationML / DrawingML | 幻灯片尺寸/引用迭代、段落文本提取与幻灯片统计 |
| 办公与电子书 | `epub.cp` | EPUB 2/3 (OCF & OPF) | ZIP 校验、`container.xml` 根路径解析、OPF 元数据、Manifest 与 Spine 迭代 |
| 办公与电子书 | `odt.cp` | OASIS OpenDocument (ODT/ODS/ODP) | ODF MIME 检测、`meta.xml` 解析、Manifest 迭代、内容块迭代与纯文本提取 |
| 办公与电子书 | `rtf.cp` | RTF 1.9 | 零堆分配 RTF 词法分析器、头部编码信息解析与纯文本提取 |
| 学术与出版 | `bibtex.cp` | BibTeX (`.bib`) | 零拷贝文献条目与字段迭代、字段查找与格式化输出 |
| 日程与联系人 | `ical.cp` | RFC 5545 iCalendar | 折行展开、文本转义、日期算术、`VEVENT`/`VCALENDAR` 解析生成与 `RRULE` 展开引擎 |
| 日程与联系人 | `vcard.cp` | RFC 6350 / 2426 vCard 3.0 & 4.0 | 联系人解析与序列化、分组前缀、`TYPE` 参数、`N`/`ADR` 结构化字段 |
| 订阅源 | `rss.cp` | RSS 2.0 | 频道级元数据解析、条目零堆分配迭代器与 XML 条目格式化 |
| 订阅源 | `atom.cp` | RFC 4287 Atom 1.0 | Feed 元数据解析、Entry 零拷贝迭代器与 Entry 序列化 |
| 流式表格 | `csv_stream.cp` | RFC 4180 CSV / TSV | 零堆分配切片级流式读取器、字段反转义、引号检测与行格式化 |
| 参考归档 | `markdown_table_.cp` | GFM Pipe Table（仅参考） | 与权威 Bun MD4C 引擎 `lib/md/table.cp` 功能重叠，仅作对比归档，不在 `cap:doc` 重复导出 |

---

## 核心 API 速查

### 1. PDF 1.4 (`pdf_writer.cp` / `pdf_inspector.cp`)
- 类型：`PdfError`, `PdfFont`, `PageSize`, `PdfPage`, `PdfDocument`, `PdfInfo`, `PdfInspector`
- 支持 A4 / Letter / 自定义尺寸、4 种 Base14 字体、RGB 描边与填充、线段/矩形绘制、精确字节偏移 xref 表及结构验证。

### 2. OOXML 办公套件 (`docx.cp` / `xlsx.cp` / `pptx.cp`)
- **DOCX**：`DOCX_MAIN_CONTENT_TYPE`, `DocxCoreProps`, `DocxParagraph`, `DocxStats`, `DocxParagraphIter`, `isWordDocumentXml`, `isDocxContentTypes`, `parseDocxCoreProps`, `extractDocxParagraphText`, `extractDocxText`, `summarizeDocx`
- **XLSX**：`MAX_XLSX_COL`, `MAX_XLSX_ROW`, `XlsxCellRef`, `XlsxSheetInfo`, `XlsxCellType`, `XlsxCell`, `XlsxSheetIter`, `XlsxSharedStringIter`, `XlsxCellIter`, `parseCellRef`, `formatColName`, `extractSharedString`
- **PPTX**：`PPTX_MAIN_CONTENT_TYPE`, `EMU_PER_INCH`, `PptxSlideSize`, `PptxSlideRef`, `PptxSlideStats`, `PptxSlideRefIter`, `PptxParagraphIter`, `isPresentationXml`, `isSlideXml`, `isNotesSlideXml`, `parsePptxSlideSize`, `extractSlideText`, `summarizeSlide`

### 3. 电子书、ODF 与富文本 (`epub.cp` / `odt.cp` / `rtf.cp`)
- **EPUB**：`EPUB_MIMETYPE`, `EPUB_OPF_MEDIA_TYPE`, `EpubRootfile`, `EpubMetadata`, `EpubManifestItem`, `EpubSpineItem`, `isEpubZip`, `parseContainerXml`, `parseOpfMetadata`, `EpubManifestIter`, `findManifestItem`, `EpubSpineIter`
- **ODT/ODS/ODP**：`ODT_MIMETYPE`, `ODS_MIMETYPE`, `ODP_MIMETYPE`, `OdtMeta`, `OdtManifestEntry`, `OdtBlock`, `detectOdfMimetype`, `isOdtZip`, `parseOdtMeta`, `OdtManifestIter`, `OdtBlockIter`, `extractOdtText`
- **RTF**：`RTF_MAGIC`, `MAX_RTF_DEPTH`, `RtfTokenKind`, `RtfCharset`, `RtfToken`, `RtfHeaderInfo`, `isRtf`, `RtfLexer`, `parseRtfHeader`, `extractRtfText`

### 4. 学术、日程与联系人 (`bibtex.cp` / `ical.cp` / `vcard.cp`)
- **BibTeX**：`BibEntry`, `BibField`, `BibEntryIter`, `BibFieldIter`, `findBibField`, `formatBibField`
- **iCalendar**：`IcalError`, `RRuleFreq`, `Weekday`, `IcalDateTime`, `RRule`, `IcalEvent`, `IcalCalendar`
- **vCard**：`VCardError`, `VCardVersion`, `StructuredName`, `TypedValue`, `VCardAddress`, `VCardContact`, `VCardCodec`

### 5. 订阅源与流式表格 (`rss.cp` / `atom.cp` / `csv_stream.cp`)
- **RSS 2.0**：`RssChannel`, `RssItem`, `isRss2Feed`, `parseRssChannel`, `RssItemIter`, `formatRssItem`
- **Atom 1.0**：`AtomFeed`, `AtomEntry`, `isAtomFeed`, `parseAtomFeed`, `AtomEntryIter`, `formatAtomEntry`
- **CSV/TSV 流式**：`CsvField`, `CsvRow`, `CsvReader`, `unescapeField`, `needsCsvQuoting`, `formatCsvField`, `formatCsvRow`

---

## 使用示例

```cp
const std = @import("std");
import {
    PdfDocument,
    PageSize,
    PdfFont,
    PdfInspector,
    parseCellRef,
    RssItemIter,
    CsvReader,
} from "cap:doc";

fn main()! {
    const alloc = std.heap.page_allocator;

    // 1. 生成并校验 PDF 1.4 文档
    let pdf = PdfDocument.init(alloc);
    defer pdf.deinit();
    const page = pdf.addPage(PageSize.a4())?;
    page.drawText(72.0, 720.0, PdfFont.HelveticaBold, 16.0, "Hello cap:doc")?;
    const bytes = pdf.build(alloc)?;
    defer alloc.free(bytes);
    const info = PdfInspector.inspect(bytes)?;
    * = info;

    // 2. 零堆分配解析 XLSX A1 引用
    if (ref := parseCellRef("AB42")) {
        * = ref.col;
    }

    // 3. 零堆分配流式读取 CSV
    let reader = CsvReader.init("name,score\nAlice,99\n", ',');
    while (row := reader.nextRow()) {
        * = row;
    }
}
```
