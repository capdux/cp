// Type definitions for CP

import { StreamLifecycle, isFinished } from "../../on-finished/src/index.cp";
export class DestroyableStream {
    kind: u8,
    has_fd: bool,
    fd: i32,
    fd_open: bool,
    destroyed: bool,
    destroy_calls: usize,
    close_calls: usize,
    lifecycle: StreamLifecycle,
    fn generic(): DestroyableStream;
    fn readFile(fd: i32, fd_open: bool): DestroyableStream;
    fn zlib(): DestroyableStream;
    fn isDestroyed(): bool;
    fn onOpen(_: auto, fd: i32): u8;
}

export fn destroyStream(s: &mut DestroyableStream): u8;
export fn destroyWithError(s: &mut DestroyableStream, err_code: i32): u8;
