// cap/test/testCapPrettyBytes.cp
// Comprehensive unit tests for @cap/pretty-bytes

import { test, expect_eq } from "cap:test";
import {
    PrettyBytesOpts,
    prettyBytes,
    prettyBytesWith
} from "../../packages/@cap/pretty-bytes/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/pretty-bytes — Basic SI formatting (0, 100, 1337, 1000000)", () => {
        let buf: [32]u8 = undefined;

        let n0 = prettyBytes(0.0, buf[0..32]);
        expect_eq(sliceEq(buf[0..n0], "0 B"), true);

        let n1 = prettyBytes(100.0, buf[0..32]);
        expect_eq(sliceEq(buf[0..n1], "100 B"), true);

        let n2 = prettyBytes(1337.0, buf[0..32]);
        expect_eq(sliceEq(buf[0..n2], "1.34 kB"), true);

        let n3 = prettyBytes(1000000.0, buf[0..32]);
        expect_eq(sliceEq(buf[0..n3], "1 MB"), true);

        let n4 = prettyBytes(1050.0, buf[0..32]);
        expect_eq(sliceEq(buf[0..n4], "1.05 kB"), true);
    });

    test("2. @cap/pretty-bytes — Negative numbers & signed_val=true (+42 B, -1.5 MB, ' 0 B')", () => {
        let buf: [32]u8 = undefined;

        let n1 = prettyBytes(-1500000.0, buf[0..32]);
        expect_eq(sliceEq(buf[0..n1], "-1.5 MB"), true);

        let opts = PrettyBytesOpts.defaults();
        opts.signed_val = true;

        let n2 = prettyBytesWith(42.0, opts, buf[0..32]);
        expect_eq(sliceEq(buf[0..n2], "+42 B"), true);

        let n3 = prettyBytesWith(-42.0, opts, buf[0..32]);
        expect_eq(sliceEq(buf[0..n3], "-42 B"), true);

        let n4 = prettyBytesWith(0.0, opts, buf[0..32]);
        expect_eq(sliceEq(buf[0..n4], " 0 B"), true);
    });

    test("3. @cap/pretty-bytes — IEC binary=true (1024 -> 1 KiB, 1536 -> 1.5 KiB, 1 MiB)", () => {
        let buf: [32]u8 = undefined;
        let opts = PrettyBytesOpts.defaults();
        opts.binary = true;

        let n1 = prettyBytesWith(1024.0, opts, buf[0..32]);
        expect_eq(sliceEq(buf[0..n1], "1 KiB"), true);

        let n2 = prettyBytesWith(1536.0, opts, buf[0..32]);
        expect_eq(sliceEq(buf[0..n2], "1.5 KiB"), true);

        let n3 = prettyBytesWith(1048576.0, opts, buf[0..32]);
        expect_eq(sliceEq(buf[0..n3], "1 MiB"), true);
    });

    test("4. @cap/pretty-bytes — Bit units (bits=true) for SI & IEC binary", () => {
        let buf: [32]u8 = undefined;

        let si_bits = PrettyBytesOpts.defaults();
        si_bits.bits = true;
        let n1 = prettyBytesWith(0.0, si_bits, buf[0..32]);
        expect_eq(sliceEq(buf[0..n1], "0 b"), true);

        let n2 = prettyBytesWith(1337.0, si_bits, buf[0..32]);
        expect_eq(sliceEq(buf[0..n2], "1.34 kbit"), true);

        let iec_bits = PrettyBytesOpts.defaults();
        iec_bits.bits = true;
        iec_bits.binary = true;
        let n3 = prettyBytesWith(1024.0, iec_bits, buf[0..32]);
        expect_eq(sliceEq(buf[0..n3], "1 kibit"), true);

        let n4 = prettyBytesWith(1048576.0, iec_bits, buf[0..32]);
        expect_eq(sliceEq(buf[0..n4], "1 Mibit"), true);
    });

    test("5. @cap/pretty-bytes — Compact mode (space=false) & custom precision", () => {
        let buf: [32]u8 = undefined;
        let opts = PrettyBytesOpts.defaults();
        opts.space = false;

        let n1 = prettyBytesWith(1337.0, opts, buf[0..32]);
        expect_eq(sliceEq(buf[0..n1], "1.34kB"), true);

        opts.precision = 3;
        let n2 = prettyBytesWith(1337.0, opts, buf[0..32]);
        expect_eq(sliceEq(buf[0..n2], "1.337kB"), true);
    });

    test("6. @cap/pretty-bytes — Large units (TB, PB, EB) & small buffer boundary safety", () => {
        let buf: [32]u8 = undefined;

        let n_tb = prettyBytes(1000000000000.0, buf[0..32]);
        expect_eq(sliceEq(buf[0..n_tb], "1 TB"), true);

        let n_pb = prettyBytes(2500000000000000.0, buf[0..32]);
        expect_eq(sliceEq(buf[0..n_pb], "2.5 PB"), true);

        let n_eb = prettyBytes(1000000000000000000.0, buf[0..32]);
        expect_eq(sliceEq(buf[0..n_eb], "1 EB"), true);

        // Buffer too small ("1.34 kB" needs 7 bytes) -> safely returns 0
        let tiny: [5]u8 = undefined;
        expect_eq(prettyBytes(1337.0, tiny[0..5]), 0 as usize);
        expect_eq(prettyBytes(0.0, tiny[0..0]), 0 as usize);
    });
}
