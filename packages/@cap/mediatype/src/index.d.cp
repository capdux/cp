// Type definitions for CP

import { MediaType, parseMediaType, formatMediaType, mediaTypeEq, mtPushLower } from "../../media-typer/src/index.cp";
import { eqAscii } from "../../unicase/src/index.cp";
export class MediaParam {
    name: string,
    value: string,
}

export fn mtyTrim(s: string): string;
export fn mtyPushStr(out: []u8, pos: usize, s: string): usize;
export class ParsedMediaType {
    valid: bool,
    top_level: string,
    sub_level: string,
    suffix: string,
    params: [8]MediaParam,
    param_count: usize,
    fn invalid(): ParsedMediaType;
    fn parse(raw: string): ParsedMediaType;
    fn getParam(key: string): string?;
    fn hasSuffix(suf: string): bool;
    fn formatEssence(out: []u8): usize;
    fn formatFull(out: []u8): usize;
    fn isJson(): bool;
    fn isText(): bool;
}

export fn essenceEq(a: string, b: string): bool;
