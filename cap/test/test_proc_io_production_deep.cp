import { ProcIoStats, parseProcIo } from "cap:sys";
import { test, expect, expect_eq } from "cap:test";

const SAMPLE_T1 =
    "rchar: 100000\n" ++
    "wchar: 50000\r\n" ++
    "syscr: 120\n" ++
    "syscw: 30\n" ++
    "read_bytes: 40960\n" ++
    "write_bytes: 20480\n" ++
    "cancelled_write_bytes: 4096\n";

const SAMPLE_T2 =
    "rchar: 250000\n" ++
    "wchar: 120000\n" ++
    "syscr: 300\n" ++
    "syscw: 90\n" ++
    "read_bytes: 81920\n" ++
    "write_bytes: 61440\n" ++
    "cancelled_write_bytes: 8192\n";

test("parseProcIo parses all 7 standard fields with LF and CRLF", () => {
    const s = parseProcIo(SAMPLE_T1).?;
    expect_eq(s.rchar, 100000);
    expect_eq(s.wchar, 50000);
    expect_eq(s.syscr, 120);
    expect_eq(s.syscw, 30);
    expect_eq(s.read_bytes, 40960);
    expect_eq(s.write_bytes, 20480);
    expect_eq(s.cancelled_write_bytes, 4096);

    expect_eq(s.netWriteBytes(), 16384);
    expect_eq(s.totalStorageBytes(), 57344);
    expect_eq(s.totalSyscalls(), 150);
    expect_eq(s.totalCharBytes(), 150000);
});

test("ProcIoStats.diff computes per-field saturating sampling-window deltas", () => {
    const t1 = parseProcIo(SAMPLE_T1).?;
    const t2 = parseProcIo(SAMPLE_T2).?;
    const d = t2.diff(t1);

    expect_eq(d.rchar, 150000);
    expect_eq(d.wchar, 70000);
    expect_eq(d.syscr, 180);
    expect_eq(d.syscw, 60);
    expect_eq(d.read_bytes, 40960);
    expect_eq(d.write_bytes, 40960);
    expect_eq(d.cancelled_write_bytes, 4096);
    expect_eq(d.netWriteBytes(), 36864);
    expect_eq(d.totalStorageBytes(), 77824);
    expect_eq(d.totalSyscalls(), 240);

    // Reverse diff (earlier > later) must saturate to 0 on all fields
    const rev = t1.diff(t2);
    expect_eq(rev.rchar, 0);
    expect_eq(rev.wchar, 0);
    expect_eq(rev.syscr, 0);
    expect_eq(rev.syscw, 0);
    expect_eq(rev.read_bytes, 0);
    expect_eq(rev.write_bytes, 0);
    expect_eq(rev.cancelled_write_bytes, 0);
});

test("Saturating arithmetic prevents u64 addition overflow and subtraction underflow", () => {
    const U64_MAX: u64 = 18446744073709551615;
    const raw =
        "rchar: 18446744073709551615\n" ++
        "wchar: 100\n" ++
        "syscr: 18446744073709551610\n" ++
        "syscw: 20\n" ++
        "read_bytes: 18446744073709551615\n" ++
        "write_bytes: 1024\n" ++
        "cancelled_write_bytes: 4096\n";

    const s = parseProcIo(raw).?;
    // cancelled_write_bytes > write_bytes saturates netWriteBytes to 0
    expect_eq(s.netWriteBytes(), 0);
    expect_eq(s.totalStorageBytes(), U64_MAX);
    expect_eq(s.totalSyscalls(), U64_MAX);
    expect_eq(s.totalCharBytes(), U64_MAX);
});

test("Skips blank lines and unknown extension keys while allowing partial known subsets", () => {
    const ext =
        "\n" ++
        "   \t  \r\n" ++
        "future_kernel_metric: 999999\n" ++
        "not_a_kv_line\n" ++
        "  read_bytes :  \t 8192  \r\n" ++
        "unknown_str: hello_world\n" ++
        "write_bytes: 4096";

    const s = parseProcIo(ext).?;
    expect_eq(s.read_bytes, 8192);
    expect_eq(s.write_bytes, 4096);
    expect_eq(s.rchar, 0);
    expect_eq(s.cancelled_write_bytes, 0);
    expect_eq(s.totalStorageBytes(), 12288);
});

test("Rejects empty/unknown-only payloads, invalid numbers, and u64 overflows", () => {
    // Empty or no known keys
    expect(parseProcIo("") == null);
    expect(parseProcIo("   \n\t\r\n") == null);
    expect(parseProcIo("unknown_key: 123\nanother: 456\n") == null);
    expect(parseProcIo("malformed line without colon\n") == null);

    // Invalid numeric values on known keys
    expect(parseProcIo("rchar:\n") == null);
    expect(parseProcIo("rchar: -1\n") == null);
    expect(parseProcIo("wchar: 123abc\n") == null);
    expect(parseProcIo("syscr: 12 34\n") == null);
    expect(parseProcIo("read_bytes: 0x100\n") == null);

    // u64::MAX + 1 = 18446744073709551616 overflow rejected
    expect(parseProcIo("write_bytes: 18446744073709551616\n") == null);
    expect(parseProcIo("cancelled_write_bytes: 99999999999999999999\n") == null);
});
