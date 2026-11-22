// Origin: Pure-CP Stdlib Test Suite (lib/doc/xlsx.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Upgraded Zero-Heap OOXML XLSX Engine:
//   1. A1/XFD cell reference codec (`encodeCellRef`/`decodeCellRef` with `$` absolute markers) & `parseCellRange`
//   2. Excel 1900 (incl. Lotus 1-2-3 bug serial 60 == 1900-02-29) & 1904 Mac date serial + HH:MM:SS roundtrip
//   3. `buildSharedStringsXml` deduplication + XML entity escaping -> `parseSharedStringsXml` (+ rich-text `<r><t>`)
//   4. Multi-sheet `buildWorkbookXml`/`parseWorkbookSheetsXml` + `parseStylesNumFmtsXml` + `buildWorksheetXml`/`parseWorksheetCellsXml` roundtrip
//   5. Formula evaluator (`evalXlsxFormula`): `SUM`/`AVERAGE`/`MIN`/`MAX`/`COUNT`/`IF`/`ABS`/`ROUND` & nested cell references
//   6. Formula error guards: `#DIV/0!` (`DivisionByZero`), `#REF!` (`InvalidReference`) & `#CYCLE!` (`CycleDetected`)
//   7. Malformed XML/range guards & `cap.doc` namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MAX_XLSX_COL,
    MAX_XLSX_ROW,
    XlsxCellRef,
    XlsxSheetInfo,
    XlsxCellType,
    XlsxError,
    encodeCellRef,
    decodeCellRef,
    parseCellRange,
    XlsxDateTime,
    excelSerialToYmd,
    ymdToExcelSerial,
    parseSharedStringsXml,
    parseWorkbookSheetsXml,
    XlsxNumFmtKind,
    XlsxStyleInfo,
    parseStylesNumFmtsXml,
    XlsxCellValue,
    parseWorksheetCellsXml,
    buildSharedStringsXml,
    buildWorkbookXml,
    buildWorksheetXml,
    evalXlsxFormula,
} from "cap:doc";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

test("XLSX A1/XFD cell coordinate codec ($ absolute markers) & range parser") {
    const r0 = decodeCellRef("$A$1") catch unreachable;
    expect_eq(r0.col, 0);
    expect_eq(r0.row, 0);

    const rmax = decodeCellRef("$XFD$1048576") catch unreachable;
    expect_eq(rmax.col, MAX_XLSX_COL);
    expect_eq(rmax.row, MAX_XLSX_ROW);

    let buf: [16]u8 = [0; 16];
    const n1 = encodeCellRef(rmax, buf[0..]) catch unreachable;
    expect_true(bytesEq(buf[0..n1], "XFD1048576"));

    const rg = parseCellRange("$F$100:B2") catch unreachable;
    expect_eq(rg.start.col, 1);
    expect_eq(rg.start.row, 1);
    expect_eq(rg.end.col, 5);
    expect_eq(rg.end.row, 99);
    expect_eq(rg.cellCount(), 5 * 99);
    expect_true(rg.contains(XlsxCellRef{ col: 3, row: 50 }));
    expect_true(!rg.contains(XlsxCellRef{ col: 0, row: 0 }));
}

test("Excel 1900 (incl. serial 60 == 1900-02-29 bug) & 1904 Mac date serial roundtrip") {
    // 1. 1900 system: serial 60 == 1900-02-29 (Lotus 1-2-3 historical bug)
    const d60 = excelSerialToYmd(60.0, false) catch unreachable;
    expect_eq(d60.year, 1900);
    expect_eq(d60.month, 2);
    expect_eq(d60.day, 29);
    expect_eq(ymdToExcelSerial(d60, false) catch unreachable, 60.0);

    // 2. 1900 system: 2026-11-23 12:30:00 -> serial roundtrip
    const dt = XlsxDateTime{ year: 2026, month: 11, day: 23, hour: 12, minute: 30, second: 0 };
    const ser = ymdToExcelSerial(dt, false) catch unreachable;
    const back = excelSerialToYmd(ser, false) catch unreachable;
    expect_eq(back.year, 2026);
    expect_eq(back.month, 11);
    expect_eq(back.day, 23);
    expect_eq(back.hour, 12);
    expect_eq(back.minute, 30);
    expect_eq(back.second, 0);

    // 3. 1904 Mac system: 1904-01-01 == 0.0, 2026-11-23 == ser - 1462
    const m0 = excelSerialToYmd(0.0, true) catch unreachable;
    expect_eq(m0.year, 1904);
    expect_eq(m0.month, 1);
    expect_eq(m0.day, 1);
    const ser1904 = ymdToExcelSerial(dt, true) catch unreachable;
    expect_true(absF64((ser - ser1904) - 1462.0) < 0.0001);
}

test("SharedStrings deduplication build -> parse roundtrip & rich-text <r><t> decoding") {
    const inputs: [4][]const u8 = ["Revenue & Cost", "Q1", "Revenue & Cost", "<Total>"];
    let unique: [4][]const u8 = [""; 4];
    let indices: [4]u32 = [0; 4];
    let xml_buf: [1024]u8 = [0; 1024];

    const xlen = buildSharedStringsXml(inputs[0..], unique[0..], indices[0..], xml_buf[0..]) catch unreachable;
    expect_eq(indices[0], 0);
    expect_eq(indices[1], 1);
    expect_eq(indices[2], 0); // deduplicated!
    expect_eq(indices[3], 2);

    let parsed_ss: [4][]const u8 = [""; 4];
    let slab: [256]u8 = [0; 256];
    const cnt = parseSharedStringsXml(xml_buf[0..xlen], parsed_ss[0..], slab[0..]) catch unreachable;
    expect_eq(cnt, 3);
    expect_true(bytesEq(parsed_ss[0], "Revenue & Cost"));
    expect_true(bytesEq(parsed_ss[1], "Q1"));
    expect_true(bytesEq(parsed_ss[2], "<Total>"));

    // Rich-text <si><r><t>Hello </t></r><r><t>World &amp; CP</t></r></si>
    const rich_xml = "<sst><si><r><t>Hello </t></r><r><t>World &amp; CP</t></r></si></sst>";
    const rcnt = parseSharedStringsXml(rich_xml, parsed_ss[0..], slab[0..]) catch unreachable;
    expect_eq(rcnt, 1);
    expect_true(bytesEq(parsed_ss[0], "Hello World & CP"));
}

test("Multi-sheet workbook + styles.xml + worksheet build -> parse full roundtrip") {
    // 1. Multi-sheet workbook.xml roundtrip
    const sheet_names: [3][]const u8 = ["Summary", "Q1 & Q2", "Archive"];
    let wb_xml: [1024]u8 = [0; 1024];
    const wblen = buildWorkbookXml(sheet_names[0..], wb_xml[0..]) catch unreachable;
    let out_sheets: [4]XlsxSheetInfo = [XlsxSheetInfo{}; 4];
    const scnt = parseWorkbookSheetsXml(wb_xml[0..wblen], out_sheets[0..]) catch unreachable;
    expect_eq(scnt, 3);
    expect_true(bytesEq(out_sheets[0].name, "Summary"));
    expect_eq(out_sheets[0].sheet_id, 1);
    expect_true(bytesEq(out_sheets[2].name, "Archive"));
    expect_eq(out_sheets[2].sheet_id, 3);

    // 2. styles.xml numFmtId + custom formatCode classification
    const styles_xml = "<styleSheet>"
        ++ "<numFmts count=\"2\">"
        ++ "<numFmt numFmtId=\"164\" formatCode=\"yyyy-mm-dd\"/>"
        ++ "<numFmt numFmtId=\"165\" formatCode=\"$#,##0.00\"/>"
        ++ "</numFmts>"
        ++ "<cellXfs count=\"4\">"
        ++ "<xf numFmtId=\"0\"/>"
        ++ "<xf numFmtId=\"10\"/>"
        ++ "<xf numFmtId=\"164\"/>"
        ++ "<xf numFmtId=\"165\"/>"
        ++ "</cellXfs></styleSheet>";
    let styles: [8]XlsxStyleInfo = [XlsxStyleInfo{}; 8];
    const st_cnt = parseStylesNumFmtsXml(styles_xml, styles[0..]) catch unreachable;
    expect_eq(st_cnt, 4);
    expect_eq(styles[0].kind, XlsxNumFmtKind.general);
    expect_eq(styles[1].kind, XlsxNumFmtKind.percent);
    expect_eq(styles[2].kind, XlsxNumFmtKind.date);
    expect_eq(styles[3].kind, XlsxNumFmtKind.currency);

    // 3. Worksheet build -> parse with shared strings, numbers, bools, styles & formulas
    const ss_tbl: [2][]const u8 = ["Item", "Total"];
    const cells_in: [4]XlsxCellValue = [
        XlsxCellValue{ ref: XlsxCellRef{ col: 0, row: 0 }, cell_type: XlsxCellType.shared_string, text: "Item" },
        XlsxCellValue{ ref: XlsxCellRef{ col: 1, row: 0 }, cell_type: XlsxCellType.number, style_idx: 3, num_val: 42.5, has_num: true },
        XlsxCellValue{ ref: XlsxCellRef{ col: 0, row: 1 }, cell_type: XlsxCellType.bool_val, bool_val: true },
        XlsxCellValue{ ref: XlsxCellRef{ col: 1, row: 1 }, cell_type: XlsxCellType.number, num_val: 85.0, has_num: true, formula: "B1*2" },
    ];
    let sh_xml: [2048]u8 = [0; 2048];
    const sh_len = buildWorksheetXml(cells_in[0..], ss_tbl[0..], sh_xml[0..]) catch unreachable;

    let cells_out: [8]XlsxCellValue = [XlsxCellValue{}; 8];
    const ccnt = parseWorksheetCellsXml(sh_xml[0..sh_len], ss_tbl[0..], cells_out[0..]) catch unreachable;
    expect_eq(ccnt, 4);
    expect_true(bytesEq(cells_out[0].text, "Item"));
    expect_eq(cells_out[1].style_idx, 3);
    expect_true(absF64(cells_out[1].num_val - 42.5) < 0.0001);
    expect_true(cells_out[2].bool_val);
    expect_true(bytesEq(cells_out[3].formula, "B1*2"));
    expect_true(absF64(cells_out[3].num_val - 85.0) < 0.0001);
}

test("Formula evaluator: SUM, AVERAGE, MIN, MAX, COUNT, IF, ABS, ROUND & cell chains") {
    // A1=10, A2=20, A3=30, B1="=SUM(A1:A3)" (60), B2="=B1/3" (20)
    const sheet: [5]XlsxCellValue = [
        XlsxCellValue{ ref: XlsxCellRef{ col: 0, row: 0 }, num_val: 10.0, has_num: true },
        XlsxCellValue{ ref: XlsxCellRef{ col: 0, row: 1 }, num_val: 20.0, has_num: true },
        XlsxCellValue{ ref: XlsxCellRef{ col: 0, row: 2 }, num_val: 30.0, has_num: true },
        XlsxCellValue{ ref: XlsxCellRef{ col: 1, row: 0 }, formula: "=SUM(A1:A3)" },
        XlsxCellValue{ ref: XlsxCellRef{ col: 1, row: 1 }, formula: "=B1/3" },
    ];

    expect_eq(evalXlsxFormula("=SUM($A$1:$A$3) + 5", sheet[0..]) catch unreachable, 65.0);
    expect_eq(evalXlsxFormula("=AVERAGE(A1:A3)", sheet[0..]) catch unreachable, 20.0);
    expect_eq(evalXlsxFormula("=MIN(A1:A3)", sheet[0..]) catch unreachable, 10.0);
    expect_eq(evalXlsxFormula("=MAX(A1:A3, B1)", sheet[0..]) catch unreachable, 60.0);
    expect_eq(evalXlsxFormula("=COUNT(A1:A3)", sheet[0..]) catch unreachable, 3.0);
    expect_eq(evalXlsxFormula("=B2 * 2 + 1", sheet[0..]) catch unreachable, 41.0);
    expect_eq(evalXlsxFormula("=IF(B1>=60, ABS(-12.345), 0)", sheet[0..]) catch unreachable, 12.345);
    expect_true(absF64((evalXlsxFormula("=ROUND(3.14159, 2)", sheet[0..]) catch unreachable) - 3.14) < 0.0001);
}

test("Formula error guards: #DIV/0!, #REF! & circular reference #CYCLE!") {
    // A1 -> "=B1+1", B1 -> "=A1+1" (mutual cycle!)
    const cyclic: [2]XlsxCellValue = [
        XlsxCellValue{ ref: XlsxCellRef{ col: 0, row: 0 }, formula: "=B1+1" },
        XlsxCellValue{ ref: XlsxCellRef{ col: 1, row: 0 }, formula: "=A1+1" },
    ];
    let got_cycle: bool = false;
    * = evalXlsxFormula("=A1", cyclic[0..]) catch |e| {
        if (e == XlsxError.CycleDetected) got_cycle = true;
    };
    expect_true(got_cycle);

    let got_div0: bool = false;
    * = evalXlsxFormula("=10 / (5 - 5)", cyclic[0..]) catch |e| {
        if (e == XlsxError.DivisionByZero) got_div0 = true;
    };
    expect_true(got_div0);

    let got_ref: bool = false;
    * = evalXlsxFormula("=ZZZZ9999999", cyclic[0..]) catch |e| {
        if (e == XlsxError.InvalidReference) got_ref = true;
    };
    expect_true(got_ref);
}

test("XLSX malformed guards & cap.doc namespace parity") {
    let got_bad_range: bool = false;
    * = parseCellRange("INVALID") catch |e| {
        if (e == XlsxError.InvalidRange) got_bad_range = true;
    };
    expect_true(got_bad_range);

    let got_bad_date: bool = false;
    * = excelSerialToYmd(-5.0, false) catch |e| {
        if (e == XlsxError.InvalidDate) got_bad_date = true;
    };
    expect_true(got_bad_date);

    // cap.doc namespace parity
    const dt60 = cap.doc.excelSerialToYmd(60.0, false) catch unreachable;
    expect_eq(dt60.year, 1900);
    expect_eq(dt60.month, 2);
    expect_eq(dt60.day, 29);
}
