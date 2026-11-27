// @cap/yallist — Zero-allocation O(1) index-pool doubly linked list in pure CP
// Clean-room ISC implementation inspired by npm `yallist` v5.

pub const YALLIST_CAP: usize = 64;

const YAL_NONE: i32 = -1;

fn yalValidHandle(handle: i32): bool {
    return handle >= 0 && (handle as usize) < YALLIST_CAP;
}

/// Zero-heap O(1) doubly linked list of `u64` values backed by a 64-slot freelist pool.
pub struct YallistU64 {
    values: [64]u64,
    prev: [64]i32,
    next: [64]i32,
    alive: [64]bool,
    free_stack: [64]i32,
    free_top: usize,
    head: i32,
    tail: i32,
    length: usize,

    /// Create an empty `YallistU64` with 64 pre-populated freelist slots.
    static fn new(): YallistU64 {
        let list: YallistU64 = YallistU64 {
            values: undefined,
            prev: undefined,
            next: undefined,
            alive: undefined,
            free_stack: undefined,
            free_top: YALLIST_CAP,
            head: YAL_NONE,
            tail: YAL_NONE,
            length: 0,
        };
        let i: usize = 0;
        while (i < YALLIST_CAP) : (i += 1) {
            list.values[i] = 0;
            list.prev[i] = YAL_NONE;
            list.next[i] = YAL_NONE;
            list.alive[i] = false;
            list.free_stack[i] = (YALLIST_CAP - 1 - i) as i32;
        }
        return list;
    }

    /// Number of elements currently in the list.
    fn len(): usize {
        return _.length;
    }

    /// Maximum fixed capacity (64).
    fn capacity(): usize {
        return YALLIST_CAP;
    }

    /// Returns `true` if the list contains no elements.
    fn isEmpty(): bool {
        return _.length == 0;
    }

    /// Append `val` to the tail in O(1). Returns the node handle (`0..63`), or `-1` if full.
    fn push(*_, val: u64): i32 {
        if (_.free_top == 0) {
            return YAL_NONE;
        }
        _.free_top -= 1;
        const h: i32 = _.free_stack[_.free_top];
        const idx: usize = h as usize;
        _.values[idx] = val;
        _.alive[idx] = true;
        _.prev[idx] = _.tail;
        _.next[idx] = YAL_NONE;

        if (_.tail != YAL_NONE) {
            _.next[_.tail as usize] = h;
        } else {
            _.head = h;
        }
        _.tail = h;
        _.length += 1;
        return h;
    }

    /// Remove and return the tail value in O(1), or `null` if empty.
    fn pop(*_): u64? {
        if (_.tail == YAL_NONE) {
            return null;
        }
        const h: i32 = _.tail;
        const idx: usize = h as usize;
        const val: u64 = _.values[idx];
        const p: i32 = _.prev[idx];
        _.tail = p;
        if (p != YAL_NONE) {
            _.next[p as usize] = YAL_NONE;
        } else {
            _.head = YAL_NONE;
        }
        _.alive[idx] = false;
        _.prev[idx] = YAL_NONE;
        _.next[idx] = YAL_NONE;
        _.free_stack[_.free_top] = h;
        _.free_top += 1;
        _.length -= 1;
        return val;
    }

    /// Prepend `val` to the head in O(1). Returns the node handle (`0..63`), or `-1` if full.
    fn unshift(*_, val: u64): i32 {
        if (_.free_top == 0) {
            return YAL_NONE;
        }
        _.free_top -= 1;
        const h: i32 = _.free_stack[_.free_top];
        const idx: usize = h as usize;
        _.values[idx] = val;
        _.alive[idx] = true;
        _.prev[idx] = YAL_NONE;
        _.next[idx] = _.head;

        if (_.head != YAL_NONE) {
            _.prev[_.head as usize] = h;
        } else {
            _.tail = h;
        }
        _.head = h;
        _.length += 1;
        return h;
    }

    /// Remove and return the head value in O(1), or `null` if empty.
    fn shift(*_): u64? {
        if (_.head == YAL_NONE) {
            return null;
        }
        const h: i32 = _.head;
        const idx: usize = h as usize;
        const val: u64 = _.values[idx];
        const n: i32 = _.next[idx];
        _.head = n;
        if (n != YAL_NONE) {
            _.prev[n as usize] = YAL_NONE;
        } else {
            _.tail = YAL_NONE;
        }
        _.alive[idx] = false;
        _.prev[idx] = YAL_NONE;
        _.next[idx] = YAL_NONE;
        _.free_stack[_.free_top] = h;
        _.free_top += 1;
        _.length -= 1;
        return val;
    }

    /// Return the element at 0-based index `idx` (walking from nearest end), or `null` if out of bounds.
    fn get(idx: usize): u64? {
        if (idx >= _.length) {
            return null;
        }
        if (idx <= _.length / 2) {
            let cur: i32 = _.head;
            let i: usize = 0;
            while (i < idx && cur != YAL_NONE) : (i += 1) {
                cur = _.next[cur as usize];
            }
            if (cur == YAL_NONE) {
                return null;
            }
            return _.values[cur as usize];
        }
        let cur: i32 = _.tail;
        let steps: usize = _.length - 1 - idx;
        while (steps > 0 && cur != YAL_NONE) : (steps -= 1) {
            cur = _.prev[cur as usize];
        }
        if (cur == YAL_NONE) {
            return null;
        }
        return _.values[cur as usize];
    }

    /// Peek the head value without removing it.
    fn headVal(): u64? {
        if (_.head == YAL_NONE) {
            return null;
        }
        return _.values[_.head as usize];
    }

    /// Peek the tail value without removing it.
    fn tailVal(): u64? {
        if (_.tail == YAL_NONE) {
            return null;
        }
        return _.values[_.tail as usize];
    }

    /// Remove an active node by its handle in O(1) and return its slot to the freelist.
    fn removeNode(*_, handle: i32): bool {
        if (!yalValidHandle(handle)) {
            return false;
        }
        const idx: usize = handle as usize;
        if (!_.alive[idx]) {
            return false;
        }
        const p: i32 = _.prev[idx];
        const n: i32 = _.next[idx];
        if (p != YAL_NONE) {
            _.next[p as usize] = n;
        } else {
            _.head = n;
        }
        if (n != YAL_NONE) {
            _.prev[n as usize] = p;
        } else {
            _.tail = p;
        }
        _.alive[idx] = false;
        _.prev[idx] = YAL_NONE;
        _.next[idx] = YAL_NONE;
        _.free_stack[_.free_top] = handle;
        _.free_top += 1;
        _.length -= 1;
        return true;
    }

    /// Move an existing node to the head in O(1) without reallocating its handle.
    fn unshiftNode(*_, handle: i32): bool {
        if (!yalValidHandle(handle)) {
            return false;
        }
        const idx: usize = handle as usize;
        if (!_.alive[idx]) {
            return false;
        }
        if (_.head == handle) {
            return true;
        }
        const p: i32 = _.prev[idx];
        const n: i32 = _.next[idx];
        if (p != YAL_NONE) {
            _.next[p as usize] = n;
        }
        if (n != YAL_NONE) {
            _.prev[n as usize] = p;
        } else {
            _.tail = p;
        }
        _.prev[idx] = YAL_NONE;
        _.next[idx] = _.head;
        if (_.head != YAL_NONE) {
            _.prev[_.head as usize] = handle;
        }
        _.head = handle;
        return true;
    }

    /// Move an existing node to the tail in O(1) without reallocating its handle.
    fn pushNode(*_, handle: i32): bool {
        if (!yalValidHandle(handle)) {
            return false;
        }
        const idx: usize = handle as usize;
        if (!_.alive[idx]) {
            return false;
        }
        if (_.tail == handle) {
            return true;
        }
        const p: i32 = _.prev[idx];
        const n: i32 = _.next[idx];
        if (p != YAL_NONE) {
            _.next[p as usize] = n;
        } else {
            _.head = n;
        }
        if (n != YAL_NONE) {
            _.prev[n as usize] = p;
        }
        _.next[idx] = YAL_NONE;
        _.prev[idx] = _.tail;
        if (_.tail != YAL_NONE) {
            _.next[_.tail as usize] = handle;
        }
        _.tail = handle;
        return true;
    }

    /// Reverse the list in-place in O(N).
    fn reverse(*_) {
        let cur: i32 = _.head;
        while (cur != YAL_NONE) {
            const idx: usize = cur as usize;
            const old_next: i32 = _.next[idx];
            _.next[idx] = _.prev[idx];
            _.prev[idx] = old_next;
            cur = old_next;
        }
        const old_head: i32 = _.head;
        _.head = _.tail;
        _.tail = old_head;
    }

    /// Copy elements from head to tail into `out`. Returns the number of elements written.
    fn toArray(out: []u64): usize {
        let count: usize = 0;
        let cur: i32 = _.head;
        while (cur != YAL_NONE && count < out.len) {
            out[count] = _.values[cur as usize];
            count += 1;
            cur = _.next[cur as usize];
        }
        return count;
    }
}

export {
    YALLIST_CAP,
    YallistU64,
};
