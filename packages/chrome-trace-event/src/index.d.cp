// Type definitions for CP

import { hexCharLower } from "cap:ascii";
import { writeU64 } from "cap:fmt";
export enum TracePhase {
    Begin,
    End,
    Complete,
    Instant,
    Counter,
    Metadata
}

export class TraceEvent {
    name: string,
    cat: string,
    ph: TracePhase,
    ts_us: u64,
    dur_us: u64,
    pid: u32,
    tid: u32,
}

export fn phaseCode(ph: TracePhase): u8;
export fn ctevWriteByte(out: []u8, pos: i64, b: u8): i64;
export fn ctevWriteStr(out: []u8, pos: i64, s: string): i64;
export fn ctevWriteJsonStr(out: []u8, pos: i64, s: string): i64;
export fn ctevWriteU64(out: []u8, pos: i64, val: u64): i64;
export fn formatTraceEvent(ev: TraceEvent, out: []u8): usize;
export fn formatTraceArray(events: &[]TraceEvent, out: []u8): usize;
