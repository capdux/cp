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
  - **词法与 XRef（对标 `lopdf` 补齐 PDF 1.5+ 支持）**：`PdfParseError`、`PdfTokenKind`、`PdfToken`、`PdfLexer`（支持 `%` 注释、嵌套 `(...)`、`<...>` 十六进制串、`/#XX` Name、`<<...>>`、`[...]`）、`PdfObjRef`、`PdfXrefEntry`（含 `is_compressed`/`obj_stm_num`/`obj_stm_index`）、`PdfTrailerInfo`（含 `is_xref_stream`/`has_xref_stm`）、`PdfXrefTable`（支持传统 `xref`、PDF 1.5+ `/Type /XRef` 交叉引用流、混合 `/XRefStm`、增量更新 `/Prev` 链最新优先覆盖及 `locateObjectBody`/`resolveObjectBody`）、`locateObjectBodyFromObjStm`（`/Type /ObjStm` 压缩对象透明提取）
  - **字符串与流解码**：`decodePdfName`、`decodePdfStringToUtf8`（支持 `\n/\r/\t/八进制` 转义、十六进制串与 UTF-16BE BOM `FE FF` + 代理对转 UTF-8）、`PdfStreamFilter`、`PdfDecodeParms`、`PdfStreamView`、`applyPdfPredictorDecode`（PNG Predictor 10..15 None/Sub/Up/Avg/Paeth 与 TIFF Predictor 2 零堆反滤波）、`extractStreamFromObject`（支持直接与间接 `/Length N 0 R` 与 `/DecodeParms`）、`decodePdfStream`（跨模块复用 `cap:pack` 的 `inflate` 实现 `/FlateDecode` + Predictor 解压，兼支持原始流与 `/ASCIIHexDecode`）
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

### 生产级零堆 CSV/TSV 方言嗅探、Schema 推理、查询/排序/聚合与 JSONL 互转 (`csvTable.cp`)

在 100% 向下兼容既有 `CsvTableReader`/`writeCsvRow` 的基础上，新增全套零堆外部切片 ETL 引擎：

- **方言嗅探 (`sniffCsvDialect`)**：自动识别 `,`/`\t`/`;`/`|`、`"`/`'`、`CRLF`/`LF`/`CR`、UTF-8 BOM 及首行表头启发式；`CsvTableReader` 支持自定义引号、`#` 注释行跳过与严格列数校验。
- **`BurntSushi/rust-csv` 对标边界加固**：`CsvTableOptions` 补齐 `quoting_enabled`、`double_quote_enabled`、`escape_char`、`trim_fields`、`skip_empty_lines`、`flexible_quotes`、`record_terminator`（含单独 `\r` 与 ASCII `\x1F`/`\x1E` 快捷工厂 `CsvTableOptions.relaxed()` / `CsvTableOptions.ascii()`）；新增零堆单遍流式解转义状态机 `CsvTableReader.readRecordUnescaped`、行号追踪 `line()`/`setLine()`、自定义转义解编码 `unescapeCsvFieldWith`，以及 `CsvQuoteStyle`/`CsvWriteOptions`（`necessary`/`always`/`never`/`non_numeric`、`always_quote`/`never_quote`/`quote_non_numeric`、自定义 `escape_char`/`comment_char`/`line_terminator`）与 `isCsvFieldNonNumeric`/`writeCsvRowWith`。
- **列 Schema 推理与 Welford 在线统计 (`inferCsvSchema`)**：单调类型格提升（`empty -> bool/int/iso_date -> float -> string`），计算 `non_null_count`/`null_count`/`min`/`max`/`sum`/`mean`/`variance()`/`sampleVariance()`/`max_str_len`。
- **过滤、投影、分组聚合与稳定排序**：
  - `CsvFilterOp` / `CsvFilterPred` / `projectAndFilterCsv`（多条件 AND 过滤 + 子列投影输出 RFC 4180 CSV）；
  - `CsvAggKind` / `CsvGroupAggSlot` / `aggregateCsvByGroup`（零堆分组 `count`/`sum`/`min`/`max`/`avg`）；
  - `CsvRowSpan` / `sortCsvRowsByColumn`（O(N log N) 迭代稳定归并排序，支持数值与词典序升降序）。
- **CSV ↔ JSONL (NDJSON) 双向零堆转换**：`csvToJsonl`（按 Schema 输出未加引号数值/布尔、`null` 与完整 RFC 8259 转义字符串）与 `jsonlToCsv`（按列头提取标量并生成规范 RFC 4180 CSV）。

### 生产级零堆 OOXML XLSX 电子表格解析、日期序列、公式求值与多 Sheet 生成引擎 (`xlsx.cp`)

在 100% 向下兼容既有 `XlsxSheetIter`/`XlsxSharedStringIter`/`XlsxCellIter`/`parseCellRef`/`formatColName` 的基础上，新增全套零堆外部切片电子表格引擎：

- **A1/范围坐标与 Excel 1900/1904 日期序列**：
  - `encodeCellRef` / `decodeCellRef`（支持 `A..XFD`、`1..1048576` 与 `$A$1` 绝对引用剥离）、`XlsxRangeRef` / `parseCellRange`；
  - `XlsxDateTime`、`excelSerialToYmd` / `ymdToExcelSerial`（精确处理 Excel 1900 闰年兼容虫 `serial 60 == 1900-02-29`、1904 Mac 日期系统与小数 `HH:MM:SS`）。
- **OOXML 多 Sheet / SharedStrings / Styles / 合并单元格解析与生成（对标 `calamine` 加固）**：
  - `XlsxSheetVisibility`（`visible` / `hidden` / `very_hidden`）、`XlsxCellErrorKind`（完整 8 种 OOXML 错误枚举：`#DIV/0!`, `#N/A`, `#NAME?`, `#NULL!`, `#NUM!`, `#REF!`, `#VALUE!`, `#GETTING_DATA` 及 `parseXlsxCellError` / `formatXlsxCellError`）；
  - `decodeExcelEscapes`（零堆解码 Excel `_xHHHH_` UTF-16 十六进制转义、`_x005F_` 防二次转义与 UTF-16 代理对）、`extractInlineString`（支持 `<is><t>` 与 `<is><r><t>` 富文本及 `xml:space="preserve"` 空白保留）；
  - `XlsxCellIter` 自动支持缺失 `<row r="...">` 自动行递增与缺失 `<c r="...">` 自动列递增（LibreOffice/Google Sheets 常见输出）、共享公式 `<f t="shared" si="..." ref="..."/>` 元数据与 `t="d"` ISO-8601 日期单元格；
  - `parseMergedCellsXml` / `findMergedCellMaster`（`<mergeCells><mergeCell ref="A1:C3"/></mergeCells>` 解析与主格查询）；
  - `isBuiltinExcelDateFmt` 与加固版 `classifyExcelNumFmt`（跳过 `"..."` 引号字面量、`\`/`_`/`*` 转义符与 `[Color]`/`[$-loc]` 方括号后再识别日期/时间与 `[h]`/`[m]`/`[s]` 耗时格式）；
  - `parseSharedStringsXml`、`parseWorkbookSheetsXml`、`parseStylesNumFmtsXml`、`parseWorksheetCellsXml`、`buildSharedStringsXml`、`buildWorkbookXml`、`buildWorksheetXml`。
- **零堆表格公式求值器 (`evalXlsxFormula`)**：
  - 支持 `+ - * /` 括号优先级、单元格引用（`$A$1`）、范围聚合（`SUM`/`AVERAGE`/`MIN`/`MAX`/`COUNT`）、条件分支（`IF` 含 `= <> < <= > >=`）、`ABS`/`ROUND`，内置除零（`DivisionByZero`）、无效引用（`InvalidReference`）与循环引用（`CycleDetected`）防护。

### 生产级零堆 OOXML DOCX 富文本/表格/超链接/批注解析与生成及 Markdown 互转 (`docx.cp`)

在 100% 向下兼容既有 `DocxCoreProps`/`DocxParagraphIter`/`extractDocxText`/`summarizeDocx` 的基础上，新增全套零堆外部切片 WordprocessingML 引擎：

- **关系表与超链接解析/生成**：`DocxRel`、`parseDocxRelsXml`、`lookupDocxRelTarget`、`buildDocxRelsXml`；
- **富文本/表格/批注解析**：`parseDocxDocumentXml`（保留段落与表格交错顺序，解析 `Heading1..6`、列表 `<w:numPr>`、对齐 `<w:jc>`、加粗/斜体/下划线/删除线/字号、`<w:tab/>`/`<w:br/>`、XML 实体解码、`<w:hyperlink>` URL 关联及 `<w:tbl>` 含 `gridSpan` 跨列合并）与 `parseDocxCommentsXml`（审阅批注 `id`/`author`/`date`/正文）；
- **XML 生成与 Markdown 双向互转**：`buildDocxDocumentXml`、`docxXmlToMarkdown`、`markdownToDocxXml` 及中英文字数统计 `computeDocxWordStats`（`paragraph_count`/`table_count`/`char_count`/`english_word_count`/`cjk_char_count`）；
- **OOXML 安全与 16-lane SIMD 加固（对标 `roxmltree` / ECMA-376）**：`OOXML_MAX_ZIP_ENTRIES`、`OOXML_MAX_PART_SIZE`、`OOXML_MAX_TOTAL_UNCOMPRESSED`、`OOXML_MAX_XML_DEPTH`、`isSafeOoxmlPartPath`（拦截 `../`/绝对路径/盘符/NUL）、`validateDocxStrict`（零堆校验 ZIP 容器与 XML 良构/实体/代理对/控制字节/未闭合标签），以及 16-lane `@Vector(16, u8)` 热路径原语 `simdOoxmlFindTagOrAmp`/`simdOoxmlBytesEqual`（含标量基线）。

### 生产级零堆 OOXML PPTX 幻灯片/表格/演讲者备注解析生成与大纲导出 (`pptx.cp`)

在 100% 向下兼容既有 `PptxSlideSize`/`PptxSlideRefIter`/`PptxParagraphIter`/`extractSlideText`/`summarizeSlide` 的基础上，新增全套零堆外部切片 PresentationML 引擎：

- **演示文稿尺寸与 EMU/磅换算**：`EMU_PER_PT` (`12700`)、`PPTX_SLIDE_16_9_CX/CY`、`emuToPt`/`ptToEmu`、`parsePptxPresentationXml`、`buildPptxPresentationXml`；
- **富文本形状/原生表格/备注解析与生成**：`parsePptxSlideXml`（解析占位符 `title`/`ctrTitle`/`subTitle`/`body`/`ftr`/`sldNum`、EMU 几何 `<a:off>`/`<a:ext>`、多级项目符号 `lvl`、粗斜体/字号/RGB 颜色与原生 `<a:tbl>` 表格）、`parsePptxNotesXml`、`buildPptxSlideXml`、`buildPptxNotesXml`；
- **大纲 Markdown 导出与多语言统计**：`pptxSlideXmlToMarkdown`（`## Slide N: ...` + 缩进项目符号 + GFM 表格 + `> Notes: ...`）与 `computePptxSlideStats`（中英文词/字数 + 备注字数）；
- **OOXML 安全与 16-lane SIMD 加固（对标 `roxmltree` / ECMA-376）**：`validatePptxStrict`（零堆校验 `.pptx` ZIP 包路径穿越、资源上限、`ppt/presentation.xml` 与 `ppt/slides/slide*.xml` 核心部件完整性及严格 XML 实体/闭合标签校验），共享 16-lane SIMD 文本扫描与标签匹配。

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
