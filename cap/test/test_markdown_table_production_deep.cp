import {
    ColAlign,
    TableRowIter,
    parseDelimiterRow,
    unescapeTableCell,
    escapeTableCell,
} from "cap:markdown_table";
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

test("TableRowIter handles pipes, escaped pipes, and optional outer pipes", () => {
    let it = TableRowIter.init("| Name | Expr | Notes |");
    expect(strEq(it.next().?, "Name"));
    expect(strEq(it.next().?, "Expr"));
    expect(strEq(it.next().?, "Notes"));
    expect(it.next() == null);

    let it2 = TableRowIter.init("a \\| b | c");
    const c1 = it2.next().?;
    let buf: [64]u8 = [0; 64];
    expect(strEq(unescapeTableCell(c1, buf[0..]).?, "a | b"));
    expect(strEq(it2.next().?, "c"));
    expect(it2.next() == null);

    expect(strEq(escapeTableCell("x|y\nz", buf[0..]).?, "x\\|y z"));
});

test("parseDelimiterRow detects GFM column alignments and rejects non-delimiter rows", () => {
    let aligns: [8]ColAlign = [ColAlign.none; 8];
    const cols = parseDelimiterRow("| :--- | :---: | ---: | --- |", aligns[0..]).?;
    expect(cols == 4);
    expect(aligns[0] == ColAlign.left);
    expect(aligns[1] == ColAlign.center);
    expect(aligns[2] == ColAlign.right);
    expect(aligns[3] == ColAlign.none);

    expect(parseDelimiterRow("| not | a | delim |", aligns[0..]) == null);
    expect(parseDelimiterRow("| : |", aligns[0..]) == null);
});
