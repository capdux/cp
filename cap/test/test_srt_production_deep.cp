// Source: cp/cap/test/test_srt_production_deep.cp (Production deep tests for cap:srt)

import { test, expect_eq } from "cap:test";
import {
    SrtCue,
    parseSrtTimestampMs,
    formatSrtTimestampMs,
    SrtCueIter,
    formatSrtCue,
    shiftSrtCue,
} from "cap:srt";

fn main() {
    test("1. parseSrtTimestampMs parses comma/dot timestamps and rejects invalid ranges/formats", () => {
        expect_eq(parseSrtTimestampMs("00:00:00,000").?, 0 as u64);
        expect_eq(parseSrtTimestampMs("01:02:03,456").?, 3723456 as u64);
        expect_eq(parseSrtTimestampMs(" 00:59:59.999 ").?, 3599999 as u64);
        expect_eq(parseSrtTimestampMs("100:00:00,001").?, 360000001 as u64);

        // Invalid minutes/seconds/millis/format
        expect_eq(parseSrtTimestampMs("00:60:00,000") == null, true);
        expect_eq(parseSrtTimestampMs("00:00:60,000") == null, true);
        expect_eq(parseSrtTimestampMs("00:00:01,12") == null, true);
        expect_eq(parseSrtTimestampMs("00:00:01,1234") == null, true);
        expect_eq(parseSrtTimestampMs("0:00:01,000") == null, true);
        expect_eq(parseSrtTimestampMs("00:00:01:000") == null, true);
        expect_eq(parseSrtTimestampMs("not-a-time") == null, true);
    });

    test("2. formatSrtTimestampMs formats standard HH:MM:SS,mmm and guards small buffers", () => {
        let buf: [32]u8 = undefined;
        expect_eq(formatSrtTimestampMs(0, &buf).?, "00:00:00,000");
        expect_eq(formatSrtTimestampMs(3723456, &buf).?, "01:02:03,456");
        expect_eq(formatSrtTimestampMs(360000001, &buf).?, "100:00:00,001");

        let tiny: [11]u8 = undefined;
        expect_eq(formatSrtTimestampMs(1000, &tiny) == null, true);
    });

    test("3. SrtCueIter handles UTF-8 BOM, CRLF/LF, multiline text, and coordinate suffixes", () => {
        const srt =
            "\xEF\xBB\xBF1\r\n" ++
            "00:00:01,000 --> 00:00:03,500 X1:10 X2:200\r\n" ++
            "Hello, world!\r\n" ++
            "Second line.\r\n" ++
            "\r\n" ++
            "\r\n" ++
            "2\n" ++
            "00:00:04.200 --> 00:00:06.000\n" ++
            "Single line cue\n";

        let it = SrtCueIter.init(srt);

        const c1_opt = it.next();
        expect_eq(c1_opt != null, true);
        if (c1 := c1_opt) {
            expect_eq(c1.seq, 1 as u32);
            expect_eq(c1.start_ms, 1000 as u64);
            expect_eq(c1.end_ms, 3500 as u64);
            expect_eq(c1.durationMs(), 2500 as u64);
            expect_eq(c1.text, "Hello, world!\r\nSecond line.");
        }

        const c2_opt = it.next();
        expect_eq(c2_opt != null, true);
        if (c2 := c2_opt) {
            expect_eq(c2.seq, 2 as u32);
            expect_eq(c2.start_ms, 4200 as u64);
            expect_eq(c2.end_ms, 6000 as u64);
            expect_eq(c2.durationMs(), 1800 as u64);
            expect_eq(c2.text, "Single line cue");
        }

        expect_eq(it.next() == null, true);
    });

    test("4. SrtCueIter skips malformed blocks and reversed timestamps cleanly", () => {
        const mixed =
            "not-a-seq\n" ++
            "00:00:01,000 --> 00:00:02,000\n" ++
            "bad seq\n\n" ++
            "2\n" ++
            "00:00:05,000 --> 00:00:02,000\n" ++
            "reversed range\n\n" ++
            "3\n" ++
            "missing arrow line\n\n" ++
            "4\n" ++
            "00:00:10,000 --> 00:00:12,250\n" ++
            "Valid survivor\n";

        let it = SrtCueIter.init(mixed);
        const c_opt = it.next();
        expect_eq(c_opt != null, true);
        if (c := c_opt) {
            expect_eq(c.seq, 4 as u32);
            expect_eq(c.start_ms, 10000 as u64);
            expect_eq(c.end_ms, 12250 as u64);
            expect_eq(c.text, "Valid survivor");
        }
        expect_eq(it.next() == null, true);
    });

    test("5. shiftSrtCue shifts positive/negative with saturation and formatSrtCue roundtrips", () => {
        const base = SrtCue{
            seq: 7,
            start_ms: 1500,
            end_ms: 4000,
            text: "Line 1\nLine 2",
        };

        const fwd = shiftSrtCue(base, 2500);
        expect_eq(fwd.start_ms, 4000 as u64);
        expect_eq(fwd.end_ms, 6500 as u64);
        expect_eq(fwd.durationMs(), 2500 as u64);

        // Negative shift clamps start_ms to 0, end_ms to 2000
        const back = shiftSrtCue(base, -2000);
        expect_eq(back.start_ms, 0 as u64);
        expect_eq(back.end_ms, 2000 as u64);

        // Large negative shift clamps both to 0
        const clamped = shiftSrtCue(base, -999999);
        expect_eq(clamped.start_ms, 0 as u64);
        expect_eq(clamped.end_ms, 0 as u64);
        expect_eq(clamped.durationMs(), 0 as u64);

        let buf: [256]u8 = undefined;
        const formatted = formatSrtCue(fwd, &buf);
        expect_eq(formatted != null, true);
        if (out := formatted) {
            expect_eq(out, "7\n00:00:04,000 --> 00:00:06,500\nLine 1\nLine 2\n");
        }

        let tiny: [10]u8 = undefined;
        expect_eq(formatSrtCue(fwd, &tiny) == null, true);
    });
}
