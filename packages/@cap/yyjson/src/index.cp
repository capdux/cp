// @cap/yyjson — High-performance JSON reader/writer (yyjson v0.10.0) for CP
// License: MIT (Upstream: ibireme / yyjson)

import std from "std";

pub @c {
    fn cp_yyjson_read(dat: [*]const u8!, len: usize, flg: u32): *!?;
    fn cp_yyjson_doc_free(doc: *!?);
    fn cp_yyjson_doc_get_root(doc: *!?): *!?;
    fn cp_yyjson_doc_get_read_size(doc: *!?): usize;
    fn cp_yyjson_doc_get_val_count(doc: *!?): usize;
    fn cp_yyjson_write(doc: *!?, flg: u32, len: *usize?!): [*]u8?!;

    fn cp_yyjson_is_null(val: *!?): bool;
    fn cp_yyjson_is_bool(val: *!?): bool;
    fn cp_yyjson_is_int(val: *!?): bool;
    fn cp_yyjson_is_real(val: *!?): bool;
    fn cp_yyjson_is_str(val: *!?): bool;
    fn cp_yyjson_is_arr(val: *!?): bool;
    fn cp_yyjson_is_obj(val: *!?): bool;

    fn cp_yyjson_get_bool(val: *!?): bool;
    fn cp_yyjson_get_sint(val: *!?): i64;
    fn cp_yyjson_get_real(val: *!?): f64;
    fn cp_yyjson_get_str(val: *!?): [*:0]const u8?;
    fn cp_yyjson_get_len(val: *!?): usize;

    fn cp_yyjson_obj_size(obj: *!?): usize;
    fn cp_yyjson_obj_getn(obj: *!?, key: [*]const u8!, key_len: usize): *!?;
    fn cp_yyjson_arr_size(arr: *!?): usize;
    fn cp_yyjson_arr_get(arr: *!?, idx: usize): *!?;
    fn cp_yyjson_free_str(ptr: *!?);
}

pub const READ_NOFLAG: u32 = 0;
pub const READ_ALLOW_TRAILING_COMMAS: u32 = 1 << 1;
pub const READ_ALLOW_COMMENTS: u32 = 1 << 2;
pub const WRITE_NOFLAG: u32 = 0;
pub const WRITE_PRETTY: u32 = 1 << 0;

pub err YyjsonError {
    ParseFailed,
    WriteFailed,
    OutOfMemory,
}

/// Non-owning zero-copy view of a JSON value inside a `JsonDoc`.
pub struct JsonVal {
    raw: *!?,

    pub fn isNull(): bool {
        if (_.raw == null) return true;
        unsafe { return cp_yyjson_is_null(_.raw); }
    }

    pub fn isBool(): bool {
        if (_.raw == null) return false;
        unsafe { return cp_yyjson_is_bool(_.raw); }
    }

    pub fn isInt(): bool {
        if (_.raw == null) return false;
        unsafe { return cp_yyjson_is_int(_.raw); }
    }

    pub fn isReal(): bool {
        if (_.raw == null) return false;
        unsafe { return cp_yyjson_is_real(_.raw); }
    }

    pub fn isStr(): bool {
        if (_.raw == null) return false;
        unsafe { return cp_yyjson_is_str(_.raw); }
    }

    pub fn isArr(): bool {
        if (_.raw == null) return false;
        unsafe { return cp_yyjson_is_arr(_.raw); }
    }

    pub fn isObj(): bool {
        if (_.raw == null) return false;
        unsafe { return cp_yyjson_is_obj(_.raw); }
    }

    pub fn asBool(): bool? {
        if (!_.isBool()) return null;
        unsafe { return cp_yyjson_get_bool(_.raw); }
    }

    pub fn asInt(): i64? {
        if (!_.isInt()) return null;
        unsafe { return cp_yyjson_get_sint(_.raw); }
    }

    pub fn asReal(): f64? {
        if (!_.isReal()) return null;
        unsafe { return cp_yyjson_get_real(_.raw); }
    }

    pub fn asStr(): string? {
        if (!_.isStr()) return null;
        unsafe {
            if (p := cp_yyjson_get_str(_.raw)) {
                const n = cp_yyjson_get_len(_.raw);
                return p[0..n];
            }
        }
        return null;
    }

    pub fn objLen(): usize {
        if (!_.isObj()) return 0;
        unsafe { return cp_yyjson_obj_size(_.raw); }
    }

    pub fn get(key: []const u8): JsonVal? {
        if (!_.isObj()) return null;
        unsafe {
            const found = cp_yyjson_obj_getn(_.raw, key.ptr, key.len);
            if (found == null) return null;
            return JsonVal{ .raw = found };
        }
    }

    pub fn arrLen(): usize {
        if (!_.isArr()) return 0;
        unsafe { return cp_yyjson_arr_size(_.raw); }
    }

    pub fn arrGet(idx: usize): JsonVal? {
        if (!_.isArr()) return null;
        unsafe {
            const found = cp_yyjson_arr_get(_.raw, idx);
            if (found == null) return null;
            return JsonVal{ .raw = found };
        }
    }
}

/// Owned RAII JSON document backed by yyjson. Automatically frees memory on scope exit.
pub struct JsonDoc {
    doc_ptr: *!? = null,

    pub static fn parse(input: []const u8): JsonDoc! {
        return JsonDoc.parseWithFlags(input, READ_NOFLAG);
    }

    pub static fn parseWithFlags(input: []const u8, flags: u32): JsonDoc! {
        let p: *!? = null;
        unsafe {
            p = cp_yyjson_read(input.ptr, input.len, flags);
        }
        if (p == null) return YyjsonError.ParseFailed;
        return JsonDoc{ .doc_ptr = p };
    }

    pub fn root(): JsonVal {
        let r: *!? = null;
        if (_.doc_ptr != null) {
            unsafe { r = cp_yyjson_doc_get_root(_.doc_ptr); }
        }
        return JsonVal{ .raw = r };
    }

    pub fn valCount(): usize {
        if (_.doc_ptr == null) return 0;
        unsafe { return cp_yyjson_doc_get_val_count(_.doc_ptr); }
    }

    pub fn readSize(): usize {
        if (_.doc_ptr == null) return 0;
        unsafe { return cp_yyjson_doc_get_read_size(_.doc_ptr); }
    }

    pub fn stringify(allocator: std.mem.Allocator, pretty: bool): []u8! {
        if (_.doc_ptr == null) return YyjsonError.WriteFailed;
        const flg: u32 = if (pretty) WRITE_PRETTY else WRITE_NOFLAG;
        let out_len: usize = 0;
        let c_buf: [*]u8?! = null;
        unsafe {
            c_buf = cp_yyjson_write(_.doc_ptr, flg, &out_len);
        }
        if (ptr := c_buf) {
            defer { unsafe { cp_yyjson_free_str(@ptrCast(ptr)); } }
            const owned = allocator.alloc(u8, out_len) catch return YyjsonError.OutOfMemory;
            @memcpy(owned, ptr[0..out_len]);
            return owned;
        }
        return YyjsonError.WriteFailed;
    }

    ~init() {
        if (d := _.doc_ptr) {
            unsafe { cp_yyjson_doc_free(d); }
            _.doc_ptr = null;
        }
    }
}
