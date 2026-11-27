// Type definitions for CP

import {  } from "std";
export fn cp_yyjson_read(dat: [*]&u8!, len: usize, flg: u32): *auto!?;
export fn cp_yyjson_doc_free(doc: *auto!?);
export fn cp_yyjson_doc_get_root(doc: *auto!?): *auto!?;
export fn cp_yyjson_doc_get_read_size(doc: *auto!?): usize;
export fn cp_yyjson_doc_get_val_count(doc: *auto!?): usize;
export fn cp_yyjson_write(doc: *auto!?, flg: u32, len: *usize!?): [*]u8!?;
export fn cp_yyjson_is_null(val: *auto!?): bool;
export fn cp_yyjson_is_bool(val: *auto!?): bool;
export fn cp_yyjson_is_int(val: *auto!?): bool;
export fn cp_yyjson_is_real(val: *auto!?): bool;
export fn cp_yyjson_is_str(val: *auto!?): bool;
export fn cp_yyjson_is_arr(val: *auto!?): bool;
export fn cp_yyjson_is_obj(val: *auto!?): bool;
export fn cp_yyjson_get_bool(val: *auto!?): bool;
export fn cp_yyjson_get_sint(val: *auto!?): i64;
export fn cp_yyjson_get_real(val: *auto!?): f64;
export fn cp_yyjson_get_str(val: *auto!?): [*:0]&u8!?;
export fn cp_yyjson_get_len(val: *auto!?): usize;
export fn cp_yyjson_obj_size(obj: *auto!?): usize;
export fn cp_yyjson_obj_getn(obj: *auto!?, key: [*]&u8!, key_len: usize): *auto!?;
export fn cp_yyjson_arr_size(arr: *auto!?): usize;
export fn cp_yyjson_arr_get(arr: *auto!?, idx: usize): *auto!?;
export fn cp_yyjson_free_str(ptr: *auto!?);
export error YyjsonError { ParseFailed, WriteFailed, OutOfMemory }

export class JsonVal {
    raw: *auto!?,
    fn isNull(): bool;
    fn isBool(): bool;
    fn isInt(): bool;
    fn isReal(): bool;
    fn isStr(): bool;
    fn isArr(): bool;
    fn isObj(): bool;
    fn asBool(): bool?;
    fn asInt(): i64?;
    fn asReal(): f64?;
    fn asStr(): string?;
    fn objLen(): usize;
    fn get(key: &[]u8): JsonVal?;
    fn arrLen(): usize;
    fn arrGet(idx: usize): JsonVal?;
}

export class JsonDoc {
    doc_ptr: *auto!?,
    fn parse(input: &[]u8): !JsonDoc;
    fn parseWithFlags(input: &[]u8, flags: u32): !JsonDoc;
    fn root(): JsonVal;
    fn valCount(): usize;
    fn readSize(): usize;
    fn stringify(allocator: std.mem.Allocator, pretty: bool): ![]u8;
    fn __cp_deinit(_: &mut JsonDoc);
}

