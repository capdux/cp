// Source: cp/lib/doc/index.cp (Unified Document, Publishing, Calendar, Contact, Feed & Table Formats Barrel)
// Consolidated cap:doc standard library barrel covering PDF, OOXML (DOCX/XLSX/PPTX),
// EPUB, OpenDocument (ODT/ODS/ODP), RTF, BibTeX, iCalendar, vCard, RSS 2.0, Atom 1.0,
// and zero-alloc streaming CSV/TSV.
// Note: GFM pipe-table reference (`markdown_table_.cp`) is archived for comparison with
// authoritative `lib/md/table.cp` and intentionally excluded from barrel exports.

// --- PDF 1.4 Writer & Structural/Metadata Inspector ---
export {
    PdfError,
    PdfFont,
    PageSize,
    PdfPage,
    PdfDocument,
} from "./pdfWriter.cp";

export {
    PdfInfo,
    PdfInspector,
} from "./pdfInspector.cp";

// --- OOXML DOCX WordprocessingML & CoreProps ---
export {
    DOCX_MAIN_CONTENT_TYPE,
    DocxCoreProps,
    DocxParagraph,
    DocxStats,
    DocxParagraphIter,
    isWordDocumentXml,
    isDocxContentTypes,
    parseDocxCoreProps,
    extractDocxParagraphText,
    extractDocxText,
    summarizeDocx,
} from "./docx.cp";

// --- OOXML XLSX SpreadsheetML ---
export {
    MAX_XLSX_COL,
    MAX_XLSX_ROW,
    XlsxCellRef,
    XlsxSheetInfo,
    XlsxCellType,
    XlsxCell,
    XlsxSheetIter,
    XlsxSharedStringIter,
    XlsxCellIter,
    parseCellRef,
    formatColName,
    extractSharedString,
} from "./xlsx.cp";

// --- OOXML PPTX PresentationML & DrawingML ---
export {
    PPTX_MAIN_CONTENT_TYPE,
    EMU_PER_INCH,
    PptxSlideSize,
    PptxSlideRef,
    PptxSlideStats,
    PptxSlideRefIter,
    PptxParagraphIter,
    isPresentationXml,
    isSlideXml,
    isNotesSlideXml,
    parsePptxSlideSize,
    extractSlideText,
    summarizeSlide,
} from "./pptx.cp";

// --- EPUB 2/3 OCF & OPF E-Book Package ---
export {
    EPUB_MIMETYPE,
    EPUB_OPF_MEDIA_TYPE,
    EpubRootfile,
    EpubMetadata,
    EpubManifestItem,
    EpubSpineItem,
    isEpubZip,
    parseContainerXml,
    parseOpfMetadata,
    EpubManifestIter,
    findManifestItem,
    EpubSpineIter,
} from "./epub.cp";

// --- OASIS OpenDocument ODT/ODS/ODP ---
export {
    ODT_MIMETYPE,
    ODS_MIMETYPE,
    ODP_MIMETYPE,
    OdtMeta,
    OdtManifestEntry,
    OdtBlock,
    detectOdfMimetype,
    isOdtZip,
    parseOdtMeta,
    OdtManifestIter,
    OdtBlockIter,
    extractOdtText,
} from "./odt.cp";

// --- RTF 1.9 Lexer & Text Extractor ---
export {
    RTF_MAGIC,
    MAX_RTF_DEPTH,
    RtfTokenKind,
    RtfCharset,
    RtfToken,
    RtfHeaderInfo,
    isRtf,
    RtfLexer,
    parseRtfHeader,
    extractRtfText,
} from "./rtf.cp";

// --- BibTeX Academic Citations ---
export {
    BibEntry,
    BibField,
    BibEntryIter,
    BibFieldIter,
    findBibField,
    formatBibField,
} from "./bibtex.cp";

// --- RFC 5545 iCalendar & RRULE Expansion ---
export {
    IcalError,
    RRuleFreq,
    Weekday,
    IcalDateTime,
    RRule,
    IcalEvent,
    IcalCalendar,
} from "./ical.cp";

// --- RFC 6350 / 2426 vCard 3.0 & 4.0 ---
export {
    VCardError,
    VCardVersion,
    StructuredName,
    TypedValue,
    VCardAddress,
    VCardContact,
    VCardCodec,
} from "./vcard.cp";

// --- RSS 2.0 Feed Parser & Writer ---
export {
    RssChannel,
    RssItem,
    isRss2Feed,
    parseRssChannel,
    RssItemIter,
    formatRssItem,
} from "./rss.cp";

// --- RFC 4287 Atom 1.0 Feed Parser & Writer ---
export {
    AtomFeed,
    AtomEntry,
    isAtomFeed,
    parseAtomFeed,
    AtomEntryIter,
    formatAtomEntry,
} from "./atom.cp";

// --- Zero-Alloc RFC 4180 CSV/TSV Streaming ---
export {
    CsvField,
    CsvRow,
    CsvReader,
    unescapeField,
    needsCsvQuoting,
    formatCsvField,
    formatCsvRow,
} from "./csvStream.cp";

// --- Zero-Alloc RFC 4180 CSV/TSV Table Header Mapping & Typed Row Decoder ---
export {
    CsvTableOptions,
    CsvRowCursor,
    CsvTableHeader,
    CsvTableReader,
    unescapeCsvField,
    parseCsvI64,
    parseCsvF64,
    parseCsvBool,
    writeCsvRow,
} from "./csvTable.cp";
