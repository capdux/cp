// Type definitions for CP

import { prependHttp, hasScheme, isProtocolRelative } from "../../prepend-http/src/index.cp";
import { trim, strEq, eqIgnoreCase, findByte, rfindByte, toLower } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class NormalizeUrlOptions {
    default_https: bool,
    strip_www: bool,
    strip_hash: bool,
    remove_trailing_slash: bool,
    remove_default_port: bool,
    normalize_protocol: bool,
}

export class NurlSegment {
    start: usize,
    end: usize,
}

export fn defaultNormalizeUrlOptions(): NormalizeUrlOptions;
export fn nurlWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn nurlWriteLower(out: []u8, pos: usize, s: string): usize;
export fn nurlFindSchemeSep(s: string): usize;
export fn nurlIsDefaultPort(scheme: string, port: string): bool;
export fn nurlNormalizePath(raw_path: string, remove_trailing_slash: bool, has_query_or_hash: bool, out: []u8, start_pos: usize): usize;
export fn normalizeUrlWithOptions(url: string, opts: NormalizeUrlOptions, out: []u8): usize;
export fn normalizeUrl(url: string, out: []u8): usize;
