// Type definitions for CP

import { AsciiSet, componentSet, percentEncode, percentDecodePlus } from "../../percent-encoding/src/index.cp";
export class QsPair {
    key: string,
    value: string,
}

export class QsPath {
    segments: [8]string,
    depth: usize,
}

export fn sqsStrEq(a: string, b: string): bool;
export fn sqsFindByte(s: string, start: usize, end: usize, target: u8): usize;
export fn sqsNeedsDecode(s: string): bool;
export fn sqsKeyEncodeSet(): AsciiSet;
export fn parseKeyPath(key: string): QsPath;
export fn parseQs(query: string, out_pairs: []QsPair): usize;
export fn getQsValueDecoded(query: string, target_key: string, out: []u8): usize?;
export fn serializeQs(pairs: &[]QsPair, out: []u8): usize;
