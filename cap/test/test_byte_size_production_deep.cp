import {
    parseByteSize,
    formatBytesIec,
    formatBytesSi,
} from "cap:text";
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

test("parseByteSize parses SI, IEC, fractional, and rejects invalid inputs", () => {
    expect(parseByteSize("0").? == 0);
    expect(parseByteSize("512B").? == 512);
    expect(parseByteSize("10KB").? == 10000);
    expect(parseByteSize("10 KiB").? == 10240);
    expect(parseByteSize("1.5 MiB").? == 1572864);
    expect(parseByteSize("2.25GB").? == 2250000000);

    expect(parseByteSize("") == null);
    expect(parseByteSize("-5KB") == null);
    expect(parseByteSize("12XB") == null);
    expect(parseByteSize("1.MB") == null);
});

test("formatBytesIec and formatBytesSi canonical output", () => {
    let buf: [64]u8 = [0; 64];
    expect(strEq(formatBytesIec(512, buf[0..]).?, "512 B"));
    expect(strEq(formatBytesIec(1024, buf[0..]).?, "1 KiB"));
    expect(strEq(formatBytesIec(1536, buf[0..]).?, "1.5 KiB"));
    expect(strEq(formatBytesIec(1572864, buf[0..]).?, "1.5 MiB"));

    expect(strEq(formatBytesSi(999, buf[0..]).?, "999 B"));
    expect(strEq(formatBytesSi(1000, buf[0..]).?, "1 KB"));
    expect(strEq(formatBytesSi(2250000000, buf[0..]).?, "2.25 GB"));
});
