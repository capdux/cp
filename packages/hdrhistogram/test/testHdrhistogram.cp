import { test, expect_eq } from "cap:test";
import { Histogram } from "../src/index.cp";

fn main() {
    test("1. @cap/hdrhistogram — record, recordN, len, isEmpty, min, max, mean, stdev", () => {
        let h = new Histogram();
        h.open(1, 1000000, 3) catch { expect_eq(true, false); return; };
        expect_eq(h.isEmpty() catch false, true);
        expect_eq(h.len() catch 99, 0 as u64);

        h.record(10) catch { expect_eq(true, false); };
        h.recordN(20, 3) catch { expect_eq(true, false); };
        h.record(30) catch { expect_eq(true, false); };

        expect_eq(h.isEmpty() catch true, false);
        expect_eq(h.len() catch 0, 5 as u64);
        expect_eq(h.min() catch 0, 10);
        expect_eq(h.max() catch 0, 30);
        expect_eq((h.mean() catch 0.0) >= 19.5 && (h.mean() catch 0.0) <= 20.5, true);
        expect_eq((h.stdev() catch -1.0) >= 0.0, true);
        expect_eq((h.stddev() catch -1.0) >= 0.0, true);
    });

    test("2. @cap/hdrhistogram — valueAtPercentile, valueAtQuantile, percentileBelow", () => {
        let h = new Histogram();
        h.open(1, 10000, 3) catch { expect_eq(true, false); return; };
        let i: i64 = 1;
        while (i <= 100) : (i += 1) {
            h.record(i) catch {};
        }
        expect_eq(h.len() catch 0, 100 as u64);
        expect_eq(h.valueAtPercentile(50.0) catch 0, 50);
        expect_eq(h.valueAtQuantile(0.5) catch 0, 50);
        expect_eq(h.valueAtQuantile(0.99) catch 0, 99);
        const pb50 = h.percentileBelow(50) catch 0.0;
        expect_eq(pb50 >= 49.0 && pb50 <= 51.0, true);
        const qb100 = h.quantileBelow(100) catch 0.0;
        expect_eq(qb100 >= 0.999 && qb100 <= 1.001, true);
    });

    test("3. @cap/hdrhistogram — recordCorrecting & add merge", () => {
        let h1 = new Histogram();
        let h2 = new Histogram();
        h1.open(1, 100000, 3) catch { expect_eq(true, false); return; };
        h2.open(1, 100000, 3) catch { expect_eq(true, false); return; };

        h1.record(100) catch {};
        h1.record(200) catch {};
        h2.recordCorrecting(1000, 250) catch {};

        const len2 = h2.len() catch 0;
        expect_eq(len2 >= 4 as u64, true);

        const dropped = h1.add(h2) catch -1;
        expect_eq(dropped, 0);
        expect_eq(h1.len() catch 0, (2 as u64) + len2);
        expect_eq(h1.max() catch 0, 1000);
    });

    test("4. @cap/hdrhistogram — reset, saturatingRecord & equivalence helpers", () => {
        let h = new Histogram();
        h.open(1, 1000, 3) catch { expect_eq(true, false); return; };
        h.saturatingRecord(5000) catch {};
        expect_eq(h.max() catch 0, 1000);
        expect_eq(h.countAt(1000) catch 0, 1);
        expect_eq(h.equivalent(100, 100) catch false, true);
        expect_eq((h.sizeOfEquivalentRange(100) catch 0) >= 1, true);

        h.reset() catch {};
        expect_eq(h.isEmpty() catch false, true);
        expect_eq(h.len() catch 99, 0 as u64);
    });
}
