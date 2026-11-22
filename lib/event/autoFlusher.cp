// Source: Absorbed from /data/dev/bun/src/event_loop/AutoFlusher.cp | Synced: 2026-11-22T12:49:17Z
// lib/event/auto_flusher.cp
// Deferred flush coalescer for buffered I/O writers in cap:event

import std from "std";
import { DeferredTaskQueue } from "./deferredTask.cp";

pub struct AutoFlusher {
    pub registered: bool = false,

    pub static fn init(): AutoFlusher {
        return AutoFlusher{ registered: false };
    }

    pub fn isRegistered(): bool {
        return _.registered;
    }

    pub fn markRegistered(*_) {
        _.registered = true;
    }

    pub fn markUnregistered(*_) {
        _.registered = false;
    }

    pub fn registerDeferred(*_, queue: *DeferredTaskQueue, ctx: *anyopaque, task: fn (*anyopaque) bool): bool {
        if (_.registered) return false;
        _.registered = true;
        return queue.postTask(ctx, task);
    }

    pub fn unregisterDeferred(*_, queue: *DeferredTaskQueue, ctx: *anyopaque): bool {
        if (!_.registered) return false;
        _.registered = false;
        return queue.unregisterTask(ctx);
    }
}
