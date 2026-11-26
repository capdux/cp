// Type definitions for CP

import { ParsedMediaType } from "../../mediatype/src/index.cp";
import { eqAscii } from "../../unicase/src/index.cp";
export class MultipartPart {
    name: string,
    filename: string,
    content_type: string,
    data: string,
    is_file: bool,
}

export fn mpartIsSpace(c: u8): bool;
export fn mpartTrim(s: string): string;
export fn mpartMatchAt(haystack: string, pos: usize, needle: string): bool;
export fn mpartIsBoundaryAt(body: string, pos: usize, boundary: string): bool;
export fn mpartFindFirstBoundary(body: string, boundary: string): usize;
export fn mpartFindNextBoundary(body: string, start: usize, boundary: string, data_end_out: &mut usize, next_bnd_out: &mut usize): bool;
export fn mpartSkipLineEnd(body: string, pos: &mut usize): bool;
export fn mpartParseDisposition(val: string, part: &mut MultipartPart);
export fn mpartWriteStr(out: []u8, pos: usize, s: string): usize;
export fn extractBoundary(content_type: string): string?;
export fn formatContentTypeHeader(boundary: string, out: []u8): usize;
export class MultipartReader {
    boundary: string,
    body: string,
    cursor: usize,
    finished: bool,
    fn new(boundary: string, body: string): MultipartReader;
    fn nextPart(_: auto): MultipartPart?;
    fn countParts(): usize;
}

export class MultipartWriter {
    boundary: string,
    fn new(boundary: string): MultipartWriter;
    fn writeField(name: string, value: string, out: []u8, pos: usize): usize;
    fn writeFile(name: string, filename: string, content_type: string, data: string, out: []u8, pos: usize): usize;
    fn finish(out: []u8, pos: usize): usize;
}

