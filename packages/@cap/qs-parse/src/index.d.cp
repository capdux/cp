// Type definitions for CP

import { parseKeyPath, QsPair, QsPath } from "../../serde-qs/src/index.cp";
import { AsciiSet, componentSet, percentEncode, percentDecodePlus } from "../../percent-encoding/src/index.cp";
export class QsOptions {
    delimiter: u8,
    allow_dots: bool,
    max_depth: usize,
    array_format: u8,
    add_query_prefix: bool,
    fn defaultOpts(): QsOptions;
}

export fn qspFindByte(s: string, start: usize, end: usize, target: u8): usize;
export fn qspWriteByte(dst: []u8, pos: usize, ch: u8): usize;
export fn qspWriteUsize(dst: []u8, pos: usize, val: usize): usize;
export fn qspKeyEncodeSet(): AsciiSet;
export fn qspCountDotSegments(s: string): usize;
export fn qspMeasureKeyDepth(key: string, allow_dots: bool): usize;
export fn parseKeyPathWithDots(key: string, allow_dots: bool): QsPath;
export fn parseQuery(query: string, opts: QsOptions, out_pairs: []QsPair): usize;
export fn stringifyPairs(pairs: &[]QsPair, opts: QsOptions, out: []u8): usize;
export fn stringifyArray(key: string, values: &[]string, array_format: u8, out: []u8): usize;
export fn decodeQueryValue(raw: string, out: []u8): usize;
