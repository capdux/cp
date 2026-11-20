// Source: cp/cap/test/test_sheet_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:sheet & cap.sheet

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SheetError,
    CellCoord,
    CellValue,
    Spreadsheet,
} from "cap:sheet";

fn approxEq(a: f64, b: f64): bool {
    return @abs(a - b) < 0.000000001;
}

fn expectCellNum(sheet: Spreadsheet, ref_str: string, expected: f64): !void {
    const opt = sheet.getNumber(ref_str)?;
    const val = opt.?;
    expect_eq(approxEq(val, expected), true);
}

test("1. CellCoord parseRef/formatRef Round-Trip & InvalidCellRef Guards") {
    const alloc = std.testing.allocator;

    const c1 = CellCoord.parseRef("A1")?;
    expect_eq(c1.row, 0 as usize);
    expect_eq(c1.col, 0 as usize);
    const s1 = c1.formatRef(alloc)?;
    defer alloc.free(s1);
    expect_eq(s1, "A1");

    const c2 = CellCoord.parseRef("Z10")?;
    expect_eq(c2.row, 9 as usize);
    expect_eq(c2.col, 25 as usize);
    const s2 = c2.formatRef(alloc)?;
    defer alloc.free(s2);
    expect_eq(s2, "Z10");

    const c3 = CellCoord.parseRef("AA1")?;
    expect_eq(c3.row, 0 as usize);
    expect_eq(c3.col, 26 as usize);
    const s3 = c3.formatRef(alloc)?;
    defer alloc.free(s3);
    expect_eq(s3, "AA1");

    const c4 = CellCoord.parseRef("AZ99")?;
    expect_eq(c4.row, 98 as usize);
    expect_eq(c4.col, 51 as usize);
    const s4 = c4.formatRef(alloc)?;
    defer alloc.free(s4);
    expect_eq(s4, "AZ99");

    // Case-insensitive column letters & upper bound ZZ9999
    const c5 = CellCoord.parseRef("zz9999")?;
    expect_eq(c5.row, 9998 as usize);
    expect_eq(c5.col, 701 as usize);
    const s5 = c5.formatRef(alloc)?;
    defer alloc.free(s5);
    expect_eq(s5, "ZZ9999");

    // Invalid references must return SheetError.InvalidCellRef
    const bad_refs = [
        "",
        "A",
        "1",
        "A0",
        "A01",
        "1A",
        "AAA1",
        "A10000",
        "A1B",
        "A1:B2",
    ];
    for (bad of bad_refs) {
        const tag = switch (CellCoord.parseRef(bad)) {
            SheetError.InvalidCellRef => "invalid",
            else => "other",
            ok => "ok",
        };
        expect_eq(tag, "invalid");
    }
}

test("2. Formula Parsing, Precedence & Built-ins (SUM/AVG/MIN/MAX/COUNT/IF/ROUND)") {
    const alloc = std.testing.allocator;
    let sheet = Spreadsheet.init(alloc);
    defer sheet.deinit();

    sheet.setNumber("A1", 10.0)?;
    sheet.setNumber("A2", 20.0)?;
    sheet.setNumber("A3", 30.0)?;
    sheet.setNumber("A4", 40.0)?;
    sheet.setNumber("A5", 50.0)?;
    sheet.setText("A6", "ignored_text")?;

    sheet.setFormula("B1", "=SUM(A1:A5)")?;
    sheet.setFormula("B2", "=AVG(A1:A5)")?;
    sheet.setFormula("B3", "=MIN(A1:A5)")?;
    sheet.setFormula("B4", "=MAX(A1:A5)")?;
    sheet.setFormula("B5", "=COUNT(A1:A6)")?;
    sheet.setFormula("B6", "=IF(A1 > 10, A1 * 2, 0)")?;
    sheet.setFormula("B7", "IF(A2 > 10, A2 * 2, 0)")?;
    sheet.setFormula("B8", "=(1 + 2) * 3 - 4 / 2 + -1")?;
    sheet.setFormula("B9", "=ROUND(3.14159, 2)")?;

    const count = sheet.recalculate()?;
    expect_eq(count, 9 as usize);

    expectCellNum(sheet, "B1", 150.0)?;
    expectCellNum(sheet, "B2", 30.0)?;
    expectCellNum(sheet, "B3", 10.0)?;
    expectCellNum(sheet, "B4", 50.0)?;
    expectCellNum(sheet, "B5", 5.0)?;
    expectCellNum(sheet, "B6", 0.0)?;
    expectCellNum(sheet, "B7", 40.0)?;
    expectCellNum(sheet, "B8", 6.0)?;
    expectCellNum(sheet, "B9", 3.14)?;
}

test("3. DAG Multi-Level Cascading Recalculation") {
    const alloc = std.testing.allocator;
    let sheet = Spreadsheet.init(alloc);
    defer sheet.deinit();

    // A1 -> B1(=A1*2) -> C1(=B1+SUM(A1:B1))
    sheet.setNumber("A1", 5.0)?;
    sheet.setFormula("B1", "=A1 * 2")?;
    sheet.setFormula("C1", "=B1 + SUM(A1:B1)")?;
    sheet.setFormula("D1", "=C1 + 100")?;

    const n1 = sheet.recalculate()?;
    expect_eq(n1, 3 as usize);
    expectCellNum(sheet, "B1", 10.0)?;
    expectCellNum(sheet, "C1", 25.0)?;
    expectCellNum(sheet, "D1", 125.0)?;

    // Second recalculate with no edits does 0 work
    expect_eq(sheet.recalculate()?, 0 as usize);

    // Update upstream A1 -> all downstream cells must be marked dirty and recalculated in topo order
    sheet.setNumber("A1", 20.0)?;
    const n2 = sheet.recalculate()?;
    expect_eq(n2, 3 as usize);
    expectCellNum(sheet, "B1", 40.0)?;
    expectCellNum(sheet, "C1", 100.0)?;
    expectCellNum(sheet, "D1", 200.0)?;
}

test("4. CycleDetected, DivisionByZero, CSV Export & Zero-Leak Allocator") {
    const alloc = std.testing.allocator;
    let sheet = Spreadsheet.init(alloc);
    defer sheet.deinit();

    // 1. Cycle detection: A1 -> B1 -> A1
    sheet.setFormula("A1", "=B1 + 1")?;
    sheet.setFormula("B1", "=A1 + 1")?;
    const cyc_tag = switch (sheet.recalculate()) {
        SheetError.CycleDetected => "cycle",
        else => "other",
        ok => "ok",
    };
    expect_eq(cyc_tag, "cycle");

    // Break cycle and verify recovery
    sheet.setNumber("A1", 10.0)?;
    expect_eq(sheet.recalculate()?, 1 as usize);
    expectCellNum(sheet, "B1", 11.0)?;

    // 2. DivisionByZero interception (and IF short-circuit safety)
    sheet.setNumber("C1", 0.0)?;
    sheet.setFormula("D1", "=IF(C1 == 0, 99, 10 / C1)")?;
    expect_eq(sheet.recalculate()?, 1 as usize);
    expectCellNum(sheet, "D1", 99.0)?;

    sheet.setFormula("E1", "=10 / C1")?;
    const div_tag = switch (sheet.recalculate()) {
        SheetError.DivisionByZero => "div_zero",
        else => "other",
        ok => "ok",
    };
    expect_eq(div_tag, "div_zero");
    sheet.clear("E1")?;

    // 3. CSV export (including quoted text with comma)
    sheet.setText("A2", "Revenue, Q1")?;
    sheet.setNumber("B2", 42.5)?;
    sheet.setFormula("C2", "=IF(B2 > 40, \"PASS\", \"FAIL\")")?;
    * = sheet.recalculate()?;

    const csv = sheet.toCsv(alloc, 2, 3)?;
    defer alloc.free(csv);
    expect_eq(csv, "10,11,0\n\"Revenue, Q1\",42.5,PASS");
}

test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.sheet`") {
    const alloc = std.testing.allocator;
    let sheet = cap.sheet.Spreadsheet.init(alloc);
    defer sheet.deinit();

    const coord = cap.sheet.CellCoord.parseRef("B2")?;
    expect_eq(coord.row, 1 as usize);
    expect_eq(coord.col, 1 as usize);

    sheet.setNumber("A1", 12.0)?;
    sheet.setNumber("A2", 18.0)?;
    sheet.setFormula("B1", "=AVG(A1:A2)")?;
    sheet.setFormula("B2", "=IF(B1 >= 15, ROUND(B1 * 1.1, 1), 0)")?;
    const n = sheet.recalculate()?;
    expect_eq(n, 2 as usize);

    const val = sheet.getValue("B2")?;
    expect_eq(approxEq(val.asNumber().?, 16.5), true);
}
