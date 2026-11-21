import { ProcStatm, parseProcStatm } from "cap:proc_statm";
import { test, expect, expect_eq } from "cap:test";

test("parseProcStatm parses standard 7-column /proc/pid/statm output", () => {
    const m = parseProcStatm("25600 4096 1024 512 0 2048 0\n").?;
    expect_eq(m.size_pages, 25600);
    expect_eq(m.resident_pages, 4096);
    expect_eq(m.shared_pages, 1024);
    expect_eq(m.text_pages, 512);
    expect_eq(m.lib_pages, 0);
    expect_eq(m.data_pages, 2048);
    expect_eq(m.dirty_pages, 0);
});

test("Byte conversions and private RSS calculations with 4KiB and 64KiB page sizes", () => {
    const m = parseProcStatm("1000 400 150 50 0 200 10\r\n").?;

    expect_eq(m.vmBytes(4096), 4096000);
    expect_eq(m.rssBytes(4096), 1638400);
    expect_eq(m.sharedBytes(4096), 614400);
    expect_eq(m.textBytes(4096), 204800);
    expect_eq(m.dataBytes(4096), 819200);
    expect_eq(m.privateRssPages(), 250);
    expect_eq(m.privateRssBytes(4096), 1024000);

    // 64KiB ARM64 page size & 0 page size
    expect_eq(m.rssBytes(65536), 26214400);
    expect_eq(m.privateRssBytes(65536), 16384000);
    expect_eq(m.vmBytes(0), 0);
});

test("Saturating arithmetic prevents multiplication overflow and subtraction underflow", () => {
    const U64_MAX: u64 = 18446744073709551615;

    // shared_pages > resident_pages must saturate privateRssPages to 0
    const underflow_case = parseProcStatm("500 100 250 10 0 20 0").?;
    expect_eq(underflow_case.privateRssPages(), 0);
    expect_eq(underflow_case.privateRssBytes(4096), 0);

    // Extreme page counts multiplied by 4096 must saturate to U64_MAX
    const huge = parseProcStatm("18446744073709551615 9223372036854775808 0 1 0 1 0").?;
    expect_eq(huge.size_pages, U64_MAX);
    expect_eq(huge.vmBytes(4096), U64_MAX);
    expect_eq(huge.rssBytes(4096), U64_MAX);
    expect_eq(huge.privateRssBytes(4096), U64_MAX);
    expect_eq(huge.vmBytes(1), U64_MAX);
    expect_eq(huge.vmBytes(0), 0);
});

test("Whitespace tolerance and strict 7-column count enforcement", () => {
    // Leading/trailing spaces, tabs, CRLF
    const spaced = parseProcStatm("  \t 10\t20   30\t\t40 50 60 70 \r\n ").?;
    expect_eq(spaced.size_pages, 10);
    expect_eq(spaced.dirty_pages, 70);

    // All zeros valid
    const zeros = parseProcStatm("0 0 0 0 0 0 0").?;
    expect_eq(zeros.vmBytes(4096), 0);
    expect_eq(zeros.privateRssPages(), 0);

    // Fewer than 7 columns rejected
    expect(parseProcStatm("") == null);
    expect(parseProcStatm("   \t\r\n") == null);
    expect(parseProcStatm("1 2 3 4 5 6") == null);
    expect(parseProcStatm("1") == null);

    // More than 7 columns rejected
    expect(parseProcStatm("1 2 3 4 5 6 7 8") == null);
    expect(parseProcStatm("1 2 3 4 5 6 7 extra") == null);
});

test("Rejects negative numbers, non-digits, and u64 overflows", () => {
    // Negative / signed values
    expect(parseProcStatm("-1 2 3 4 5 6 7") == null);
    expect(parseProcStatm("1 -2 3 4 5 6 7") == null);
    expect(parseProcStatm("+1 2 3 4 5 6 7") == null);

    // Non-digit characters
    expect(parseProcStatm("1 2 3a 4 5 6 7") == null);
    expect(parseProcStatm("1 2 3 4 5 6 0x7") == null);
    expect(parseProcStatm("1 2 3 4 5 6 7.0") == null);

    // u64::MAX + 1 = 18446744073709551616 must be rejected
    expect(parseProcStatm("18446744073709551616 0 0 0 0 0 0") == null);
    expect(parseProcStatm("0 0 0 0 0 0 99999999999999999999") == null);

    // Exact u64::MAX is valid
    const exact = parseProcStatm("0 0 0 0 0 0 18446744073709551615").?;
    expect_eq(exact.dirty_pages, 18446744073709551615);
});
