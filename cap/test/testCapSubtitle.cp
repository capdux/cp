// cap/test/testCapSubtitle.cp
// Comprehensive unit tests for @cap/subtitle

import { test, expect_eq } from "cap:test";
import {
    SubtitleFormat,
    CueList,
    parseTimestampMs,
    formatTimestampMs,
    parseSubtitles,
    shiftCues,
    formatSubtitles,
} from "../../packages/subtitle/src/index.cp";

fn main() {
    test("1. @cap/subtitle — SRT & WebVTT timestamp parsing", () => {
        expect_eq(parseTimestampMs("00:01:02,345") ?? -1, 62345 as i64);
        expect_eq(parseTimestampMs("01:02:03.500") ?? -1, 3723500 as i64);
        expect_eq(parseTimestampMs("02:15.250") ?? -1, 135250 as i64);
        expect_eq(parseTimestampMs("bad") == null, true);
    });

    test("2. @cap/subtitle — timestamp formatting for SRT & WebVTT", () => {
        let buf: [32]u8 = undefined;
        expect_eq(formatTimestampMs(62345, SubtitleFormat.Srt, buf[0..32]), "00:01:02,345");
        expect_eq(formatTimestampMs(62345, SubtitleFormat.WebVtt, buf[0..32]), "00:01:02.345");
        expect_eq(formatTimestampMs(-500, SubtitleFormat.Srt, buf[0..32]), "00:00:00,000");
    });

    test("3. @cap/subtitle — parse multi-cue SRT", () => {
        const srt: string = "1\n00:00:01,000 --> 00:00:03,500\nHello World\n\n2\n00:00:04,000 --> 00:00:06,250\nLine 1\nLine 2\n";
        const list: CueList = parseSubtitles(srt);
        expect_eq(list.count, 2 as usize);
        expect_eq(list.cues[0].start_ms, 1000 as i64);
        expect_eq(list.cues[0].end_ms, 3500 as i64);
        expect_eq(list.cues[0].text, "Hello World");
        expect_eq(list.cues[1].start_ms, 4000 as i64);
        expect_eq(list.cues[1].text, "Line 1\nLine 2");
    });

    test("4. @cap/subtitle — parse WebVTT with short timestamps & cue settings", () => {
        const vtt: string = "WEBVTT\n\n00:02.500 --> 00:05.000 align:start position:0%\nWelcome to Capdux!\n";
        const list: CueList = parseSubtitles(vtt);
        expect_eq(list.count, 1 as usize);
        expect_eq(list.cues[0].start_ms, 2500 as i64);
        expect_eq(list.cues[0].end_ms, 5000 as i64);
        expect_eq(list.cues[0].text, "Welcome to Capdux!");
    });

    test("5. @cap/subtitle — cue time-shifting & clamping", () => {
        let list: CueList = parseSubtitles("1\n00:00:02,000 --> 00:00:05,000\nSync me\n");
        shiftCues(&list, 1500);
        expect_eq(list.cues[0].start_ms, 3500 as i64);
        expect_eq(list.cues[0].end_ms, 6500 as i64);

        shiftCues(&list, -5000);
        expect_eq(list.cues[0].start_ms, 0 as i64);
        expect_eq(list.cues[0].end_ms, 1500 as i64);
    });

    test("6. @cap/subtitle — SRT to WebVTT conversion", () => {
        const srt: string = "1\n00:00:01,200 --> 00:00:03,400\nHi!\n";
        const list: CueList = parseSubtitles(srt);
        let out: [512]u8 = undefined;
        const vtt: string = formatSubtitles(list, SubtitleFormat.WebVtt, out[0..512]);
        expect_eq(vtt, "WEBVTT\n\n00:00:01.200 --> 00:00:03.400\nHi!\n");

        const backSrt: string = formatSubtitles(list, SubtitleFormat.Srt, out[0..512]);
        expect_eq(backSrt, "1\n00:00:01,200 --> 00:00:03,400\nHi!\n");
    });
}
