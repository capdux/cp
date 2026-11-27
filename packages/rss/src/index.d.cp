// Type definitions for CP

import { isSpace, trim } from "cap:ascii";
import { startsWithBytes, endsWithBytes, findByteInBytes } from "cap:bin";
import { writeStr } from "cap:fmt";
export class RssChannel {
    title: &[]u8,
    link: &[]u8,
    description: &[]u8,
    language: &[]u8,
}

export class RssItem {
    title: &[]u8,
    link: &[]u8,
    description: &[]u8,
    guid: &[]u8,
    pub_date: &[]u8,
}

export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;
export fn pushXmlEscaped(dst: []u8, pos: usize, src: &[]u8): i64;
export fn pushTag(dst: []u8, pos: usize, tag: &[]u8, val: &[]u8): i64;
export class RssWriter {
    buf: []u8,
    pos: usize,
    item_count: usize,
    failed: bool,
    finished: bool,
    fn init(buf: []u8, ch: RssChannel): RssWriter;
    fn addItem(_: auto, item: RssItem): bool;
    fn finish(_: auto): i64;
    fn itemCount(): usize;
    fn written(): &[]u8;
}

export fn extractTagValue(xml: &[]u8, tag: &[]u8, start_pos: usize): &[]u8;
export fn countItems(xml: &[]u8): usize;
