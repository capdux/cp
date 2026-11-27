// Type definitions for CP

import {  } from "std";
export fn cp_dav1d_version(): [*:0]&u8!;
export fn cp_dav1d_version_api(): u32;
export fn cp_dav1d_open(n_threads: i32, max_frame_delay: i32): *auto!?;
export fn cp_dav1d_flush(ctx: *auto!?);
export fn cp_dav1d_close(ctx: *auto!?);
export error Dav1dError { OpenFailed }

export fn version(): string;
export fn versionApi(): u32;
export class Dav1dDecoder {
    raw: *auto!?,
    fn open(n_threads: i32, max_frame_delay: i32): !Dav1dDecoder;
    fn isOpen(): bool;
    fn flush();
    fn __cp_deinit(_: &mut Dav1dDecoder);
}

