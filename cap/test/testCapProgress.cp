import { test, expect_eq } from "cap:test";
import {
    newProgressBar,
    renderProgress,
} from "../../packages/@cap/progress/src/index.cp";

fn main() {
    test("1. @cap/progress — renders 0%, 50%, and 100% bars accurately", () => {
        let bar = newProgressBar(100, 10);
        let buf: [128]u8 = [0; 128];
        const n0 = renderProgress(bar, "[:bar] :percent", buf[0..128]);
        expect_eq(buf[0..n0] as string, "[----------] 0%");

        bar.tick(50);
        const n50 = renderProgress(bar, "[:bar] :percent (:current/:total)", buf[0..128]);
        expect_eq(buf[0..n50] as string, "[====>-----] 50% (50/100)");

        bar.tick(50);
        expect_eq(bar.isComplete(), true);
        const n100 = renderProgress(bar, "[:bar] :percent", buf[0..128]);
        expect_eq(buf[0..n100] as string, "[==========] 100%");
    });

    test("2. @cap/progress — clamps tick beyond total", () => {
        let bar = newProgressBar(10, 10);
        bar.tick(25);
        expect_eq(bar.current, 10 as u64);
        expect_eq(bar.percent(), 100 as u32);
    });

    test("3. @cap/progress — computes ETA and elapsed seconds", () => {
        let bar = newProgressBar(100, 10);
        bar.tick(25);
        bar.elapsed_ms = 2000; // 2.0s elapsed for 25% -> 6.0s remaining
        let buf: [128]u8 = [0; 128];
        const n = renderProgress(bar, "eta=:etas elapsed=:elapseds", buf[0..128]);
        expect_eq(buf[0..n] as string, "eta=6.0s elapsed=2.0s");
    });

    test("4. @cap/progress — handles zero total gracefully as 100%", () => {
        const bar = newProgressBar(0, 5);
        expect_eq(bar.isComplete(), true);
        expect_eq(bar.percent(), 100 as u32);
        let buf: [64]u8 = [0; 64];
        const n = renderProgress(bar, "[:bar]", buf[0..64]);
        expect_eq(buf[0..n] as string, "[=====]");
    });

    test("5. @cap/progress — preserves unrecognized colon text", () => {
        let bar = newProgressBar(10, 4);
        bar.tick(5);
        let buf: [64]u8 = [0; 64];
        const n = renderProgress(bar, "step: :current/:total :unknown", buf[0..64]);
        expect_eq(buf[0..n] as string, "step: 5/10 :unknown");
    });

    test("6. @cap/progress — supports custom complete/incomplete characters", () => {
        let bar = newProgressBar(4, 4);
        bar.complete_char = 35;   // '#'
        bar.incomplete_char = 46; // '.'
        bar.head_char = 0;
        bar.tick(3);
        let buf: [64]u8 = [0; 64];
        const n = renderProgress(bar, "[:bar] :percent", buf[0..64]);
        expect_eq(buf[0..n] as string, "[###.] 75%");
    });
}
