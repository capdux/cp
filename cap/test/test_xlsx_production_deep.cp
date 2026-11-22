// Source: cp/cap/test/test_xlsx_production_deep.cp
// Deep Production Test Suite for cap:xlsx
// Covers:
//   1. A1/Z10/AA1/XFD1048576 bidirectional cell reference & column name codec
//   2. xl/workbook.xml multi-sheet & hidden/veryHidden state iteration
//   3. xl/sharedStrings.xml plain & rich-text <r><t> concatenation, <rPh> skipping & XML entity unescaping
//   4. xl/worksheets/sheet1.xml streaming cell iteration (number, shared_string, inline_string, formula, bool, error, empty)
//   5. Invalid cell references, out-of-bounds col/row, non-XML & truncated XML resilience

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
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
} from "cap:doc";

fn approxEq(a: f64, b: f64): bool {
    return @abs(a - b) < 0.000001;
}

fn testCellRefAndColNameCodec() {
    // A1 -> (0, 0)
    const a1 = parseCellRef("A1").?;
    expect_eq(a1.col, 0);
    expect_eq(a1.row, 0);

    // Z10 -> (25, 9)
    const z10 = parseCellRef("Z10").?;
    expect_eq(z10.col, 25);
    expect_eq(z10.row, 9);

    // AA1 -> (26, 0)
    const aa1 = parseCellRef("AA1").?;
    expect_eq(aa1.col, 26);
    expect_eq(aa1.row, 0);

    // AZ100 -> (51, 99)
    const az100 = parseCellRef("AZ100").?;
    expect_eq(az100.col, 51);
    expect_eq(az100.row, 99);

    // XFD1048576 -> (16383, 1048575)
    const max_ref = parseCellRef("XFD1048576").?;
    expect_eq(max_ref.col, MAX_XLSX_COL);
    expect_eq(max_ref.row, MAX_XLSX_ROW);

    // Bidirectional formatColName
    let buf: [4]u8 = [0; 4];

    const n0 = formatColName(0, buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n0], "A"), true);

    const n25 = formatColName(25, buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n25], "Z"), true);

    const n26 = formatColName(26, buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n26], "AA"), true);

    const n701 = formatColName(701, buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n701], "ZZ"), true);

    const n702 = formatColName(702, buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n702], "AAA"), true);

    const n_max = formatColName(16383, buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n_max], "XFD"), true);

    // Buffer too small returns null
    let tiny: [2]u8 = [0; 2];
    expect_eq(formatColName(16383, tiny[0..]) == null, true);
    expect_eq(formatColName(16384, buf[0..]) == null, true);
}

fn testWorkbookSheetIteration() {
    const wb_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" ++
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" " ++
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">" ++
        "<!-- comment with <sheet name=\"Ignored\" sheetId=\"99\" r:id=\"rId99\"/> -->" ++
        "<sheets>" ++
        "<sheet name=\"Summary\" sheetId=\"1\" r:id=\"rId1\"/>" ++
        "<sheet name=\"RawData\" sheetId=\"2\" state=\"hidden\" r:id=\"rId2\"/>" ++
        "<x:sheet name=\"AuditLog\" sheetId=\"5\" state=\"veryHidden\" r:id=\"rId3\"/>" ++
        "<sheet name=\"Chart\" sheetId=\"7\" state=\"visible\" r:id=\"rId4\"/>" ++
        "</sheets>" ++
        "</workbook>";

    let it = XlsxSheetIter.init(wb_xml);

    const s0 = it.next().?;
    expect_eq(std.mem.eql(u8, s0.name, "Summary"), true);
    expect_eq(s0.sheet_id, 1);
    expect_eq(std.mem.eql(u8, s0.r_id, "rId1"), true);
    expect_eq(s0.is_hidden, false);

    const s1 = it.next().?;
    expect_eq(std.mem.eql(u8, s1.name, "RawData"), true);
    expect_eq(s1.sheet_id, 2);
    expect_eq(std.mem.eql(u8, s1.r_id, "rId2"), true);
    expect_eq(s1.is_hidden, true);

    const s2 = it.next().?;
    expect_eq(std.mem.eql(u8, s2.name, "AuditLog"), true);
    expect_eq(s2.sheet_id, 5);
    expect_eq(std.mem.eql(u8, s2.r_id, "rId3"), true);
    expect_eq(s2.is_hidden, true);

    const s3 = it.next().?;
    expect_eq(std.mem.eql(u8, s3.name, "Chart"), true);
    expect_eq(s3.sheet_id, 7);
    expect_eq(std.mem.eql(u8, s3.r_id, "rId4"), true);
    expect_eq(s3.is_hidden, false);

    expect_eq(it.next() == null, true);
}

fn testSharedStringsAndRichTextUnescape() {
    const sst_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" ++
        "<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" count=\"4\" uniqueCount=\"4\">" ++
        "<si><t>Hello &amp; Welcome &lt;2026&gt;</t></si>" ++
        "<si>" ++
        "<r><rPr><b/></rPr><t xml:space=\"preserve\">Total: </t></r>" ++
        "<r><t>&quot;100%&apos;s&quot; &#65; &#x42;</t></r>" ++
        "<rPh sb=\"0\" eb=\"1\"><t>PHONETIC</t></rPh>" ++
        "</si>" ++
        "<si/>" ++
        "<si><t/></si>" ++
        "</sst>";

    let it = XlsxSharedStringIter.init(sst_xml);
    let buf: [128]u8 = [0; 128];

    const si0 = it.next().?;
    const n0 = extractSharedString(si0, buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n0], "Hello & Welcome <2026>"), true);

    const si1 = it.next().?;
    const n1 = extractSharedString(si1, buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n1], "Total: \"100%'s\" A B"), true);

    // Output buffer too small returns null
    let small_buf: [5]u8 = [0; 5];
    expect_eq(extractSharedString(si1, small_buf[0..]) == null, true);

    const si2 = it.next().?;
    const n2 = extractSharedString(si2, buf[0..]).?;
    expect_eq(n2, 0);

    const si3 = it.next().?;
    const n3 = extractSharedString(si3, buf[0..]).?;
    expect_eq(n3, 0);

    expect_eq(it.next() == null, true);
}

fn testWorksheetCellStreaming() {
    const sheet_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" ++
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">" ++
        "<cols><col min=\"1\" max=\"5\" width=\"12\"/></cols>" ++
        "<sheetData>" ++
        "<row r=\"1\">" ++
        "<c r=\"A1\" t=\"s\" s=\"1\"><v>0</v></c>" ++
        "<c r=\"B1\" s=\"2\"><v>123.45</v></c>" ++
        "<c r=\"C1\" t=\"b\"><v>1</v></c>" ++
        "<c r=\"D1\" t=\"b\"><v>0</v></c>" ++
        "</row>" ++
        "<row r=\"2\">" ++
        "<c r=\"A2\" t=\"inlineStr\"><is><t>Direct Text</t></is></c>" ++
        "<c r=\"B2\"/>" ++
        "<c r=\"C2\" t=\"str\" s=\"4\"><f>CONCAT(A1,A2)</f><v>Joined</v></c>" ++
        "<c r=\"D2\" t=\"e\"><v>#DIV/0!</v></c>" ++
        "<c r=\"E2\"><f>B1*2</f><v>246.9</v></c>" ++
        "</row>" ++
        "</sheetData>" ++
        "</worksheet>";

    let it = XlsxCellIter.init(sheet_xml);

    // A1: shared_string index 0, style 1
    const c0 = it.next().?;
    expect_eq(c0.ref.col, 0);
    expect_eq(c0.ref.row, 0);
    expect_eq(c0.cell_type == XlsxCellType.shared_string, true);
    expect_eq(c0.style_idx, 1);
    expect_eq(c0.asSharedStringIdx().?, 0);
    expect_eq(c0.asF64() == null, true);

    // B1: number 123.45, style 2
    const c1 = it.next().?;
    expect_eq(c1.ref.col, 1);
    expect_eq(c1.ref.row, 0);
    expect_eq(c1.cell_type == XlsxCellType.number, true);
    expect_eq(c1.style_idx, 2);
    expect_eq(approxEq(c1.asF64().?, 123.45), true);

    // C1: bool true
    const c2 = it.next().?;
    expect_eq(c2.cell_type == XlsxCellType.bool_val, true);
    expect_eq(c2.asBool().?, true);

    // D1: bool false
    const c3 = it.next().?;
    expect_eq(c3.cell_type == XlsxCellType.bool_val, true);
    expect_eq(c3.asBool().?, false);

    // A2: inlineStr
    const c4 = it.next().?;
    expect_eq(c4.ref.col, 0);
    expect_eq(c4.ref.row, 1);
    expect_eq(c4.cell_type == XlsxCellType.inline_string, true);
    expect_eq(std.mem.eql(u8, c4.raw_value, "Direct Text"), true);

    // B2: self-closing empty cell
    const c5 = it.next().?;
    expect_eq(c5.ref.col, 1);
    expect_eq(c5.ref.row, 1);
    expect_eq(c5.raw_value.len, 0);
    expect_eq(c5.formula.len, 0);
    expect_eq(c5.asF64() == null, true);

    // C2: formula_str with <f> and <v>
    const c6 = it.next().?;
    expect_eq(c6.cell_type == XlsxCellType.formula_str, true);
    expect_eq(c6.style_idx, 4);
    expect_eq(std.mem.eql(u8, c6.formula, "CONCAT(A1,A2)"), true);
    expect_eq(std.mem.eql(u8, c6.raw_value, "Joined"), true);

    // D2: error_val
    const c7 = it.next().?;
    expect_eq(c7.cell_type == XlsxCellType.error_val, true);
    expect_eq(std.mem.eql(u8, c7.raw_value, "#DIV/0!"), true);

    // E2: numeric formula
    const c8 = it.next().?;
    expect_eq(c8.ref.col, 4);
    expect_eq(c8.ref.row, 1);
    expect_eq(c8.cell_type == XlsxCellType.number, true);
    expect_eq(std.mem.eql(u8, c8.formula, "B1*2"), true);
    expect_eq(approxEq(c8.asF64().?, 246.9), true);

    expect_eq(it.next() == null, true);
}

fn testInvalidRefAndMalformedXmlResilience() {
    // Out-of-bounds & malformed A1 references
    expect_eq(parseCellRef("") == null, true);
    expect_eq(parseCellRef("A") == null, true);
    expect_eq(parseCellRef("1") == null, true);
    expect_eq(parseCellRef("a1") == null, true);          // lowercase rejected
    expect_eq(parseCellRef("A0") == null, true);          // row 0 invalid
    expect_eq(parseCellRef("A01") == null, true);         // leading zero invalid
    expect_eq(parseCellRef("A1B") == null, true);         // trailing letters invalid
    expect_eq(parseCellRef("XFE1") == null, true);        // col 16385 > XFD
    expect_eq(parseCellRef("AAAA1") == null, true);       // > 3 letters
    expect_eq(parseCellRef("A1048577") == null, true);    // row > 1048576

    // Non-XML / truncated XML safety across all iterators
    let s_it = XlsxSheetIter.init("not xml at all {{{");
    expect_eq(s_it.next() == null, true);

    let s_trunc = XlsxSheetIter.init("<workbook><sheets><sheet name=\"Broken");
    expect_eq(s_trunc.next() == null, true);

    let ss_it = XlsxSharedStringIter.init("<sst><si><t>Unclosed");
    expect_eq(ss_it.next() == null, true);

    let c_non = XlsxCellIter.init("{\"json\": true}");
    expect_eq(c_non.next() == null, true);

    let c_empty = XlsxCellIter.init("<worksheet><sheetData/></worksheet>");
    expect_eq(c_empty.next() == null, true);

    let c_trunc = XlsxCellIter.init("<worksheet><sheetData><row><c r=\"A1\"><v>1");
    expect_eq(c_trunc.next() == null, true);
}

test("1. A1/Z10/AA1/XFD1048576 Bidirectional CellRef & Column Name Codec", () => {
    testCellRefAndColNameCodec();
});

test("2. xl/workbook.xml Multi-Sheet & Hidden/VeryHidden State Iteration", () => {
    testWorkbookSheetIteration();
});

test("3. xl/sharedStrings.xml Plain/Rich-Text Concatenation & Entity Unescaping", () => {
    testSharedStringsAndRichTextUnescape();
});

test("4. xl/worksheets/sheet1.xml Streaming Cell Iteration & Type Helpers", () => {
    testWorksheetCellStreaming();
});

test("5. Invalid CellRef Bounds, Non-XML & Truncated XML Resilience", () => {
    testInvalidRefAndMalformedXmlResilience();
});
