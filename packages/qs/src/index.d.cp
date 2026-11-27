// Type definitions for CP

import { hexVal, isAlnum, hexCharUpper, findByte } from "cap:ascii";
export error QsError { BufferTooSmall, TooManyParams, DepthExceeded, InvalidPercentEncoding, InvalidSyntax }

export class QsParseOptions {
    max_params: usize,
    max_depth: usize,
    allow_dots: bool,
    comma_arrays: bool,
    ignore_query_prefix: bool,
    plus_as_space: bool,
    strict_null_handling: bool,
}

export class QsPathSegment {
    name: &[]u8,
    is_index: bool,
    index: usize,
    is_empty_bracket: bool,
    name_off: u16,
    name_len: u16,
    name_in_pool: bool,
}

export class QsEntry {
    raw_key: &[]u8,
    decoded_key: &[]u8,
    decoded_value: &[]u8,
    is_null: bool,
    segments: [8]QsPathSegment,
    depth: usize,
    key_off: u16,
    key_len: u16,
    key_in_pool: bool,
    val_off: u16,
    val_len: u16,
    val_in_pool: bool,
}

export fn isUnreserved(c: u8): bool;
export fn needsDecode(s: &[]u8, plus_as_space: bool): bool;
export fn parseDecimalIndex(s: &[]u8): usize?;
export fn percentDecode(input: &[]u8, plus_as_space: bool, out: []u8): &[]u8 ! QsError;
export fn percentEncode(input: &[]u8, space_as_plus: bool, out: []u8): &[]u8 ! QsError;
export fn makeSegment(slice: &[]u8, base_off: u16, seg_rel_off: usize, in_pool: bool, is_empty_bracket: bool): QsPathSegment;
export class QsParsed {
    pool: [4096]u8,
    pool_len: usize,
    entries: [64]QsEntry,
    count: usize,
    fn init(): QsParsed;
    fn parse(input: &[]u8, opts: QsParseOptions): QsParsed ! QsError;
    fn parseInto(_: auto, input: &[]u8, opts: QsParseOptions): void ! QsError;
    fn resolveKey(_: auto, idx: usize): &[]u8;
    fn resolveVal(_: auto, idx: usize): &[]u8;
    fn resolveSegName(_: auto, entry_idx: usize, seg_idx: usize): &[]u8;
    fn len(_: auto): usize;
    fn entryAt(_: auto, idx: usize): QsEntry?;
    fn entryKey(_: auto, idx: usize): (&[]u8)?;
    fn entryValue(_: auto, idx: usize): (&[]u8)?;
    fn isNull(_: auto, flat_key: &[]u8): bool;
    fn get(_: auto, flat_key: &[]u8): (&[]u8)?;
    fn getNested2(_: auto, k1: &[]u8, k2: &[]u8): (&[]u8)?;
    fn getNested3(_: auto, k1: &[]u8, k2: &[]u8, k3: &[]u8): (&[]u8)?;
    fn getArrayItem(_: auto, key: &[]u8, idx: usize): (&[]u8)?;
    fn countArrayItems(_: auto, key: &[]u8): usize;
}

export fn parseKeySegments(dec_key: &[]u8, key_off: u16, key_in_pool: bool, allow_dots: bool, max_depth: usize, out_segs: &mut [8]QsPathSegment): usize ! QsError;
export class QsBuilder {
    buf: []u8,
    pos: usize,
    count: usize,
    add_query_prefix: bool,
    space_as_plus: bool,
    fn init(buf: []u8): QsBuilder;
    fn initWithOptions(buf: []u8, add_query_prefix: bool, space_as_plus: bool): QsBuilder;
    fn writeRawByte(_: auto, b: u8): void ! QsError;
    fn writeRawSlice(_: auto, s: &[]u8): void ! QsError;
    fn writeEncoded(_: auto, s: &[]u8): void ! QsError;
    fn writePrefixOrSep(_: auto): void ! QsError;
    fn add(_: auto, key: &[]u8, val: &[]u8): void ! QsError;
    fn addNested2(_: auto, k1: &[]u8, k2: &[]u8, val: &[]u8): void ! QsError;
    fn addNested3(_: auto, k1: &[]u8, k2: &[]u8, k3: &[]u8, val: &[]u8): void ! QsError;
    fn addArrayItem(_: auto, key: &[]u8, idx: usize, val: &[]u8): void ! QsError;
    fn addBracketArrayItem(_: auto, key: &[]u8, val: &[]u8): void ! QsError;
    fn addNull(_: auto, key: &[]u8): void ! QsError;
    fn written(): &[]u8;
    fn reset(_: auto);
}

