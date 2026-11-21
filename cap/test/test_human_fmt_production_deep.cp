import { test, expect } from "cap:test";
import {
    TableStyle,
    formatBytes,
    formatDurationNs,
    redactUrl,
    formatTableBorder,
    formatTableRow,
} from "cap:human_fmt";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("human_fmt: formatBytes and formatDurationNs", () => {
    let buf: [64]u8 = [0; 64];

    expect(strEq(formatBytes(512, buf[0..]).?, "512 B"));
    expect(strEq(formatBytes(1536, buf[0..]).?, "1.5 KB"));
    expect(strEq(formatBytes(2 * 1024 * 1024, buf[0..]).?, "2 MB"));
    expect(strEq(formatBytes(3 * 1024 * 1024 * 1024, buf[0..]).?, "3 GB"));

    expect(strEq(formatDurationNs(850, buf[0..]).?, "850ns"));
    expect(strEq(formatDurationNs(12_500, buf[0..]).?, "12.5us"));
    expect(strEq(formatDurationNs(42_500_000, buf[0..]).?, "42.5ms"));
    expect(strEq(formatDurationNs(1_250_000_000, buf[0..]).?, "1.25s"));
    expect(strEq(formatDurationNs(125_000_000_000, buf[0..]).?, "2m5s"));
});

test("human_fmt: redactUrl and CLI table rendering", () => {
    let buf: [256]u8 = [0; 256];

    const u1 = redactUrl("https://alice:topsecret@registry.example.com/tok/npm_AbCdEf123456/id/550e8400-e29b-41d4-a716-446655440000", buf[0..]).?;
    expect(strEq(u1, "https://alice:***@registry.example.com/tok/***/id/***"));

    const widths: [2]usize = [4, 5];
    expect(strEq(formatTableBorder(widths[0..], TableStyle.ascii, 0, buf[0..]).?, "+------+-------+"));
    const cells: [2][]const u8 = ["Name", "Size"];
    expect(strEq(formatTableRow(cells[0..], widths[0..], TableStyle.ascii, buf[0..]).?, "| Name | Size  |"));
});
