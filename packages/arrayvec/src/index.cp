// @cap/arrayvec — Zero-allocation stack-backed fixed-capacity containers for CP
// Clean-room MIT implementation inspired by Rust `arrayvec` (crates.io #214, 730M+ downloads).
//
// Containers:
// - `ArrayString64`  (64-byte inline stack string)
// - `ArrayString256` (256-byte inline stack string)
// - `ArrayVecI64`    (64-element inline stack vector of i64)
// - `ArrayVecU64`    (64-element inline stack vector of u64)

const STR64_CAP: usize = 64;
const STR256_CAP: usize = 256;
const VEC64_CAP: usize = 64;

/// Fixed-capacity 64-byte inline stack string (zero heap allocation).
pub struct ArrayString64 {
    pub buf: [STR64_CAP]u8,
    pub count: usize,

    pub static fn init() -> ArrayString64 {
        let b: [STR64_CAP]u8 = undefined;
        return ArrayString64{ buf: b, count: 0 };
    }

    pub static fn fromStr(s: string) -> ArrayString64? {
        if (s.len > STR64_CAP) {
            return null;
        }
        let res = ArrayString64.init();
        let i: usize = 0;
        while (i < s.len) {
            res.buf[i] = s[i];
            i += 1;
        }
        res.count = s.len;
        return res;
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn capacity() -> usize {
        return STR64_CAP;
    }

    pub fn remaining() -> usize {
        return STR64_CAP - _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn isFull() -> bool {
        return _.count >= STR64_CAP;
    }

    pub fn tryPushByte(*_, b: u8) -> bool {
        if (_.count >= STR64_CAP) {
            return false;
        }
        _.buf[_.count] = b;
        _.count += 1;
        return true;
    }

    pub fn tryPushStr(*_, s: string) -> bool {
        if (_.count + s.len > STR64_CAP) {
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.count + i] = s[i];
            i += 1;
        }
        _.count += s.len;
        return true;
    }

    /// Appends as many bytes from `s` as fit in remaining capacity, returning the number of bytes written.
    pub fn pushStrTruncating(*_, s: string) -> usize {
        const rem = STR64_CAP - _.count;
        const n = if (s.len < rem) { s.len } else { rem };
        let i: usize = 0;
        while (i < n) {
            _.buf[_.count + i] = s[i];
            i += 1;
        }
        _.count += n;
        return n;
    }

    pub fn popByte(*_) -> u8? {
        if (_.count == 0) {
            return null;
        }
        _.count -= 1;
        return _.buf[_.count];
    }

    pub fn truncate(*_, new_len: usize) {
        if (new_len < _.count) {
            _.count = new_len;
        }
    }

    pub fn clear(*_) {
        _.count = 0;
    }

    pub fn asStr() -> string {
        return _.buf[0.._.count];
    }
}

/// Fixed-capacity 256-byte inline stack string (zero heap allocation).
pub struct ArrayString256 {
    pub buf: [STR256_CAP]u8,
    pub count: usize,

    pub static fn init() -> ArrayString256 {
        let b: [STR256_CAP]u8 = undefined;
        return ArrayString256{ buf: b, count: 0 };
    }

    pub static fn fromStr(s: string) -> ArrayString256? {
        if (s.len > STR256_CAP) {
            return null;
        }
        let res = ArrayString256.init();
        let i: usize = 0;
        while (i < s.len) {
            res.buf[i] = s[i];
            i += 1;
        }
        res.count = s.len;
        return res;
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn capacity() -> usize {
        return STR256_CAP;
    }

    pub fn remaining() -> usize {
        return STR256_CAP - _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn isFull() -> bool {
        return _.count >= STR256_CAP;
    }

    pub fn tryPushByte(*_, b: u8) -> bool {
        if (_.count >= STR256_CAP) {
            return false;
        }
        _.buf[_.count] = b;
        _.count += 1;
        return true;
    }

    pub fn tryPushStr(*_, s: string) -> bool {
        if (_.count + s.len > STR256_CAP) {
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.count + i] = s[i];
            i += 1;
        }
        _.count += s.len;
        return true;
    }

    pub fn pushStrTruncating(*_, s: string) -> usize {
        const rem = STR256_CAP - _.count;
        const n = if (s.len < rem) { s.len } else { rem };
        let i: usize = 0;
        while (i < n) {
            _.buf[_.count + i] = s[i];
            i += 1;
        }
        _.count += n;
        return n;
    }

    pub fn popByte(*_) -> u8? {
        if (_.count == 0) {
            return null;
        }
        _.count -= 1;
        return _.buf[_.count];
    }

    pub fn truncate(*_, new_len: usize) {
        if (new_len < _.count) {
            _.count = new_len;
        }
    }

    pub fn clear(*_) {
        _.count = 0;
    }

    pub fn asStr() -> string {
        return _.buf[0.._.count];
    }
}

/// Fixed-capacity 64-element stack vector of `i64` (zero heap allocation).
pub struct ArrayVecI64 {
    pub items: [VEC64_CAP]i64,
    pub count: usize,

    pub static fn init() -> ArrayVecI64 {
        let arr: [VEC64_CAP]i64 = undefined;
        return ArrayVecI64{ items: arr, count: 0 };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn isFull() -> bool {
        return _.count >= VEC64_CAP;
    }

    pub fn tryPush(*_, v: i64) -> bool {
        if (_.count >= VEC64_CAP) {
            return false;
        }
        _.items[_.count] = v;
        _.count += 1;
        return true;
    }

    pub fn pop(*_) -> i64? {
        if (_.count == 0) {
            return null;
        }
        _.count -= 1;
        return _.items[_.count];
    }

    pub fn get(idx: usize) -> i64? {
        if (idx >= _.count) {
            return null;
        }
        return _.items[idx];
    }

    pub fn set(*_, idx: usize, v: i64) -> bool {
        if (idx >= _.count) {
            return false;
        }
        _.items[idx] = v;
        return true;
    }

    pub fn tryInsert(*_, idx: usize, v: i64) -> bool {
        if (_.count >= VEC64_CAP || idx > _.count) {
            return false;
        }
        let j = _.count;
        while (j > idx) {
            _.items[j] = _.items[j - 1];
            j -= 1;
        }
        _.items[idx] = v;
        _.count += 1;
        return true;
    }

    /// Removes and returns the element at `idx`, shifting subsequent elements left to preserve order.
    pub fn remove(*_, idx: usize) -> i64? {
        if (idx >= _.count) {
            return null;
        }
        const removed = _.items[idx];
        let j = idx;
        while (j + 1 < _.count) {
            _.items[j] = _.items[j + 1];
            j += 1;
        }
        _.count -= 1;
        return removed;
    }

    /// O(1) swap-remove: replaces `idx` with the last element and returns the removed value.
    pub fn swapRemove(*_, idx: usize) -> i64? {
        if (idx >= _.count) {
            return null;
        }
        const removed = _.items[idx];
        _.count -= 1;
        if (idx < _.count) {
            _.items[idx] = _.items[_.count];
        }
        return removed;
    }

    pub fn truncate(*_, new_len: usize) {
        if (new_len < _.count) {
            _.count = new_len;
        }
    }

    pub fn clear(*_) {
        _.count = 0;
    }
}

/// Fixed-capacity 64-element stack vector of `u64` (zero heap allocation).
pub struct ArrayVecU64 {
    pub items: [VEC64_CAP]u64,
    pub count: usize,

    pub static fn init() -> ArrayVecU64 {
        let arr: [VEC64_CAP]u64 = undefined;
        return ArrayVecU64{ items: arr, count: 0 };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn isFull() -> bool {
        return _.count >= VEC64_CAP;
    }

    pub fn tryPush(*_, v: u64) -> bool {
        if (_.count >= VEC64_CAP) {
            return false;
        }
        _.items[_.count] = v;
        _.count += 1;
        return true;
    }

    pub fn pop(*_) -> u64? {
        if (_.count == 0) {
            return null;
        }
        _.count -= 1;
        return _.items[_.count];
    }

    pub fn get(idx: usize) -> u64? {
        if (idx >= _.count) {
            return null;
        }
        return _.items[idx];
    }

    pub fn set(*_, idx: usize, v: u64) -> bool {
        if (idx >= _.count) {
            return false;
        }
        _.items[idx] = v;
        return true;
    }

    pub fn tryInsert(*_, idx: usize, v: u64) -> bool {
        if (_.count >= VEC64_CAP || idx > _.count) {
            return false;
        }
        let j = _.count;
        while (j > idx) {
            _.items[j] = _.items[j - 1];
            j -= 1;
        }
        _.items[idx] = v;
        _.count += 1;
        return true;
    }

    pub fn remove(*_, idx: usize) -> u64? {
        if (idx >= _.count) {
            return null;
        }
        const removed = _.items[idx];
        let j = idx;
        while (j + 1 < _.count) {
            _.items[j] = _.items[j + 1];
            j += 1;
        }
        _.count -= 1;
        return removed;
    }

    pub fn swapRemove(*_, idx: usize) -> u64? {
        if (idx >= _.count) {
            return null;
        }
        const removed = _.items[idx];
        _.count -= 1;
        if (idx < _.count) {
            _.items[idx] = _.items[_.count];
        }
        return removed;
    }

    pub fn truncate(*_, new_len: usize) {
        if (new_len < _.count) {
            _.count = new_len;
        }
    }

    pub fn clear(*_) {
        _.count = 0;
    }
}
