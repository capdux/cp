// Type definitions for CP

import { parseAcceptList } from "../../negotiator/src/core.cp";

export class AcceptHeaders {
    accept: []const u8,
    accept_encoding: []const u8,
    accept_charset: []const u8,
    accept_language: []const u8,
    static fn empty(): AcceptHeaders;
}

export class Accepts {
    headers: AcceptHeaders,
    static fn new(headers: AcceptHeaders): Accepts;
    fn typeIndex(available: []const []const u8): i64;
    fn types(available: []const []const u8): ([]const u8)?;
    fn encodingIndex(available: []const []const u8): i64;
    fn encodings(available: []const []const u8): ([]const u8)?;
    fn charsetIndex(available: []const []const u8): i64;
    fn charsets(available: []const []const u8): ([]const u8)?;
    fn languageIndex(available: []const []const u8): i64;
    fn languages(available: []const []const u8): ([]const u8)?;
}

export fn parseQValue(s_in: []const u8): i32;
export fn negotiateEncoding(accept_header: []const u8, available: []const []const u8): i64;
export fn negotiateLanguage(accept_header: []const u8, available: []const []const u8): i64;
export fn negotiateMediaType(accept_header: []const u8, available: []const []const u8): i64;
export fn negotiateCharset(accept_charset: []const u8, available: []const []const u8): i64;
export fn rankMediaTypes(accept_header: []const u8, available: []const []const u8, out_indices: []usize): usize;
