// Type definitions for CP

export fn dlvlZeroI64(): [DLV_CAP]i64;
export fn dlvlNoneU32(): [DLV_CAP]u32;
export fn dlvlFalseBool(): [DLV_CAP]bool;
export fn dlvlAllocSlot(list: &mut DlvListI64): u32;
export class DlvListI64 {
    values: [DLV_CAP]i64,
    prev: [DLV_CAP]u32,
    next: [DLV_CAP]u32,
    occupied: [DLV_CAP]bool,
    head: u32,
    tail: u32,
    freeHead: u32,
    usedSlots: u32,
    count: usize,
    fn new(): DlvListI64;
    fn pushFront(_: auto, val: i64): u32;
    fn pushBack(_: auto, val: i64): u32;
    fn get(idx: u32): i64?;
    fn remove(_: auto, idx: u32): i64?;
    fn popFront(_: auto): i64?;
    fn popBack(_: auto): i64?;
    fn front(): i64?;
    fn back(): i64?;
    fn len(): usize;
    fn isEmpty(): bool;
}

