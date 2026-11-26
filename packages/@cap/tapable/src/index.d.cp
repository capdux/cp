// Type definitions for CP

export class TapEntry {
    name: string,
    stage: i32,
    handler_id: u32,
    enabled: bool,
}

export fn tapEmptyEntry(): TapEntry;
export fn tapStrEq(a: string, b: string): bool;
export fn tapInsertSorted(entries: []TapEntry, count: usize, entry: TapEntry): usize;
export fn tapInsertBefore(entries: []TapEntry, count: usize, name: string, before_name: string, handler_id: u32): usize;
export fn tapSetEnabled(entries: []TapEntry, count: usize, name: string, enabled: bool): bool;
export fn tapCollectOrdered(entries: &[]TapEntry, count: usize, out: []u32): usize;
export class SyncHook {
    entries: [32]TapEntry,
    count: usize,
    fn init(): SyncHook;
    fn tap(_: auto, name: string, handler_id: u32): bool;
    fn tapStage(_: auto, name: string, stage: i32, handler_id: u32): bool;
    fn tapBefore(_: auto, name: string, before_name: string, handler_id: u32): bool;
    fn setEnabled(_: auto, name: string, enabled: bool): bool;
    fn isUsed(): bool;
    fn tapCount(): usize;
    fn getOrderedHandlers(out: []u32): usize;
    fn call(cb: TapSyncFn, arg: i64): usize;
}

export class SyncBailHook {
    entries: [32]TapEntry,
    count: usize,
    fn init(): SyncBailHook;
    fn tap(_: auto, name: string, handler_id: u32): bool;
    fn tapStage(_: auto, name: string, stage: i32, handler_id: u32): bool;
    fn tapBefore(_: auto, name: string, before_name: string, handler_id: u32): bool;
    fn setEnabled(_: auto, name: string, enabled: bool): bool;
    fn isUsed(): bool;
    fn tapCount(): usize;
    fn getOrderedHandlers(out: []u32): usize;
    fn call(cb: TapBailFn, arg: i64): i64?;
}

export class SyncWaterfallHook {
    entries: [32]TapEntry,
    count: usize,
    fn init(): SyncWaterfallHook;
    fn tap(_: auto, name: string, handler_id: u32): bool;
    fn tapStage(_: auto, name: string, stage: i32, handler_id: u32): bool;
    fn tapBefore(_: auto, name: string, before_name: string, handler_id: u32): bool;
    fn setEnabled(_: auto, name: string, enabled: bool): bool;
    fn isUsed(): bool;
    fn tapCount(): usize;
    fn getOrderedHandlers(out: []u32): usize;
    fn call(cb: TapWaterfallFn, initial: i64): i64;
}

export class SyncLoopHook {
    entries: [32]TapEntry,
    count: usize,
    fn init(): SyncLoopHook;
    fn tap(_: auto, name: string, handler_id: u32): bool;
    fn tapStage(_: auto, name: string, stage: i32, handler_id: u32): bool;
    fn tapBefore(_: auto, name: string, before_name: string, handler_id: u32): bool;
    fn setEnabled(_: auto, name: string, enabled: bool): bool;
    fn isUsed(): bool;
    fn tapCount(): usize;
    fn getOrderedHandlers(out: []u32): usize;
    fn call(cb: TapLoopFn, arg: i64, max_rounds: usize): bool;
}

