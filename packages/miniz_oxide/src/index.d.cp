// Type definitions for CP

import {  } from "std";
export fn cp_miniz_version(): [*:0]&u8!;
export fn cp_miniz_zip_create_single(entry_name: [*:0]&u8!, data: [*]&u8!, data_len: usize, out_zip_len: *usize!): [*]u8!?;
export fn cp_miniz_zip_extract_entry(zip_bytes: [*]&u8!, zip_len: usize, entry_name: [*:0]&u8!, out_len: *usize!): [*]u8!?;
export fn cp_miniz_free(ptr: *auto!?);
export error MinizError { ZipCreateFailed, ZipExtractFailed, OutOfMemory }

export fn versionString(): string;
export class MinizBuf {
    raw: [*]u8!?,
    len: usize,
    fn zipSingleFile(entry_name: string, payload: &[]u8): !MinizBuf;
    fn extractFile(zip_bytes: &[]u8, entry_name: string): !MinizBuf;
    fn slice(): &[]u8;
    fn __cp_deinit(_: &mut MinizBuf);
}

