// Type definitions for CP

export class ListenerSlot {
    id: u32,
    fired: bool,
    reason: u8,
    err_code: i32,
}

export class StreamLifecycle {
    readable_ended: bool,
    writable_finished: bool,
    socket_closed: bool,
    errored: bool,
    err_code: i32,
    finish_reason: u8,
    listeners: [16]ListenerSlot,
    listener_count: usize,
    fired_count: usize,
    fn new(): StreamLifecycle;
    fn isFinished(): bool;
    fn isReadableFinished(): bool;
    fn isWritableFinished(): bool;
    fn onFinished(_: auto, listener_id: u32): bool;
    fn dispatchAll(_: auto, reason: u8, err_code: i32): usize;
    fn markEnd(_: auto): usize;
    fn markFinish(_: auto): usize;
    fn markClose(_: auto): usize;
    fn markError(_: auto, code: i32): usize;
    fn getListener(listener_id: u32): ListenerSlot?;
}

export fn isFinished(lc: StreamLifecycle): bool;
