import {
    formatGroupedU64,
    formatGroupedI64,
    formatOrdinal,
    formatRadixU64,
    formatFixedPoint,
} from "cap:number_format";
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

test("formatGrouped, formatOrdinal, formatRadixU64, and formatFixedPoint", () => {
    let buf: [64]u8 = [0; 64];
    expect(strEq(formatGroupedU64(0, ',', buf[0..]).?, "0"));
    expect(strEq(formatGroupedU64(1234567890, ',', buf[0..]).?, "1,234,567,890"));
    expect(strEq(formatGroupedI64(-987654, '_', buf[0..]).?, "-987_654"));

    expect(strEq(formatOrdinal(1, buf[0..]).?, "1st"));
    expect(strEq(formatOrdinal(2, buf[0..]).?, "2nd"));
    expect(strEq(formatOrdinal(3, buf[0..]).?, "3rd"));
    expect(strEq(formatOrdinal(11, buf[0..]).?, "11th"));
    expect(strEq(formatOrdinal(12, buf[0..]).?, "12th"));
    expect(strEq(formatOrdinal(13, buf[0..]).?, "13th"));
    expect(strEq(formatOrdinal(21, buf[0..]).?, "21st"));

    expect(strEq(formatRadixU64(255, 16, true, buf[0..]).?, "FF"));
    expect(strEq(formatRadixU64(10, 2, false, buf[0..]).?, "1010"));
    expect(strEq(formatRadixU64(35, 36, false, buf[0..]).?, "z"));
    expect(formatRadixU64(10, 1, false, buf[0..]) == null);

    expect(strEq(formatFixedPoint(123456, 2, ',', buf[0..]).?, "1,234.56"));
    expect(strEq(formatFixedPoint(-5, 2, ',', buf[0..]).?, "-0.05"));
});
