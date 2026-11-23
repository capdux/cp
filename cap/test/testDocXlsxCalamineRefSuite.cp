// Calamine (MIT) Reference-Hardened XLSX Regression Suite
// Ports 26 real-world Excel/OOXML edge-case tests from tafia/calamine.

const std = @import("std");
import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    MAX_XLSX_COL,
    MAX_XLSX_ROW,
    XlsxCellRef,
    XlsxSheetVisibility,
    XlsxSheetInfo,
    XlsxCellType,
    XlsxCellErrorKind,
    parseXlsxCellError,
    formatXlsxCellError,
    XlsxCell,
    XlsxSheetIter,
    XlsxSharedStringIter,
    XlsxCellIter,
    parseCellRef,
    decodeExcelEscapes,
    extractSharedString,
    extractInlineString,
    XlsxError,
    encodeCellRef,
    decodeCellRef,
    XlsxRangeRef,
    parseCellRange,
    excelSerialToYmd,
    ymdToExcelSerial,
    parseWorkbookSheetsXml,
    XlsxNumFmtKind,
    XlsxStyleInfo,
    isBuiltinExcelDateFmt,
    classifyExcelNumFmt,
    parseMergedCellsXml,
    findMergedCellMaster,
    parseStylesNumFmtsXml,
    XlsxCellValue,
    parseWorksheetCellsXml,
    buildWorksheetXml,
} from "../../lib/doc/index.cp";

fn assertEscapeDecodes(input: []const u8, expected: []const u8) {
    let buf: [256]u8 = [0; 256];
    const n = decodeExcelEscapes(input, buf[0..]) ?? 999999;
    expect_true(n == expected.len);
    if (n == expected.len) {
        expect_true(std.mem.eql(u8, buf[0..n], expected));
    }
}

test("calamine_ref_01_cell_error_enum_parse_and_format_all_8") {
    expect_eq(parseXlsxCellError("#DIV/0!"), XlsxCellErrorKind.div0);
    expect_eq(parseXlsxCellError("#N/A"), XlsxCellErrorKind.na);
    expect_eq(parseXlsxCellError("#NAME?"), XlsxCellErrorKind.name);
    expect_eq(parseXlsxCellError("#NULL!"), XlsxCellErrorKind.null_err);
    expect_eq(parseXlsxCellError("#NUM!"), XlsxCellErrorKind.num);
    expect_eq(parseXlsxCellError("#REF!"), XlsxCellErrorKind.ref_err);
    expect_eq(parseXlsxCellError("#VALUE!"), XlsxCellErrorKind.value);
    expect_eq(parseXlsxCellError("#GETTING_DATA"), XlsxCellErrorKind.getting_data);
    expect_eq(parseXlsxCellError("#UNKNOWN!"), XlsxCellErrorKind.none);
    expect_eq(parseXlsxCellError(""), XlsxCellErrorKind.none);

    expect_true(std.mem.eql(u8, formatXlsxCellError(XlsxCellErrorKind.div0), "#DIV/0!"));
    expect_true(std.mem.eql(u8, formatXlsxCellError(XlsxCellErrorKind.na), "#N/A"));
    expect_true(std.mem.eql(u8, formatXlsxCellError(XlsxCellErrorKind.name), "#NAME?"));
    expect_true(std.mem.eql(u8, formatXlsxCellError(XlsxCellErrorKind.null_err), "#NULL!"));
    expect_true(std.mem.eql(u8, formatXlsxCellError(XlsxCellErrorKind.num), "#NUM!"));
    expect_true(std.mem.eql(u8, formatXlsxCellError(XlsxCellErrorKind.ref_err), "#REF!"));
    expect_true(std.mem.eql(u8, formatXlsxCellError(XlsxCellErrorKind.value), "#VALUE!"));
    expect_true(std.mem.eql(u8, formatXlsxCellError(XlsxCellErrorKind.getting_data), "#GETTING_DATA"));
    expect_true(std.mem.eql(u8, formatXlsxCellError(XlsxCellErrorKind.none), ""));
}

test("calamine_ref_02_worksheet_error_cells_and_roundtrip") {
    const xml =
        "<worksheet><sheetData><row r=\"1\">" ++
        "<c r=\"A1\" t=\"e\"><v>#DIV/0!</v></c>" ++
        "<c r=\"B1\" t=\"e\"><v>#N/A</v></c>" ++
        "<c r=\"C1\" t=\"e\"><v>#NAME?</v></c>" ++
        "<c r=\"D1\" t=\"e\"><v>#NULL!</v></c>" ++
        "<c r=\"E1\" t=\"e\"><v>#NUM!</v></c>" ++
        "<c r=\"F1\" t=\"e\"><v>#REF!</v></c>" ++
        "<c r=\"G1\" t=\"e\"><v>#VALUE!</v></c>" ++
        "<c r=\"H1\" t=\"e\"><v>#GETTING_DATA</v></c>" ++
        "</row></sheetData></worksheet>";

    let empty_ss: [1][]const u8 = [""];
    let cells: [16]XlsxCellValue = [XlsxCellValue{}; 16];
    const n = parseWorksheetCellsXml(xml, empty_ss[0..0], cells[0..]) catch 0;
    expect_eq(n, 8);
    expect_eq(cells[0].error_kind, XlsxCellErrorKind.div0);
    expect_eq(cells[1].error_kind, XlsxCellErrorKind.na);
    expect_eq(cells[2].error_kind, XlsxCellErrorKind.name);
    expect_eq(cells[3].error_kind, XlsxCellErrorKind.null_err);
    expect_eq(cells[4].error_kind, XlsxCellErrorKind.num);
    expect_eq(cells[5].error_kind, XlsxCellErrorKind.ref_err);
    expect_eq(cells[6].error_kind, XlsxCellErrorKind.value);
    expect_eq(cells[7].error_kind, XlsxCellErrorKind.getting_data);

    // Roundtrip serialize via buildWorksheetXml
    let out_xml: [2048]u8 = [0; 2048];
    const written = buildWorksheetXml(cells[0..n], empty_ss[0..0], out_xml[0..]) catch 0;
    expect_true(written > 0);
    let cells2: [16]XlsxCellValue = [XlsxCellValue{}; 16];
    const n2 = parseWorksheetCellsXml(out_xml[0..written], empty_ss[0..0], cells2[0..]) catch 0;
    expect_eq(n2, 8);
    expect_eq(cells2[0].error_kind, XlsxCellErrorKind.div0);
    expect_eq(cells2[7].error_kind, XlsxCellErrorKind.getting_data);
}

test("calamine_ref_03_omitted_cell_r_auto_col_increment") {
    // LibreOffice / Google Sheets frequently omit r="..." on <c>
    const xml =
        "<worksheet><sheetData><row r=\"1\">" ++
        "<c><v>10</v></c>" ++
        "<c><v>20</v></c>" ++
        "<c><v>30</v></c>" ++
        "</row></sheetData></worksheet>";

    let it = XlsxCellIter.init(xml);
    const c0 = it.next().?;
    expect_eq(c0.ref.row, 0);
    expect_eq(c0.ref.col, 0);
    expect_true(std.mem.eql(u8, c0.raw_value, "10"));

    const c1 = it.next().?;
    expect_eq(c1.ref.row, 0);
    expect_eq(c1.ref.col, 1);
    expect_true(std.mem.eql(u8, c1.raw_value, "20"));

    const c2 = it.next().?;
    expect_eq(c2.ref.row, 0);
    expect_eq(c2.ref.col, 2);
    expect_true(std.mem.eql(u8, c2.raw_value, "30"));

    expect_true(it.next() == null);
}

test("calamine_ref_04_omitted_row_r_and_mixed_explicit_r") {
    const xml =
        "<worksheet><sheetData>" ++
        "<row><c><v>1</v></c><c><v>2</v></c></row>" ++
        "<row><c r=\"C2\"><v>3</v></c><c><v>4</v></c></row>" ++
        "<row r=\"5\"><c><v>5</v></c></row>" ++
        "<row><c><v>6</v></c></row>" ++
        "</sheetData></worksheet>";

    let it = XlsxCellIter.init(xml);
    const c0 = it.next().?;
    expect_eq(c0.ref.row, 0);
    expect_eq(c0.ref.col, 0);

    const c1 = it.next().?;
    expect_eq(c1.ref.row, 0);
    expect_eq(c1.ref.col, 1);

    const c2 = it.next().?;
    expect_eq(c2.ref.row, 1);
    expect_eq(c2.ref.col, 2); // C2

    const c3 = it.next().?;
    expect_eq(c3.ref.row, 1);
    expect_eq(c3.ref.col, 3); // D2 auto-increment after C2

    const c4 = it.next().?;
    expect_eq(c4.ref.row, 4); // row r="5" -> 0-based row 4
    expect_eq(c4.ref.col, 0); // A5

    const c5 = it.next().?;
    expect_eq(c5.ref.row, 5); // omitted row after r="5" -> row 5 (A6)
    expect_eq(c5.ref.col, 0);

    expect_true(it.next() == null);
}

test("calamine_ref_05_merged_cells_parse_and_master_lookup") {
    const xml =
        "<worksheet><sheetData><row r=\"1\"><c r=\"A1\"><v>1</v></c></row></sheetData>" ++
        "<mergeCells count=\"3\">" ++
        "<mergeCell ref=\"A1:C3\"/>" ++
        "<mergeCell ref=\"E2:F4\"/>" ++
        "<mergeCell ref=\"H8\"/>" ++
        "</mergeCells></worksheet>";

    let ranges: [8]XlsxRangeRef = [XlsxRangeRef{}; 8];
    const n = parseMergedCellsXml(xml, ranges[0..]) catch 0;
    expect_eq(n, 3);
    expect_eq(ranges[0].start.col, 0);
    expect_eq(ranges[0].start.row, 0);
    expect_eq(ranges[0].end.col, 2);
    expect_eq(ranges[0].end.row, 2);

    // Interior cell B2 maps to master A1 (0, 0)
    const m_b2 = findMergedCellMaster(ranges[0..n], XlsxCellRef{ col: 1, row: 1 }).?;
    expect_eq(m_b2.col, 0);
    expect_eq(m_b2.row, 0);

    // Corner cell F4 maps to master E2 (4, 1)
    const m_f4 = findMergedCellMaster(ranges[0..n], XlsxCellRef{ col: 5, row: 3 }).?;
    expect_eq(m_f4.col, 4);
    expect_eq(m_f4.row, 1);

    // Single-cell degenerate merge H8 -> H8 (7, 7)
    const m_h8 = findMergedCellMaster(ranges[0..n], XlsxCellRef{ col: 7, row: 7 }).?;
    expect_eq(m_h8.col, 7);
    expect_eq(m_h8.row, 7);

    // Outside cell D1 -> null
    expect_true(findMergedCellMaster(ranges[0..n], XlsxCellRef{ col: 3, row: 0 }) == null);
}

test("calamine_ref_06_merged_cells_empty_and_overflow_guard") {
    const empty_xml = "<worksheet><sheetData/><mergeCells count=\"0\"/></worksheet>";
    let ranges: [2]XlsxRangeRef = [XlsxRangeRef{}; 2];
    const n0 = parseMergedCellsXml(empty_xml, ranges[0..]) catch 99;
    expect_eq(n0, 0);

    const overflow_xml =
        "<worksheet><mergeCells count=\"3\">" ++
        "<mergeCell ref=\"A1:B2\"/><mergeCell ref=\"C1:D2\"/><mergeCell ref=\"E1:F2\"/>" ++
        "</mergeCells></worksheet>";
    let overflowed = false;
    * = parseMergedCellsXml(overflow_xml, ranges[0..]) catch {
        overflowed = true;
    };
    expect_true(overflowed);
}

test("calamine_ref_07_shared_formulas_and_cached_values") {
    const xml =
        "<worksheet><sheetData>" ++
        "<row r=\"1\"><c r=\"C1\"><f t=\"shared\" ref=\"C1:C3\" si=\"0\">A1+B1</f><v>15</v></c></row>" ++
        "<row r=\"2\"><c r=\"C2\"><f t=\"shared\" si=\"0\"/><v>35</v></c></row>" ++
        "<row r=\"3\"><c r=\"C3\"><f t=\"shared\" si=\"0\"/><v>55</v></c></row>" ++
        "</sheetData></worksheet>";

    let it = XlsxCellIter.init(xml);
    const c1 = it.next().?;
    expect_true(c1.is_shared_formula);
    expect_eq(c1.shared_formula_idx, 0);
    expect_true(std.mem.eql(u8, c1.shared_formula_ref, "C1:C3"));
    expect_true(std.mem.eql(u8, c1.formula, "A1+B1"));
    expect_eq(c1.asF64().?, 15.0);

    const c2 = it.next().?;
    expect_true(c2.is_shared_formula);
    expect_eq(c2.shared_formula_idx, 0);
    expect_eq(c2.formula.len, 0);
    expect_eq(c2.asF64().?, 35.0);

    const c3 = it.next().?;
    expect_true(c3.is_shared_formula);
    expect_eq(c3.shared_formula_idx, 0);
    expect_eq(c3.asF64().?, 55.0);
}

test("calamine_ref_08_inline_str_rich_text_and_redundant_v_ignored") {
    const xml =
        "<worksheet><sheetData><row r=\"1\">" ++
        "<c r=\"A1\" t=\"inlineStr\"><is><r><t xml:space=\"preserve\">Hello </t></r><r><rPr><b/></rPr><t>World &amp; Co</t></r></is><v>IGNORED</v></c>" ++
        "<c r=\"B1\" t=\"is\"><is><t xml:space=\"preserve\">  spaced  </t></is></c>" ++
        "</row></sheetData></worksheet>";

    let it = XlsxCellIter.init(xml);
    const c1 = it.next().?;
    expect_eq(c1.cell_type, XlsxCellType.inline_string);
    let out1: [128]u8 = [0; 128];
    const n1 = extractInlineString(c1.inline_xml, out1[0..]).?;
    expect_true(std.mem.eql(u8, out1[0..n1], "Hello World & Co"));

    const c2 = it.next().?;
    expect_eq(c2.cell_type, XlsxCellType.inline_string);
    let out2: [128]u8 = [0; 128];
    const n2 = extractInlineString(c2.inline_xml, out2[0..]).?;
    expect_true(std.mem.eql(u8, out2[0..n2], "  spaced  "));
}

test("calamine_ref_09_iso_date_cell_type_and_roundtrip") {
    const xml =
        "<worksheet><sheetData><row r=\"1\">" ++
        "<c r=\"A1\" t=\"d\"><v>2024-06-15T12:30:00Z</v></c>" ++
        "</row></sheetData></worksheet>";

    let empty_ss: [1][]const u8 = [""];
    let cells: [4]XlsxCellValue = [XlsxCellValue{}; 4];
    const n = parseWorksheetCellsXml(xml, empty_ss[0..0], cells[0..]) catch 0;
    expect_eq(n, 1);
    expect_eq(cells[0].cell_type, XlsxCellType.iso_date);
    expect_true(std.mem.eql(u8, cells[0].text, "2024-06-15T12:30:00Z"));

    let out_xml: [512]u8 = [0; 512];
    const written = buildWorksheetXml(cells[0..1], empty_ss[0..0], out_xml[0..]) catch 0;
    expect_true(written > 0);
    let cells2: [4]XlsxCellValue = [XlsxCellValue{}; 4];
    const n2 = parseWorksheetCellsXml(out_xml[0..written], empty_ss[0..0], cells2[0..]) catch 0;
    expect_eq(n2, 1);
    expect_eq(cells2[0].cell_type, XlsxCellType.iso_date);
    expect_true(std.mem.eql(u8, cells2[0].text, "2024-06-15T12:30:00Z"));
}

test("calamine_ref_10_excel_escapes_basic_and_control_chars") {
    assertEscapeDecodes("_x000D_", "\r");
    assertEscapeDecodes("_x000A_", "\n");
    assertEscapeDecodes("_x000a_", "\n");
    assertEscapeDecodes("_x005F_", "_");
    assertEscapeDecodes("_x001F_", "\x1F");
    assertEscapeDecodes("Hello_x000D_World", "Hello\rWorld");
    assertEscapeDecodes("Control_x001F_Char", "Control\x1FChar");
    assertEscapeDecodes("Hello_x000D_World_x000D__x000D_", "Hello\rWorld\r\r");
    assertEscapeDecodes("Multiple_x000D__x000D__x000D_Chars", "Multiple\r\r\rChars");
}

test("calamine_ref_11_excel_escapes_double_escape_x005F_guard") {
    // _x005F_ escapes a literal '_' so the following xHHHH_ is NOT decoded again
    assertEscapeDecodes("_x005F_x0000_", "_x0000_");
    assertEscapeDecodes("_x005F_x000a_", "_x000a_");
    assertEscapeDecodes("_x005F_x000A_", "_x000A_");
    assertEscapeDecodes("_x005F_x005F_", "_x005F_");
    assertEscapeDecodes("_x005F_x597D_", "_x597D_");
    assertEscapeDecodes("__x005F_x0000__", "__x0000__");
    assertEscapeDecodes("test_x005F_x0000_end", "test_x0000_end");
    assertEscapeDecodes("_x005F_x2764_", "_x2764_");
}

test("calamine_ref_12_excel_escapes_malformed_and_passthrough") {
    assertEscapeDecodes("_", "_");
    assertEscapeDecodes("_x", "_x");
    assertEscapeDecodes("_x0", "_x0");
    assertEscapeDecodes("_x00", "_x00");
    assertEscapeDecodes("_x000", "_x000");
    assertEscapeDecodes("_x000D", "_x000D");
    assertEscapeDecodes("_x00ZZ_", "_x00ZZ_");
    assertEscapeDecodes("_x_x_x", "_x_x_x");
    assertEscapeDecodes("_x000G_", "_x000G_");
    assertEscapeDecodes("_x+123_", "_x+123_");
    assertEscapeDecodes("_x00+1_", "_x00+1_");
    assertEscapeDecodes("Just_a_normal_string", "Just_a_normal_string");
    assertEscapeDecodes("/path/to/kernel32_x64d8.pyd", "/path/to/kernel32_x64d8.pyd");
    assertEscapeDecodes("test_xGGGG_end", "test_xGGGG_end");
}

test("calamine_ref_13_excel_escapes_unicode_bmp_and_surrogate_pairs") {
    // BMP characters above U+00FF
    assertEscapeDecodes("_x00E9_", "\xC3\xA9");          // U+00E9 'é'
    assertEscapeDecodes("_x2764_", "\xE2\x9D\xA4");      // U+2764 '❤'
    assertEscapeDecodes("_x597d_", "\xE5\xA5\xBD");      // U+597D '好'
    // UTF-16 surrogate pairs -> 4-byte UTF-8
    assertEscapeDecodes("_xD83D__xDCF7_", "\xF0\x9F\x93\xB7");   // U+1F4F7 camera
    assertEscapeDecodes("_xd83d__xdcf7_", "\xF0\x9F\x93\xB7");   // lowercase hex
    assertEscapeDecodes("a_xD83D__xDE00_b", "a\xF0\x9F\x98\x80b"); // U+1F600 grinning face
}

test("calamine_ref_14_excel_escapes_unpaired_surrogates_to_fffd") {
    // Unpaired high or low surrogates must yield U+FFFD (\xEF\xBF\xBD)
    assertEscapeDecodes("_xD83D_", "\xEF\xBF\xBD");
    assertEscapeDecodes("_xDCF7_", "\xEF\xBF\xBD");
    assertEscapeDecodes("_xDCF7__xD83D_", "\xEF\xBF\xBD\xEF\xBF\xBD");
    assertEscapeDecodes("_xD83D__xD83D__xDCF7_", "\xEF\xBF\xBD\xF0\x9F\x93\xB7");
    assertEscapeDecodes("_xD83D_x", "\xEF\xBF\xBDx");
    assertEscapeDecodes("_xD83D__x0041_", "\xEF\xBF\xBDA");
    assertEscapeDecodes("_x005F_xD83D__xDCF7_", "_xD83D_\xEF\xBF\xBD");
}

test("calamine_ref_15_shared_string_combines_xml_entities_and_excel_escapes") {
    const si_xml = "<t xml:space=\"preserve\"> Line1 &amp; _x000D__x000A_Line2 _x005F_x000D_ </t>";
    let out: [128]u8 = [0; 128];
    const n = extractSharedString(si_xml, out[0..]).?;
    expect_true(std.mem.eql(u8, out[0..n], " Line1 & \r\nLine2 _x000D_ "));
}

test("calamine_ref_16_shared_string_rich_text_skips_phonetic_rPh") {
    const si_xml =
        "<r><t xml:space=\"preserve\">東京</t></r>" ++
        "<r><t>都</t></r>" ++
        "<rPh sb=\"0\" eb=\"2\"><t>トウキョウ</t></rPh>";
    let out: [128]u8 = [0; 128];
    const n = extractSharedString(si_xml, out[0..]).?;
    expect_true(std.mem.eql(u8, out[0..n], "東京都"));
}

test("calamine_ref_17_sheet_visibility_visible_hidden_veryHidden") {
    const wb_xml =
        "<workbook><sheets>" ++
        "<sheet name=\"Main\" sheetId=\"1\" r:id=\"rId1\"/>" ++
        "<sheet name=\"ExplicitVis\" sheetId=\"2\" r:id=\"rId2\" state=\"visible\"/>" ++
        "<sheet name=\"Secret\" sheetId=\"3\" r:id=\"rId3\" state=\"hidden\"/>" ++
        "<sheet name=\"UltraSecret\" sheetId=\"4\" r:id=\"rId4\" state=\"veryHidden\"/>" ++
        "</sheets></workbook>";

    let sheets: [8]XlsxSheetInfo = [XlsxSheetInfo{}; 8];
    const n = parseWorkbookSheetsXml(wb_xml, sheets[0..]) catch 0;
    expect_eq(n, 4);

    expect_eq(sheets[0].visibility, XlsxSheetVisibility.visible);
    expect_false(sheets[0].is_hidden);

    expect_eq(sheets[1].visibility, XlsxSheetVisibility.visible);
    expect_false(sheets[1].is_hidden);

    expect_eq(sheets[2].visibility, XlsxSheetVisibility.hidden);
    expect_true(sheets[2].is_hidden);

    expect_eq(sheets[3].visibility, XlsxSheetVisibility.very_hidden);
    expect_true(sheets[3].is_hidden);
}

test("calamine_ref_18_builtin_date_format_ids") {
    // Standard date/time IDs 14..=22, East Asian 27..=36 & 50..=58, duration 45..=47
    expect_true(isBuiltinExcelDateFmt(14));
    expect_true(isBuiltinExcelDateFmt(22));
    expect_true(isBuiltinExcelDateFmt(27));
    expect_true(isBuiltinExcelDateFmt(36));
    expect_true(isBuiltinExcelDateFmt(45));
    expect_true(isBuiltinExcelDateFmt(46));
    expect_true(isBuiltinExcelDateFmt(47));
    expect_true(isBuiltinExcelDateFmt(50));
    expect_true(isBuiltinExcelDateFmt(58));

    expect_false(isBuiltinExcelDateFmt(0));
    expect_false(isBuiltinExcelDateFmt(1));
    expect_false(isBuiltinExcelDateFmt(13));
    expect_false(isBuiltinExcelDateFmt(23));
    expect_false(isBuiltinExcelDateFmt(44));
    expect_false(isBuiltinExcelDateFmt(48));
    expect_false(isBuiltinExcelDateFmt(49));
    expect_false(isBuiltinExcelDateFmt(164));

    expect_eq(classifyExcelNumFmt(14, ""), XlsxNumFmtKind.date);
    expect_eq(classifyExcelNumFmt(20, ""), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(22, ""), XlsxNumFmtKind.datetime);
    expect_eq(classifyExcelNumFmt(46, ""), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(30, ""), XlsxNumFmtKind.datetime);
}

test("calamine_ref_19_custom_numfmt_date_and_time_positives") {
    expect_eq(classifyExcelNumFmt(164, "DD/MM/YY"), XlsxNumFmtKind.date);
    expect_eq(classifyExcelNumFmt(165, "H:MM:SS;@"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(166, "m\"M\"d\"D\";@"), XlsxNumFmtKind.date);
    expect_eq(classifyExcelNumFmt(167, "yyyy-mm-dd hh:mm:ss"), XlsxNumFmtKind.datetime);
    expect_eq(classifyExcelNumFmt(168, "ha/p\\\\m"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(169, "*-yyyy-mm-dd"), XlsxNumFmtKind.date);
}

test("calamine_ref_20_custom_numfmt_elapsed_duration_brackets") {
    expect_eq(classifyExcelNumFmt(164, "[h]:mm:ss"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(165, "[h]"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(166, "[ss]"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(167, "[s].000"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(168, "[m]"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(169, "[mm]"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(170, "[Blue]\\+[h]:mm;[Red]\\-[h]:mm;[Green][h]:mm"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(171, "[>=100][Magenta][s].00"), XlsxNumFmtKind.time);
    expect_eq(classifyExcelNumFmt(172, "[h]:mm;[=0]\\-"), XlsxNumFmtKind.time);
}

test("calamine_ref_21_custom_numfmt_quoted_and_escaped_false_positive_guards") {
    // None of these contain actual date/time tokens — all d/m/y/h/s are inside quotes,
    // color brackets, or escaped by \, _, or *!
    expect_eq(classifyExcelNumFmt(164, "\"Y: \"0.00\"m\";\"Y: \"-0.00\"m\";@"), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(165, "#,##0.0####\" YMD\""), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(166, "\"$\"#,##0_);[Red](\"$\"#,##0)"), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(167, "0_ ;[Red]\\-0\\ "), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(168, "\\Y000000"), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(169, "#,##0.00\\ _M\"H\"_);[Red]#,##0.00\\ _M\"S\"_)"), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(170, "#,##0*y"), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(171, "0\"x\"*d"), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(172, "[>=100][Magenta].00"), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(173, "[>=100][Magenta]General"), XlsxNumFmtKind.custom);
    expect_eq(classifyExcelNumFmt(174, "\"USD\" #,##0.00"), XlsxNumFmtKind.custom);
}

test("calamine_ref_22_styles_xml_custom_numfmt_classification_e2e") {
    const styles_xml =
        "<styleSheet>" ++
        "<numFmts count=\"3\">" ++
        "<numFmt numFmtId=\"164\" formatCode=\"&quot;USD&quot; #,##0.00\"/>" ++
        "<numFmt numFmtId=\"165\" formatCode=\"[Red]yyyy-mm-dd\"/>" ++
        "<numFmt numFmtId=\"166\" formatCode=\"[h]:mm:ss\"/>" ++
        "</numFmts>" ++
        "<cellXfs count=\"4\">" ++
        "<xf numFmtId=\"0\"/>" ++
        "<xf numFmtId=\"164\"/>" ++
        "<xf numFmtId=\"165\"/>" ++
        "<xf numFmtId=\"166\"/>" ++
        "</cellXfs></styleSheet>";

    let styles: [8]XlsxStyleInfo = [XlsxStyleInfo{}; 8];
    const n = parseStylesNumFmtsXml(styles_xml, styles[0..]) catch 0;
    expect_eq(n, 4);
    expect_eq(styles[0].kind, XlsxNumFmtKind.general);
    // XML entity &quot; is literal text in attribute slice; verify date/time styles classify accurately
    expect_eq(styles[2].kind, XlsxNumFmtKind.date);
    expect_eq(styles[3].kind, XlsxNumFmtKind.time);
}

test("calamine_ref_23_excel_1900_leap_bug_and_1904_date_system") {
    // Serial 60 is the infamous Lotus 1-2-3 / Excel 1900-02-29 bug
    const dt60 = excelSerialToYmd(60.0, false) catch unreachable;
    expect_eq(dt60.year, 1900);
    expect_eq(dt60.month, 2);
    expect_eq(dt60.day, 29);

    // Serial 61 is 1900-03-01
    const dt61 = excelSerialToYmd(61.0, false) catch unreachable;
    expect_eq(dt61.year, 1900);
    expect_eq(dt61.month, 3);
    expect_eq(dt61.day, 1);

    // 1904 Mac date system: serial 0 is 1904-01-01
    const dt1904 = excelSerialToYmd(0.0, true) catch unreachable;
    expect_eq(dt1904.year, 1904);
    expect_eq(dt1904.month, 1);
    expect_eq(dt1904.day, 1);

    const ser_rt = ymdToExcelSerial(2024, 2, 29, false) catch 0.0;
    const dt_rt = excelSerialToYmd(ser_rt, false) catch unreachable;
    expect_eq(dt_rt.year, 2024);
    expect_eq(dt_rt.month, 2);
    expect_eq(dt_rt.day, 29);
}

test("calamine_ref_24_extreme_cell_ref_bounds_and_malformed_skip") {
    // Max legal coordinate XFD1048576
    const max_ref = parseCellRef("XFD1048576").?;
    expect_eq(max_ref.col, MAX_XLSX_COL);
    expect_eq(max_ref.row, MAX_XLSX_ROW);

    let ref_buf: [16]u8 = [0; 16];
    const rlen = encodeCellRef(max_ref, ref_buf[0..]) catch 0;
    expect_true(std.mem.eql(u8, ref_buf[0..rlen], "XFD1048576"));

    // Out-of-bounds XFE1 and A1048577 must fail
    expect_true(parseCellRef("XFE1") == null);
    expect_true(parseCellRef("A1048577") == null);
    expect_true(parseCellRef("A0") == null);

    // Explicit malformed r="!!!" is skipped while subsequent valid cell is preserved
    const xml =
        "<worksheet><sheetData><row r=\"1\">" ++
        "<c r=\"!!!\"><v>999</v></c>" ++
        "<c r=\"B1\"><v>42</v></c>" ++
        "</row></sheetData></worksheet>";
    let it = XlsxCellIter.init(xml);
    const c = it.next().?;
    expect_eq(c.ref.col, 1);
    expect_eq(c.ref.row, 0);
    expect_eq(c.asF64().?, 42.0);
    expect_true(it.next() == null);
}

test("calamine_ref_25_decode_excel_escapes_buffer_too_small_guard") {
    let tiny: [3]u8 = [0; 3];
    // "Hello" needs 5 bytes -> null
    expect_true(decodeExcelEscapes("Hello", tiny[0..]) == null);
    // Surrogate pair produces 4 UTF-8 bytes -> null in 3-byte buffer
    expect_true(decodeExcelEscapes("_xD83D__xDCF7_", tiny[0..]) == null);
    // Single BMP U+2764 produces 3 UTF-8 bytes -> fits in 3-byte buffer
    expect_eq(decodeExcelEscapes("_x2764_", tiny[0..]).?, 3);
}

test("calamine_ref_26_boolean_true_false_variants_and_formula_str") {
    const xml =
        "<worksheet><sheetData><row r=\"1\">" ++
        "<c r=\"A1\" t=\"b\"><v>1</v></c>" ++
        "<c r=\"B1\" t=\"b\"><v>0</v></c>" ++
        "<c r=\"C1\" t=\"b\"><v>TRUE</v></c>" ++
        "<c r=\"D1\" t=\"b\"><v>false</v></c>" ++
        "<c r=\"E1\" t=\"str\"><f>\"hi\"&amp;\"!\"</f><v>hi!</v></c>" ++
        "</row></sheetData></worksheet>";

    let it = XlsxCellIter.init(xml);
    expect_true(it.next().?.asBool().?);
    expect_false(it.next().?.asBool().?);
    expect_true(it.next().?.asBool().?);
    expect_false(it.next().?.asBool().?);
    const ce = it.next().?;
    expect_eq(ce.cell_type, XlsxCellType.formula_str);
    expect_true(std.mem.eql(u8, ce.raw_value, "hi!"));
}
