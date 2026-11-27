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
export class DestroyState {
    resource_id: u64,
    destroyed: bool,
    destroy_calls: usize,
    err_code: i32,
    fn new(resource_id: u64): DestroyState;
    fn isDestroyed(): bool;
    fn destroy(_: auto): bool;
    fn destroyWithError(_: auto, err_code: i32): bool;
    fn reset(_: auto);
}

export fn destroy(s: &mut DestroyableStream): u8;
export fn isDestroyed(s: DestroyableStream): bool;
export fn destroyAll(streams: []DestroyableStream): usize;
export fn destroyAllStates(states: []DestroyState): usize;
export fn isAnyDestroyed(streams: &[]DestroyableStream): bool;
export fn areAllDestroyed(streams: &[]DestroyableStream): bool;
