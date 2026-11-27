// Type definitions for CP

import { eqIgnoreCase, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class IcalEvent {
    uid: &[]u8,
    dtstart: &[]u8,
    dtend: &[]u8,
    summary: &[]u8,
    location: &[]u8,
    description: &[]u8,
}

export class ParsedEvent {
    uid_start: usize,
    uid_len: usize,
    dtstart_start: usize,
    dtstart_len: usize,
    dtend_start: usize,
    dtend_len: usize,
    summary_start: usize,
    summary_len: usize,
    location_start: usize,
    location_len: usize,
    valid: bool,
}

export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;
export fn pushEscapedText(dst: []u8, pos: usize, src: &[]u8): i64;
export fn pushPropRaw(dst: []u8, pos: usize, prop: &[]u8, val: &[]u8): i64;
export fn pushPropText(dst: []u8, pos: usize, prop: &[]u8, val: &[]u8): i64;
export class IcalWriter {
    buf: []u8,
    pos: usize,
    event_count: usize,
    failed: bool,
    finished: bool,
    fn init(buf: []u8, prod_id: &[]u8): IcalWriter;
    fn addEvent(_: auto, ev: IcalEvent): bool;
    fn finish(_: auto): i64;
    fn eventCount(): usize;
    fn written(): &[]u8;
}

export fn emptyParsedEvent(): ParsedEvent;
export fn parseFirstEvent(ics: &[]u8): ParsedEvent;
export fn unescapeText(src: &[]u8, dst: []u8): i64;
