// Type definitions for CP

import { strEq, findByte } from "cap:ascii";
import { AsciiSet, componentSet, percentEncode, percentDecodePlus } from "../../percent-encoding/src/index.cp";
export class QsPair {
    key: string,
    value: string,
}

export class QsPath {
    segments: [8]string,
    depth: usize,
}

export fn sqsKeyEncodeSet(): AsciiSet;
export fn parseKeyPath(key: string): QsPath;
export fn parseQs(query: string, out_pairs: []QsPair): usize;
export fn getQsValueDecoded(query: string, target_key: string, out: []u8): usize?;
export fn serializeQs(pairs: &[]QsPair, out: []u8): usize;
