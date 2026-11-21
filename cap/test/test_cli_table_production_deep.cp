import {
    TableBorderStyle,
    CellAlign,
    computeColumnWidths,
    renderBorderLine,
    renderTableRow,
    renderTable,
} from "cap:cli";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("computeColumnWidths calculates max widths, clamps safely, and ignores ANSI codes", () => {
    const r0: [3][]const u8 = ["Name", "Size", "Status"];
    const r1: [3][]const u8 = ["\x1b[32mcap\x1b[0m", "12KB", "OK"];
    const r2: [3][]const u8 = ["中文包", "1024KB", "READY"];
    const rows: [3][]const []const u8 = [r0[0..], r1[0..], r2[0..]];

    let widths: [4]usize = [99; 4];
    const cols = computeColumnWidths(rows[0..], widths[0..]);
    expect(cols == 3);
    // "中文包" has 3 CJK chars => display width 6
    expect(widths[0] == 6);
    expect(widths[1] == 6);
    expect(widths[2] == 6);
    expect(widths[3] == 0);

    // Clamps safely when widths_out is smaller than row width
    let small_widths: [2]usize = [0; 2];
    const clamped = computeColumnWidths(rows[0..], small_widths[0..]);
    expect(clamped == 2);
    expect(small_widths[0] == 6 && small_widths[1] == 6);
});

test("renderBorderLine supports ascii, unicode_light, and compact styles", () => {
    const widths: [2]usize = [3, 4];
    let buf: [128]u8 = [0; 128];

    // ASCII
    expect(strEq(renderBorderLine(widths[0..], TableBorderStyle.ascii, 0, buf[0..]).?, "+-----+------+"));
    expect(strEq(renderBorderLine(widths[0..], TableBorderStyle.ascii, 1, buf[0..]).?, "+-----+------+"));
    expect(strEq(renderBorderLine(widths[0..], TableBorderStyle.ascii, 2, buf[0..]).?, "+-----+------+"));

    // Unicode light
    expect(strEq(renderBorderLine(widths[0..], TableBorderStyle.unicode_light, 0, buf[0..]).?, "┌─────┬──────┐"));
    expect(strEq(renderBorderLine(widths[0..], TableBorderStyle.unicode_light, 1, buf[0..]).?, "├─────┼──────┤"));
    expect(strEq(renderBorderLine(widths[0..], TableBorderStyle.unicode_light, 2, buf[0..]).?, "└─────┴──────┘"));

    // Compact: empty top & bottom, dashed middle
    expect(strEq(renderBorderLine(widths[0..], TableBorderStyle.compact, 0, buf[0..]).?, ""));
    expect(strEq(renderBorderLine(widths[0..], TableBorderStyle.compact, 1, buf[0..]).?, "---  ----"));
    expect(strEq(renderBorderLine(widths[0..], TableBorderStyle.compact, 2, buf[0..]).?, ""));

    // Invalid pos returns null
    expect(renderBorderLine(widths[0..], TableBorderStyle.ascii, 3, buf[0..]) == null);
});

test("renderTableRow aligns left, right, center and handles missing cells/alignments", () => {
    const widths: [3]usize = [5, 6, 5];
    const col_aligns: [3]CellAlign = [CellAlign.left, CellAlign.right, CellAlign.center];
    const cells: [3][]const u8 = ["ab", "42", "ok"];
    let buf: [128]u8 = [0; 128];

    // ASCII row: left "ab   ", right "    42", center " ok  "
    const row_ascii = renderTableRow(cells[0..], widths[0..], col_aligns[0..], TableBorderStyle.ascii, buf[0..]).?;
    expect(strEq(row_ascii, "| ab    |     42 |  ok   |"));

    // Unicode light row
    const row_uni = renderTableRow(cells[0..], widths[0..], col_aligns[0..], TableBorderStyle.unicode_light, buf[0..]).?;
    expect(strEq(row_uni, "│ ab    │     42 │  ok   │"));

    // Compact row
    const row_compact = renderTableRow(cells[0..], widths[0..], col_aligns[0..], TableBorderStyle.compact, buf[0..]).?;
    expect(strEq(row_compact, "ab         42   ok  "));

    // Missing cells default to "", missing alignments default to left
    const short_cells: [1][]const u8 = ["x"];
    const empty_aligns: [0]CellAlign = [];
    const partial = renderTableRow(short_cells[0..], widths[0..], empty_aligns[0..], TableBorderStyle.ascii, buf[0..]).?;
    expect(strEq(partial, "| x     |        |       |"));
});

test("renderTable renders complete ASCII, Unicode, and Compact tables", () => {
    const headers: [2][]const u8 = ["Pkg", "Size"];
    const r0: [2][]const u8 = ["cap", "12KB"];
    const r1: [2][]const u8 = ["cpc", "480KB"];
    const rows: [2][]const []const u8 = [r0[0..], r1[0..]];
    const col_aligns: [2]CellAlign = [CellAlign.left, CellAlign.right];

    let buf: [512]u8 = [0; 512];

    const ascii_tbl = renderTable(headers[0..], rows[0..], col_aligns[0..], TableBorderStyle.ascii, buf[0..]).?;
    const expected_ascii =
        "+-----+-------+\n" ++
        "| Pkg |  Size |\n" ++
        "+-----+-------+\n" ++
        "| cap |  12KB |\n" ++
        "| cpc | 480KB |\n" ++
        "+-----+-------+";
    expect(strEq(ascii_tbl, expected_ascii));

    const compact_tbl = renderTable(headers[0..], rows[0..], col_aligns[0..], TableBorderStyle.compact, buf[0..]).?;
    const expected_compact =
        "Pkg   Size\n" ++
        "---  -----\n" ++
        "cap   12KB\n" ++
        "cpc  480KB";
    expect(strEq(compact_tbl, expected_compact));

    const uni_tbl = renderTable(headers[0..], rows[0..], col_aligns[0..], TableBorderStyle.unicode_light, buf[0..]).?;
    const expected_uni =
        "┌─────┬───────┐\n" ++
        "│ Pkg │  Size │\n" ++
        "├─────┼───────┤\n" ++
        "│ cap │  12KB │\n" ++
        "│ cpc │ 480KB │\n" ++
        "└─────┴───────┘";
    expect(strEq(uni_tbl, expected_uni));
});

test("buffer capacity limits and empty table edge cases", () => {
    const headers: [2][]const u8 = ["A", "B"];
    const r0: [2][]const u8 = ["1", "2"];
    const rows: [1][]const []const u8 = [r0[0..]];
    const col_aligns: [2]CellAlign = [CellAlign.left, CellAlign.right];

    // Tiny buffer returns null cleanly
    let tiny: [8]u8 = [0; 8];
    expect(renderTable(headers[0..], rows[0..], col_aligns[0..], TableBorderStyle.ascii, tiny[0..]) == null);

    const widths: [2]usize = [3, 3];
    expect(renderBorderLine(widths[0..], TableBorderStyle.ascii, 0, tiny[0..]) == null);
    expect(renderTableRow(headers[0..], widths[0..], col_aligns[0..], TableBorderStyle.ascii, tiny[0..]) == null);

    // Empty headers + empty rows returns empty slice
    const empty_h: [0][]const u8 = [];
    const empty_r: [0][]const []const u8 = [];
    const empty_a: [0]CellAlign = [];
    let buf: [32]u8 = [0; 32];
    expect(strEq(renderTable(empty_h[0..], empty_r[0..], empty_a[0..], TableBorderStyle.ascii, buf[0..]).?, ""));
});
