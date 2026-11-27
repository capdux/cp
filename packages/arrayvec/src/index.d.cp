// Type definitions for CP

export class ArrayString64 {
    buf: [STR64_CAP]u8,
    count: usize,
    fn init(): ArrayString64;
    fn fromStr(s: string): ArrayString64?;
    fn len(): usize;
    fn capacity(): usize;
    fn remaining(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn tryPushByte(_: auto, b: u8): bool;
    fn tryPushStr(_: auto, s: string): bool;
    fn pushStrTruncating(_: auto, s: string): usize;
    fn popByte(_: auto): u8?;
    fn truncate(_: auto, new_len: usize);
    fn clear(_: auto);
    fn asStr(): string;
}

export class ArrayString256 {
    buf: [STR256_CAP]u8,
    count: usize,
    fn init(): ArrayString256;
    fn fromStr(s: string): ArrayString256?;
    fn len(): usize;
    fn capacity(): usize;
    fn remaining(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn tryPushByte(_: auto, b: u8): bool;
    fn tryPushStr(_: auto, s: string): bool;
    fn pushStrTruncating(_: auto, s: string): usize;
    fn popByte(_: auto): u8?;
    fn truncate(_: auto, new_len: usize);
    fn clear(_: auto);
    fn asStr(): string;
}

export class ArrayVecI64 {
    items: [VEC64_CAP]i64,
    count: usize,
    fn init(): ArrayVecI64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn tryPush(_: auto, v: i64): bool;
    fn pop(_: auto): i64?;
    fn get(idx: usize): i64?;
    fn set(_: auto, idx: usize, v: i64): bool;
    fn tryInsert(_: auto, idx: usize, v: i64): bool;
    fn remove(_: auto, idx: usize): i64?;
    fn swapRemove(_: auto, idx: usize): i64?;
    fn truncate(_: auto, new_len: usize);
    fn clear(_: auto);
}

export class ArrayVecU64 {
    items: [VEC64_CAP]u64,
    count: usize,
    fn init(): ArrayVecU64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn tryPush(_: auto, v: u64): bool;
    fn pop(_: auto): u64?;
    fn get(idx: usize): u64?;
    fn set(_: auto, idx: usize, v: u64): bool;
    fn tryInsert(_: auto, idx: usize, v: u64): bool;
    fn remove(_: auto, idx: usize): u64?;
    fn swapRemove(_: auto, idx: usize): u64?;
    fn truncate(_: auto, new_len: usize);
    fn clear(_: auto);
}

