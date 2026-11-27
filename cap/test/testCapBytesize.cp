// cap/test/testCapBytesize.cp
// Comprehensive unit tests for @cap/bytesize

import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    B,
    KB,
    MB,
    GB,
    TB,
    PB,
    KIB,
    MIB,
    GIB,
    TIB,
    PIB,
    ByteSize,
    parseByteSize,
    formatSi,
    formatIec,
} from "../../packages/bytesize/src/index.cp";

fn main() {
    test("1. @cap/bytesize — SI & IEC integer constants", () => {
        expect_eq(B, 1 as u64);
        expect_eq(KB, 1000 as u64);
        expect_eq(MB, 1000000 as u64);
        expect_eq(GB, 1000000000 as u64);
        expect_eq(TB, 1000000000000 as u64);
        expect_eq(PB, 1000000000000000 as u64);

        expect_eq(KIB, 1024 as u64);
        expect_eq(MIB, 1048576 as u64);
        expect_eq(GIB, 1073741824 as u64);
        expect_eq(TIB, 1099511627776 as u64);
        expect_eq(PIB, 1125899906842624 as u64);
    });

    test("2. @cap/bytesize — ByteSize constructors, arithmetic & asU64", () => {
        const s1 = ByteSize.b(42 as u64);
        const s2 = ByteSize.kb(3 as u64);
        const s3 = ByteSize.mb(2 as u64);
        const s4 = ByteSize.gb(1 as u64);
        const s5 = ByteSize.tb(1 as u64);
        expect_eq(s1.asU64(), 42 as u64);
        expect_eq(s2.asU64(), 3000 as u64);
        expect_eq(s3.asU64(), 2000000 as u64);
        expect_eq(s4.asU64(), GB);
        expect_eq(s5.asU64(), TB);

        const i1 = ByteSize.kib(4 as u64);
        const i2 = ByteSize.mib(2 as u64);
        const i3 = ByteSize.gib(1 as u64);
        const i4 = ByteSize.tib(1 as u64);
        expect_eq(i1.asU64(), 4096 as u64);
        expect_eq(i2.asU64(), 2097152 as u64);
        expect_eq(i3.asU64(), GIB);
        expect_eq(i4.asU64(), TIB);

        const sum = i1.add(s1);
        expect_eq(sum.asU64(), 4138 as u64);
        const diff = i1.subSaturating(i2);
        expect_eq(diff.asU64(), 0 as u64);
    });

    test("3. @cap/bytesize — parseByteSize integers & case-insensitive SI/IEC units", () => {
        expect_eq(parseByteSize("0").?, 0 as u64);
        expect_eq(parseByteSize("512").?, 512 as u64);
        expect_eq(parseByteSize("512 B").?, 512 as u64);
        expect_eq(parseByteSize("500MB").?, 500000000 as u64);
        expect_eq(parseByteSize("  4 kib  ").?, 4096 as u64);
        expect_eq(parseByteSize("2GiB").?, (2 as u64) * GIB);
        expect_eq(parseByteSize("3t").?, (3 as u64) * TB);
        expect_eq(parseByteSize("1 PiB").?, PIB);
    });

    test("4. @cap/bytesize — parseByteSize fractional decimals", () => {
        expect_eq(parseByteSize("1.5 GiB").?, (GIB + GIB / (2 as u64)));
        expect_eq(parseByteSize("2.5 kb").?, 2500 as u64);
        expect_eq(parseByteSize("0.5 MiB").?, 524288 as u64);
        expect_eq(parseByteSize("1.25 MB").?, 1250000 as u64);
        expect_eq(parseByteSize("0.25 KiB").?, 256 as u64);
    });

    test("5. @cap/bytesize — parseByteSize rejects malformed inputs & overflow", () => {
        expect_eq(parseByteSize("") == null, true);
        expect_eq(parseByteSize("   ") == null, true);
        expect_eq(parseByteSize("-10 MB") == null, true);
        expect_eq(parseByteSize("10 XB") == null, true);
        expect_eq(parseByteSize("10. MB") == null, true);
        expect_eq(parseByteSize("abc") == null, true);
        expect_eq(parseByteSize("99999999999999999999999999") == null, true);
        expect_eq(parseByteSize("100000 PiB") == null, true);
    });

    test("6. @cap/bytesize — formatSi & formatIec human-readable formatting", () => {
        let buf: [64]u8 = undefined;

        const n1 = formatSi(0 as u64, buf[0..]);
        expect_eq(strEq(buf[0..n1], "0 B"), true);

        const n2 = formatSi(999 as u64, buf[0..]);
        expect_eq(strEq(buf[0..n2], "999 B"), true);

        const n3 = formatSi(1500 as u64, buf[0..]);
        expect_eq(strEq(buf[0..n3], "1.5 KB"), true);

        const n4 = formatSi(2500000000 as u64, buf[0..]);
        expect_eq(strEq(buf[0..n4], "2.5 GB"), true);

        const n5 = formatIec(1023 as u64, buf[0..]);
        expect_eq(strEq(buf[0..n5], "1023 B"), true);

        const n6 = formatIec(1536 as u64, buf[0..]);
        expect_eq(strEq(buf[0..n6], "1.5 KiB"), true);

        const mib2 = ByteSize.mib(2 as u64);
        const n7 = mib2.displayIec(buf[0..]);
        expect_eq(strEq(buf[0..n7], "2.0 MiB"), true);
    });
}
