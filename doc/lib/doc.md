# `cap:doc` — 统一文档、出版、日程、联系人、订阅与流式表格标准库

> 聚合路径：`lib/doc/index.cp`（`import { ... } from "cap:doc"` / `import cap from "cap"; cap.doc.*`）

`cap:doc` 是 CP 标准库的统一文档与出版格式主包，零外部 C 依赖，涵盖 **办公文档（PDF / OOXML / ODF / RTF）**、**电子书（EPUB）**、**学术引用（BibTeX）**、**日程与联系人（iCalendar / vCard）**、**订阅源（RSS 2.0 / Atom 1.0）** 与 **零堆分配流式 CSV/TSV**。

---

## 模块架构总览

| 分类 | 子模块文件 | 协议 / 标准 | 核心能力 |
| --- | --- | --- | --- |
| 办公与电子书 | `pdfWriter.cp` / `pdfInspector.cp` | PDF 1.4 | 多页文档生成、Base14 字体、矢量绘制、精确 10 位 xref、结构与元数据解析 |
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
| 流式表格 | `csvStream.cp` | RFC 4180 CSV / TSV | 零堆分配切片级流式读取器、字段反转义、引号检测与行格式化 |
| 强类型表格 | `csvTable.cp` | RFC 4180 CSV / TSV | 零堆分配表头列名映射、强类型 `i64`/`f64`/`bool` 解码与 RFC 4180 行写入器（`CsvTableReader` / `writeCsvRow`） |
| 参考归档 | `markdown_table_.cp` | GFM Pipe Table（仅参考） | 与权威 Bun MD4C 引擎 `lib/md/table.cp` 功能重叠，仅作对比归档，不在 `cap:doc` 重复导出 |

---

## 核心 API 速查

### 1. PDF 1.4–1.7 生成、对象/XRef/FlateDecode 解析与文本提取 (`pdfWriter.cp` / `pdfInspector.cp`)
- **向后兼容生成器 (`pdfWriter.cp`)**：`PdfError`, `PdfFont`, `PageSize`（含 `a4` / `letter` / `a4Landscape` / `letterLandscape` / `custom`）, `PdfPage`, `PdfDocument`。
- **生产级零堆外部切片排版与序列化引擎 (`pdfWriter.cp`)**：
  - `PdfWriteError`、`PdfTextAlign`（`left | center | right`）、`PdfColorSpace`（`device_rgb | device_gray`）、`PdfRgb`、`PdfMargins`、`PdfPoint`、`PdfTableStyle`、`PdfImageSpec`、`PdfBookmarkSpec`、`PdfMetadataSpec`、`PdfPageSpec`
  - `PdfPageCanvas`：多行自动换行（`drawWrappedText`）、页眉页脚与页码（`drawHeaderFooter`）、结构化表格渲染（`drawTable`）、矢量原语（`drawLine` / `drawRect` / `fillRect` / `fillStrokeRect` / `drawPolygon` / `drawRoundedRect` / 虚线 `setDashPattern`）与图像 XObject 绘制（`drawImageXObject`）
  - `pdfWriterXrefWorkLen` & `writePdfDocument`：支持 `/Filter /FlateDecode` 内容流与图像流压缩（复用 `cap:pack` `deflate`）、RGB/Gray 8-bit `Image XObject`、`/Outlines` 书签链、ASCII + UTF-16BE BOM `/Info` 元数据与精确 10 位 `xref` 表，与 `pdfInspector.cp` 100% 双向闭环。
- **轻量校验 (`pdfInspector.cp` 向后兼容)**：`PdfInfo`, `PdfInspector`（`extractVersion` / `isValidPdf` / `inspect`）。
- **生产级 PDF 1.4–1.7 对象/XRef/流解压/页面树/文本提取引擎（零堆外部切片）**：
  - **词法与 XRef**：`PdfParseError`、`PdfTokenKind`、`PdfToken`、`PdfLexer`（支持 `%` 注释、嵌套 `(...)`、`<...>` 十六进制串、`/#XX` Name、`<<...>>`、`[...]`）、`PdfObjRef`、`PdfXrefEntry`、`PdfTrailerInfo`、`PdfXrefTable`（支持多段 subsection 与增量更新 `/Prev` 链最新优先覆盖、`locateObjectBody`）
  - **字符串与流解码**：`decodePdfName`、`decodePdfStringToUtf8`（支持 `\n/\r/\t/八进制` 转义、十六进制串与 UTF-16BE BOM `FE FF` + 代理对转 UTF-8）、`PdfStreamFilter`、`PdfStreamView`、`extractStreamFromObject`（支持直接与间接 `/Length N 0 R`）、`decodePdfStream`（跨模块复用 `cap:pack` 的 `inflate` 实现 `/FlateDecode` 解压，兼支持原始流与 `/ASCIIHexDecode`）
  - **元数据、页面树与大纲**：`PdfMetaSpan`、`PdfDocMetadata`、`extractPdfMetadata`（8 项 `/Info` + `/Encrypt` 检测）、`PdfRect`、`PdfPageInfo`、`collectPdfPages`（外部栈迭代遍历任意页数 `/Pages -> /Kids` + 属性继承）、`PdfOutlineItem`、`extractPdfOutlines`
  - **正文文本提取 (`BT ... ET`)**：`extractTextFromContentStream`（支持 `Tj`、`TJ` 自动 kerning 补空格、`'`、`"`、`T*`、`Td`/`TD`/`Tm` 换行与词距启发式）与 `extractPageText`（兼容单流与 `/Contents [...]` 多流数组）

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

### 5. 订阅源与流式表格 (`rss.cp` / `atom.cp` / `csvStream.cp`)
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

### 零堆分配 RFC 4180 表头映射与强类型行解码示例 (`csvTable.cp`)

```cp
import {
    CsvTableOptions,
    CsvTableReader,
    parseCsvI64,
    parseCsvF64,
    parseCsvBool,
    writeCsvRow,
} from "cap:doc";

pub fn demoCsvTable() {
    const csv = "id,name,score,active\n101,\"Alice, PhD\",98.5,yes\n";
    let reader = CsvTableReader.init(csv, CsvTableOptions.csv());

    let hdr_names: [8]string = [""; 8];
    if (hdr := reader.parseHeader(hdr_names[0..])) {
        const col_id = hdr.columnIndex("id") ?? return;
        const col_score = hdr.columnIndex("score") ?? return;
        const col_active = hdr.columnIndex("active") ?? return;

        let cols: [8]string = [""; 8];
        if (cur := reader.nextRow(cols[0..])) {
            if (!cur.malformed) {
                * = parseCsvI64(cols[col_id]);      // 101
                * = parseCsvF64(cols[col_score]);   // 98.5
                * = parseCsvBool(cols[col_active]); // true
            }
        }
    }

    const row: [2]string = ["hello, world", "say \"hi\""];
    let out: [64]u8 = [0; 64];
    * = writeCsvRow(row[0..], ',', out[0..]);
}
```

---

## 收敛子模块深度文档

- [atom.md](doc/atom.md)
- [bibtex.md](doc/bibtex.md)
- [csv_stream.md](doc/csv_stream.md)
- [docx.md](doc/docx.md)
- [epub.md](doc/epub.md)
- [ical.md](doc/ical.md)
- [markdown_table.md](doc/markdown_table.md)
- [odt.md](doc/odt.md)
- [pdf.md](doc/pdf.md)
- [pptx.md](doc/pptx.md)
- [rss.md](doc/rss.md)
- [rtf.md](doc/rtf.md)
- [vcard.md](doc/vcard.md)
- [xlsx.md](doc/xlsx.md)
