// Comprehensive Unit Test Suite for @cap/bytes
// Covers binary/SI parsing, decimals, case-insensitivity, whitespace, u64 overflow guards,
// formatting options, carry promotion, trailing zero trimming, and small-buffer protection.
import {
    BytesError,
    KIB,
    MIB,
    GIB,
    TIB,
    PIB,
    KB,
    MB,
    GB,
    TB,
    PB,
    ByteUnitMode,
    FormatOptions,
    parseBytes,
    parseBytesBinary,
    parseBytesSi,
    formatBytes,
    formatBinary,
    formatDecimal,
    formatCompact,
} from "cap:bytes";

fn assert(cond: bool) {
    if (!cond) {
        @panic("@cap/bytes test assertion failed");
    }
}

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn expectParseErr(input: []const u8, expected: BytesError) {
    let matched = false;
    * = parseBytes(input) catch e => {
        assert(e == expected);
        matched = true;
    };
    assert(matched);
}

fn expectParseSiErr(input: []const u8, expected: BytesError) {
    let matched = false;
    * = parseBytesSi(input) catch e => {
        assert(e == expected);
        matched = true;
    };
    assert(matched);
}

fn testConstants() {
    assert(KIB == 1024);
    assert(MIB == 1024 * KIB);
    assert(GIB == 1024 * MIB);
    assert(TIB == 1024 * GIB);
    assert(PIB == 1024 * TIB);

    assert(KB == 1000);
    assert(MB == 1000 * KB);
    assert(GB == 1000 * MB);
    assert(TB == 1000 * GB);
    assert(PB == 1000 * TB);
}

fn testParseBinaryAndDefault() {
    // Plain numbers & bytes
    assert((parseBytes("0") catch unreachable) == 0);
    assert((parseBytes("512") catch unreachable) == 512);
    assert((parseBytes("512 b") catch unreachable) == 512);
    assert((parseBytes("512B") catch unreachable) == 512);
    assert((parseBytes("1 byte") catch unreachable) == 1);
    assert((parseBytes("42 Bytes") catch unreachable) == 42);
    assert((parseBytes("  1024  ") catch unreachable) == 1024);

    // Binary IEC & npm `bytes` compatible units
    assert((parseBytes("1KB") catch unreachable) == KIB);
    assert((parseBytes("100KB") catch unreachable) == 100 * KIB);
    assert((parseBytes("1k") catch unreachable) == KIB);
    assert((parseBytes("1KiB") catch unreachable) == KIB);
    assert((parseBytes("1.5MB") catch unreachable) == MIB + 512 * KIB);
    assert((parseBytes("1.5 mb") catch unreachable) == MIB + 512 * KIB);
    assert((parseBytes("2.5 GiB") catch unreachable) == 2 * GIB + 512 * MIB);
    assert((parseBytes("4TB") catch unreachable) == 4 * TIB);
    assert((parseBytes("1PB") catch unreachable) == PIB);
    assert((parseBytes("1PiB") catch unreachable) == PIB);

    // Case-insensitivity & whitespace
    assert((parseBytes("  \t 3.25   mIb \n ") catch unreachable) == 3 * MIB + 256 * KIB);
    assert((parseBytesBinary("2g") catch unreachable) == 2 * GIB);
    assert((parseBytesBinary("0.5 KiB") catch unreachable) == 512);
    assert((parseBytesBinary(".5 KiB") catch unreachable) == 512);
    assert((parseBytesBinary("1.000 MiB") catch unreachable) == MIB);
}

fn testParseSi() {
    // SI 1000-based for KB/MB/GB/TB/PB and K/M/G/T/P
    assert((parseBytesSi("1KB") catch unreachable) == KB);
    assert((parseBytesSi("100KB") catch unreachable) == 100 * KB);
    assert((parseBytesSi("1.5MB") catch unreachable) == 1500000);
    assert((parseBytesSi("2.5 GB") catch unreachable) == 2500000000);
    assert((parseBytesSi("4TB") catch unreachable) == 4 * TB);
    assert((parseBytesSi("1PB") catch unreachable) == PB);
    assert((parseBytesSi("3k") catch unreachable) == 3000);
    assert((parseBytesSi("2m") catch unreachable) == 2 * MB);

    // Explicit IEC suffixes remain 1024-based even in SI mode
    assert((parseBytesSi("1KiB") catch unreachable) == KIB);
    assert((parseBytesSi("2.5 GiB") catch unreachable) == 2 * GIB + 512 * MIB);
    assert((parseBytesSi("1TiB") catch unreachable) == TIB);
    assert((parseBytesSi("1PiB") catch unreachable) == PIB);
}

fn testParseErrorsAndOverflow() {
    // EmptyInput
    expectParseErr("", BytesError.EmptyInput);
    expectParseErr("   \t\r\n  ", BytesError.EmptyInput);

    // InvalidFormat
    expectParseErr("MB", BytesError.InvalidFormat);
    expectParseErr(".", BytesError.InvalidFormat);
    expectParseErr(".KB", BytesError.InvalidFormat);
    expectParseErr("-10KB", BytesError.InvalidFormat);
    expectParseErr("+10KB", BytesError.InvalidFormat);
    expectParseErr("1.2.3MB", BytesError.InvalidFormat);
    expectParseErr("abc", BytesError.InvalidFormat);

    // UnknownUnit
    expectParseErr("10XB", BytesError.UnknownUnit);
    expectParseErr("10 MiBytes", BytesError.UnknownUnit);
    expectParseErr("10 M B", BytesError.UnknownUnit);
    expectParseErr("100zz", BytesError.UnknownUnit);
    expectParseSiErr("500 foo", BytesError.UnknownUnit);

    // Exact u64::MAX boundary
    const u64_max: u64 = (u64(0xFFFFFFFF) << 32) | u64(0xFFFFFFFF);
    assert((parseBytes("18446744073709551615") catch unreachable) == u64_max);
    assert((parseBytes("18446744073709551615 B") catch unreachable) == u64_max);
    assert((parseBytes("16383 PiB") catch unreachable) == 16383 * PIB);

    // Overflow cases
    expectParseErr("18446744073709551616", BytesError.Overflow);
    expectParseErr("99999999999999999999999", BytesError.Overflow);
    expectParseErr("16384 PiB", BytesError.Overflow);
    expectParseErr("16384 PB", BytesError.Overflow);
    expectParseSiErr("18447 PB", BytesError.Overflow);
    expectParseErr("16383.9999999999999999 PiB", BytesError.Overflow);
}

fn testFormatBinaryDecimalAndCompact() {
    let buf: [64]u8 = [0; 64];

    // Binary IEC
    assert(sliceEql(formatBinary(0, buf[0..]) catch unreachable, "0 B"));
    assert(sliceEql(formatBinary(512, buf[0..]) catch unreachable, "512 B"));
    assert(sliceEql(formatBinary(1023, buf[0..]) catch unreachable, "1023 B"));
    assert(sliceEql(formatBinary(KIB, buf[0..]) catch unreachable, "1 KiB"));
    assert(sliceEql(formatBinary(1536, buf[0..]) catch unreachable, "1.5 KiB"));
    assert(sliceEql(formatBinary(MIB + 512 * KIB, buf[0..]) catch unreachable, "1.5 MiB"));
    assert(sliceEql(formatBinary(2 * GIB + 512 * MIB, buf[0..]) catch unreachable, "2.5 GiB"));
    assert(sliceEql(formatBinary(4 * TIB, buf[0..]) catch unreachable, "4 TiB"));
    assert(sliceEql(formatBinary(PIB, buf[0..]) catch unreachable, "1 PiB"));

    // Decimal SI
    assert(sliceEql(formatDecimal(0, buf[0..]) catch unreachable, "0 B"));
    assert(sliceEql(formatDecimal(999, buf[0..]) catch unreachable, "999 B"));
    assert(sliceEql(formatDecimal(KB, buf[0..]) catch unreachable, "1 KB"));
    assert(sliceEql(formatDecimal(1500, buf[0..]) catch unreachable, "1.5 KB"));
    assert(sliceEql(formatDecimal(1500000, buf[0..]) catch unreachable, "1.5 MB"));
    assert(sliceEql(formatDecimal(2500000000, buf[0..]) catch unreachable, "2.5 GB"));
    assert(sliceEql(formatDecimal(4 * TB, buf[0..]) catch unreachable, "4 TB"));
    assert(sliceEql(formatDecimal(PB, buf[0..]) catch unreachable, "1 PB"));

    // Compact (no space, SI)
    assert(sliceEql(formatCompact(512, buf[0..]) catch unreachable, "512B"));
    assert(sliceEql(formatCompact(1500000, buf[0..]) catch unreachable, "1.5MB"));
    assert(sliceEql(formatCompact(2500000000, buf[0..]) catch unreachable, "2.5GB"));
}

fn testFormatOptionsCarryAndBuffers() {
    let buf: [64]u8 = [0; 64];

    // Decimals = 0
    const opt0 = FormatOptions { mode: ByteUnitMode.BinaryIec, decimals: 0, space: true };
    assert(sliceEql(formatBytes(1536, opt0, buf[0..]) catch unreachable, "2 KiB"));

    // Decimals = 3 with trailing zero trimming
    const opt3 = FormatOptions { mode: ByteUnitMode.BinaryIec, decimals: 3, space: false };
    assert(sliceEql(formatBytes(1024 + 128, opt3, buf[0..]) catch unreachable, "1.125KiB"));
    assert(sliceEql(formatBytes(1024 + 256, opt3, buf[0..]) catch unreachable, "1.25KiB"));

    // Decimals > 4 clamped to 4
    const opt_high = FormatOptions { mode: ByteUnitMode.DecimalSi, decimals: 9, space: true };
    assert(sliceEql(formatBytes(1234567, opt_high, buf[0..]) catch unreachable, "1.2346 MB"));

    // Rounding carry promotion: 1024*1024 - 1 = 1048575 -> 1023.999 KiB -> rounds to 1 MiB
    assert(sliceEql(formatBinary(MIB - 1, buf[0..]) catch unreachable, "1 MiB"));
    assert(sliceEql(formatDecimal(MB - 1, buf[0..]) catch unreachable, "1 MB"));

    // u64::MAX formatting
    const u64_max: u64 = (u64(0xFFFFFFFF) << 32) | u64(0xFFFFFFFF);
    assert(sliceEql(formatBinary(u64_max, buf[0..]) catch unreachable, "16384 PiB"));
    assert(sliceEql(formatDecimal(u64_max, buf[0..]) catch unreachable, "18446.74 PB"));

    // Small buffer protection
    let tiny: [3]u8 = [0; 3];
    let got_small = false;
    * = formatBinary(MIB, tiny[0..]) catch e => {
        assert(e == BytesError.BufferTooSmall);
        got_small = true;
    };
    assert(got_small);

    got_small = false;
    * = formatCompact(0, tiny[0..0]) catch e => {
        assert(e == BytesError.BufferTooSmall);
        got_small = true;
    };
    assert(got_small);

    // Exact-fit buffer succeeds ("1.5MB" = 5 bytes)
    let exact: [5]u8 = [0; 5];
    assert(sliceEql(formatCompact(1500000, exact[0..]) catch unreachable, "1.5MB"));
}

pub fn main() {
    testConstants();
    testParseBinaryAndDefault();
    testParseSi();
    testParseErrorsAndOverflow();
    testFormatBinaryDecimalAndCompact();
    testFormatOptionsCarryAndBuffers();
}
