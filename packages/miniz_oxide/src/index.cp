// @cap/miniz — Single-file ZIP archive & DEFLATE compression (miniz 3.0.2) for CP
// License: MIT (Upstream: Rich Geldreich / miniz)

import std from "std";

pub @c {
    fn cp_miniz_version(): [*:0]const u8;
    fn cp_miniz_zip_create_single(
        entry_name: [*:0]const u8!,
        data: [*]const u8!,
        data_len: usize,
        out_zip_len: *usize!
    ): [*]u8?!;
    fn cp_miniz_zip_extract_entry(
        zip_bytes: [*]const u8!,
        zip_len: usize,
        entry_name: [*:0]const u8!,
        out_len: *usize!
    ): [*]u8?!;
    fn cp_miniz_free(ptr: *!?);
}

pub err MinizError {
    ZipCreateFailed,
    ZipExtractFailed,
    OutOfMemory,
}

pub fn versionString(): string {
    unsafe { return std.mem.span(cp_miniz_version()); }
}

/// Owned RAII byte buffer allocated by miniz.
pub struct MinizBuf {
    raw: [*]u8?! = null,
    pub len: usize = 0,

    pub static fn zipSingleFile(entry_name: string, payload: []const u8): MinizBuf! {
        const alloc = std.heap.page_allocator;
        const name_z = alloc.allocSentinel(u8, entry_name.len, 0) catch return MinizError.OutOfMemory;
        defer alloc.free(name_z);
        @memcpy(name_z[0..entry_name.len], entry_name);

        let sz: usize = 0;
        let ptr: [*]u8?! = null;
        unsafe {
            ptr = cp_miniz_zip_create_single(name_z.ptr, payload.ptr, payload.len, &sz);
        }
        if (ptr == null or sz == 0) return MinizError.ZipCreateFailed;
        return MinizBuf{ .raw = ptr, .len = sz };
    }

    pub static fn extractFile(zip_bytes: []const u8, entry_name: string): MinizBuf! {
        const alloc = std.heap.page_allocator;
        const name_z = alloc.allocSentinel(u8, entry_name.len, 0) catch return MinizError.OutOfMemory;
        defer alloc.free(name_z);
        @memcpy(name_z[0..entry_name.len], entry_name);

        let sz: usize = 0;
        let ptr: [*]u8?! = null;
        unsafe {
            ptr = cp_miniz_zip_extract_entry(zip_bytes.ptr, zip_bytes.len, name_z.ptr, &sz);
        }
        if (ptr == null) return MinizError.ZipExtractFailed;
        return MinizBuf{ .raw = ptr, .len = sz };
    }

    pub fn slice(): []const u8 {
        if (p := _.raw) return p[0.._.len];
        return "";
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            _.len = 0;
            unsafe { cp_miniz_free(@ptrCast(p)); }
        }
    }
}
