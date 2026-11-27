import { test, expect_eq } from "cap:test";
import {
    PrefixKind,
    decimalPrefix,
    binaryPrefix,
    formatPrefixed,
} from "../../packages/number_prefix/src/index.cp";

fn approx(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return d >= -eps && d <= eps;
}

fn main() {
    test("1. @cap/number-prefix — standalone values below threshold", () => {
        const d = decimalPrefix(999.0);
        expect_eq(d.is_standalone, true);
        expect_eq(d.prefix == .Standalone, true);
        expect_eq(d.symbol, "");

        const b = binaryPrefix(1023.0);
        expect_eq(b.is_standalone, true);
        let buf: [64]u8 = [0; 64];
        const n = formatPrefixed(b, 2, "B", buf[0..64]);
        expect_eq(buf[0..n] as string, "1023 B");
    });

    test("2. @cap/number-prefix — SI decimal prefixes k, M, G", () => {
        const k = decimalPrefix(1500.0);
        expect_eq(k.prefix == .Kilo, true);
        expect_eq(k.symbol, "k");
        expect_eq(approx(k.value, 1.5, 1.0e-9), true);

        const m = decimalPrefix(2500000.0);
        expect_eq(m.prefix == .Mega, true);
        expect_eq(m.symbol, "M");

        const g = decimalPrefix(3000000000.0);
        expect_eq(g.prefix == .Giga, true);
        expect_eq(g.symbol, "G");
    });

    test("3. @cap/number-prefix — IEC binary prefixes Ki, Mi, Gi", () => {
        const ki = binaryPrefix(1536.0);
        expect_eq(ki.prefix == .Kibi, true);
        expect_eq(ki.symbol, "Ki");
        expect_eq(approx(ki.value, 1.5, 1.0e-9), true);

        const mi = binaryPrefix(1048576.0);
        expect_eq(mi.prefix == .Mebi, true);
        expect_eq(mi.symbol, "Mi");

        const gi = binaryPrefix(1073741824.0);
        expect_eq(gi.prefix == .Gibi, true);
        expect_eq(gi.symbol, "Gi");
    });

    test("4. @cap/number-prefix — formatPrefixed renders fixed decimals and unit", () => {
        let buf: [64]u8 = [0; 64];
        const n1 = formatPrefixed(binaryPrefix(1536.0), 2, "B", buf[0..64]);
        expect_eq(buf[0..n1] as string, "1.50 KiB");

        const n2 = formatPrefixed(decimalPrefix(2050000.0), 2, "Hz", buf[0..64]);
        expect_eq(buf[0..n2] as string, "2.05 MHz");
    });

    test("5. @cap/number-prefix — handles negative numbers accurately", () => {
        const neg = binaryPrefix(-2048.0);
        expect_eq(neg.prefix == .Kibi, true);
        expect_eq(approx(neg.value, -2.0, 1.0e-9), true);
        let buf: [64]u8 = [0; 64];
        const n = formatPrefixed(neg, 1, "B", buf[0..64]);
        expect_eq(buf[0..n] as string, "-2.0 KiB");
    });

    test("6. @cap/number-prefix — handles Tera/Tebi scale without overflow", () => {
        const tb = decimalPrefix(4000000000000.0);
        expect_eq(tb.prefix == .Tera, true);
        expect_eq(tb.symbol, "T");
        expect_eq(approx(tb.value, 4.0, 1.0e-9), true);
    });
}
