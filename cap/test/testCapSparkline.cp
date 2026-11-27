// cap/test/testCapSparkline.cp
// Comprehensive unit tests for @cap/sparkline

import { test, expect_eq } from "cap:test";
import {
    render,
    renderAscii,
    renderWithOptions,
    renderBar,
    renderDownsampled,
    renderLabeledBar,
    computeStats,
    defaultOptions,
    quantizeLevel,
    SparkResult,
    SeriesStats,
    SparkOptions,
} from "../../packages/sparkly/src/index.cp";

fn main() {
    test("1. @cap/sparkline — full 8-level unicode ramp", () => {
        const vals: [8]f64 = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0];
        const res: SparkResult = render(vals[0..8]);
        expect_eq(res.truncated, false);
        expect_eq(res.len, 24 as usize);
        expect_eq(res.asSlice(), "▁▂▃▄▅▆▇█");
    });

    test("2. @cap/sparkline — ascii mode and flat series", () => {
        const ramp: [8]f64 = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0];
        const asc: SparkResult = renderAscii(ramp[0..8]);
        expect_eq(asc.asSlice(), "._-:=+*#");

        const flat: [4]f64 = [42.0, 42.0, 42.0, 42.0];
        const flatRes: SparkResult = render(flat[0..4]);
        expect_eq(flatRes.asSlice(), "▄▄▄▄");
    });

    test("3. @cap/sparkline — custom bounds clamping", () => {
        const vals: [5]f64 = [-10.0, 0.0, 50.0, 100.0, 200.0];
        let opts: SparkOptions = defaultOptions();
        opts.useCustomBounds = true;
        opts.minBound = 0.0;
        opts.maxBound = 100.0;
        opts.asciiOnly = true;
        const res: SparkResult = renderWithOptions(vals[0..5], opts);
        expect_eq(res.asSlice(), "..=##");
        expect_eq(quantizeLevel(50.0, 0.0, 100.0), 4 as u8);
    });

    test("4. @cap/sparkline — fractional unicode & ascii horizontal bar", () => {
        const fullHalf: SparkResult = renderBar(0.5, 4, false);
        expect_eq(fullHalf.asSlice(), "██  ");

        const quarterCell: SparkResult = renderBar(0.625, 4, false);
        expect_eq(quarterCell.asSlice(), "██▌ ");

        const ascBar: SparkResult = renderBar(0.5, 6, true);
        expect_eq(ascBar.asSlice(), "###---");

        const labeled: SparkResult = renderLabeledBar("CPU", 0.5, 4, true);
        expect_eq(labeled.asSlice(), "CPU [##--]");
    });

    test("5. @cap/sparkline — bucket downsampling", () => {
        const raw: [8]f64 = [0.0, 0.0, 2.0, 2.0, 4.0, 4.0, 6.0, 6.0];
        const ds: SparkResult = renderDownsampled(raw[0..8], 4, true);
        expect_eq(ds.len, 4 as usize);
        expect_eq(ds.asSlice(), ".-+#");
    });

    test("6. @cap/sparkline — series stats & empty input", () => {
        const vals: [4]f64 = [10.0, 20.0, 30.0, 40.0];
        const st: SeriesStats = computeStats(vals[0..4]);
        expect_eq(st.hasValues, true);
        expect_eq(st.count, 4 as usize);
        expect_eq(st.minVal, 10.0);
        expect_eq(st.maxVal, 40.0);
        expect_eq(st.mean, 25.0);

        const emptyRes: SparkResult = render(vals[0..0]);
        expect_eq(emptyRes.len, 0 as usize);
        const emptySt: SeriesStats = computeStats(vals[0..0]);
        expect_eq(emptySt.hasValues, false);
    });
}
