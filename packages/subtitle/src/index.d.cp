// Type definitions for CP

import { trim, findByte } from "cap:ascii";
import { indexOfBytes } from "cap:bin";
import { writeStr, writeU64, parseU64 } from "cap:fmt";
export enum SubtitleFormat {
    Srt,
    WebVtt
}

export class Cue {
    index: u32,
    start_ms: i64,
    end_ms: i64,
    text: string,
}

export class CueList {
    cues: [64]Cue,
    count: usize,
    format: SubtitleFormat,
}

export fn parseUint(s: string): i64?;
export fn parseTimestampMs(raw: string): i64?;
export fn put2(out: []u8, pos: &mut usize, v: i64);
export fn put3(out: []u8, pos: &mut usize, v: i64);
export fn formatTimestampMs(ms_in: i64, fmt: SubtitleFormat, out: []u8): string;
export fn parseSubtitles(input: string): CueList;
export fn shiftCues(list: &mut CueList, offset_ms: i64);
export fn formatSubtitles(list: CueList, target_fmt: SubtitleFormat, out: []u8): string;
