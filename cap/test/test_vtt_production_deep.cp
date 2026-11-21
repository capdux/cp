// Source: cp/cap/test/test_vtt_production_deep.cp (Production deep tests for cap:vtt)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VttCue,
    isWebVttHeader,
    parseVttTimestampMs,
    formatVttTimestampMs,
    VttCueIter,
    formatVttCue,
} from "cap:vtt";

test("1. isWebVttHeader validates BOM, header title, and rejects malformed prefixes", () => {
    expect_eq(isWebVttHeader("WEBVTT"), true);
    expect_eq(isWebVttHeader("WEBVTT\n\n00:00.000 --> 00:01.000\nHi"), true);
    expect_eq(isWebVttHeader("WEBVTT - Chapter 1\r\n"), true);
    expect_eq(isWebVttHeader("\xEF\xBB\xBFWEBVTT\tTitle\n"), true);

    expect_eq(isWebVttHeader(""), false);
    expect_eq(isWebVttHeader("WEBVTTX\n"), false);
    expect_eq(isWebVttHeader("webvtt\n"), false);
    expect_eq(isWebVttHeader("1\n00:00:01,000 --> 00:00:02,000\nSRT"), false);
});

test("2. parseVttTimestampMs & formatVttTimestampMs round-trip and strict validation", () => {
    // MM:SS.mmm & HH:MM:SS.mmm
    expect_eq(parseVttTimestampMs("00:00.000") ?? 99, 0);
    expect_eq(parseVttTimestampMs("01:02.345") ?? 0, 62345);
    expect_eq(parseVttTimestampMs("02:03:04.567") ?? 0, 7384567);
    expect_eq(parseVttTimestampMs("100:59:59.999") ?? 0, 363599999);

    // Format round-trip
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    const s1 = formatVttTimestampMs(62345, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, s1, "00:01:02.345"), true);
    expect_eq(parseVttTimestampMs(s1) ?? 0, 62345);

    const s2 = formatVttTimestampMs(363599999, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, s2, "100:59:59.999"), true);
    expect_eq(parseVttTimestampMs(s2) ?? 0, 363599999);

    // Reject invalid timestamps
    expect_eq(parseVttTimestampMs("00:60.000") == null, true); // sec >= 60
    expect_eq(parseVttTimestampMs("60:00.000") == null, true); // min >= 60
    expect_eq(parseVttTimestampMs("01:60:00.000") == null, true); // min >= 60
    expect_eq(parseVttTimestampMs("00:01.12") == null, true); // 2-digit ms
    expect_eq(parseVttTimestampMs("00:01.1234") == null, true); // 4-digit ms
    expect_eq(parseVttTimestampMs("00:01,123") == null, true); // SRT comma
    expect_eq(parseVttTimestampMs("1:02:03.456") == null, true); // 1-digit hour

    // Format buffer too small
    expect_eq(formatVttTimestampMs(0, buf[0..11]) == null, true);
});

test("3. VttCueIter skips NOTE/STYLE/REGION blocks and parses cues with id, settings & multiline payload", () => {
    const sample =
        "\xEF\xBB\xBFWEBVTT - Demo\n\n" ++
        "STYLE\n::cue { color: yellow; }\n\n" ++
        "REGION\nid:top\nwidth:40%\n\n" ++
        "NOTE This is a comment block\nspanning two lines.\n\n" ++
        "intro-1\n" ++
        "00:01.500 --> 00:04.250 align:start position:10%\n" ++
        "Hello CP World!\n" ++
        "Second subtitle line.\n\n" ++
        "00:01:00.000 --> 00:01:05.000\n" ++
        "Cue without explicit ID.";

    let it = VttCueIter.init(sample);

    const c1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, c1.id ?? "", "intro-1"), true);
    expect_eq(c1.start_ms, 1500);
    expect_eq(c1.end_ms, 4250);
    expect_eq(c1.durationMs(), 2750);
    expect_eq(std.mem.eql(u8, c1.settings, "align:start position:10%"), true);
    expect_eq(std.mem.eql(u8, c1.payload, "Hello CP World!\nSecond subtitle line."), true);

    const c2 = it.next() ?? unreachable;
    expect_eq(c2.id == null, true);
    expect_eq(c2.start_ms, 60000);
    expect_eq(c2.end_ms, 65000);
    expect_eq(c2.durationMs(), 5000);
    expect_eq(std.mem.eql(u8, c2.settings, ""), true);
    expect_eq(std.mem.eql(u8, c2.payload, "Cue without explicit ID."), true);

    expect_eq(it.next() == null, true);
});

test("4. VttCueIter handles CRLF line endings and skips malformed cue blocks gracefully", () => {
    const crlf =
        "WEBVTT\r\n\r\n" ++
        "bad-order\r\n00:05.000 --> 00:02.000\r\nReversed timestamps skipped\r\n\r\n" ++
        "ok-cue\r\n00:02.000 --> 00:05.500\r\nValid CRLF cue\r\n";

    let it = VttCueIter.init(crlf);
    const cue = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, cue.id ?? "", "ok-cue"), true);
    expect_eq(cue.start_ms, 2000);
    expect_eq(cue.end_ms, 5500);
    expect_eq(std.mem.eql(u8, cue.payload, "Valid CRLF cue"), true);
    expect_eq(it.next() == null, true);
});

test("5. formatVttCue round-trips and enforces buffer bounds + saturating durationMs", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    const cue = VttCue {
        id: "cue-42",
        start_ms: 1234,
        end_ms: 5678,
        settings: "line:80%",
        payload: "Line A\nLine B",
    };

    const formatted = formatVttCue(cue, buf[0..]) ?? unreachable;
    const expected =
        "cue-42\n" ++
        "00:00:01.234 --> 00:00:05.678 line:80%\n" ++
        "Line A\nLine B";
    expect_eq(std.mem.eql(u8, formatted, expected), true);

    // Round-trip via VttCueIter
    let it = VttCueIter.init(formatted);
    const reparsed = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, reparsed.id ?? "", "cue-42"), true);
    expect_eq(reparsed.start_ms, 1234);
    expect_eq(reparsed.end_ms, 5678);
    expect_eq(std.mem.eql(u8, reparsed.settings, "line:80%"), true);
    expect_eq(std.mem.eql(u8, reparsed.payload, "Line A\nLine B"), true);

    // Buffer too small returns null
    expect_eq(formatVttCue(cue, buf[0..16]) == null, true);

    // Saturating durationMs when end_ms < start_ms
    const inverted = VttCue { start_ms: 5000, end_ms: 1000 };
    expect_eq(inverted.durationMs(), 0);
});
