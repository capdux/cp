// Source: cp/lib/pqueue/pqueue.cp (Indexed Priority Queue, Bounded Top-K & Streaming Median)
// Pure CP data structures for O(log N) handle-indexed min-heap with in-place priority updates
// and arbitrary handle removal, O(K)-memory streaming BoundedTopK, and dual-heap StreamingMedian.

const std = @import("std");

const SLOT_BITS: u5 = 20;
const SLOT_MASK: u32 = (1 << 20) - 1;
const GEN_MASK: u32 = 0xFFF;
const NULL_SLOT: u32 = std.math.maxInt(u32);

/// Error set for `cap:pqueue` operations.
pub err PQueueError {
    InvalidHandle,
    InvalidCapacity,
    EmptyQueue,
    OutOfMemory,
}

/// Entry returned by `IndexedMinHeap<V>.peek()` and `IndexedMinHeap<V>.pop()`.
pub struct PriorityItem<V> {
    pub handle: u32,
    pub priority: i64,
    pub value: V,
}

/// Entry returned by `BoundedTopK<V>.topSorted()`.
pub struct TopKEntry<V> {
    pub score: f64,
    pub value: V,
}

struct HeapSlot<V> {
    priority: i64,
    value: V,
    heap_pos: usize,
    next_free: u32,
    gen: u32,
    active: bool,
}

/// Handle-indexed binary min-heap supporting O(log N) push, pop, in-place priority update,
/// and arbitrary handle removal.
pub struct IndexedMinHeap<V> {
    allocator: std.mem.Allocator,
    heap: std.ArrayListUnmanaged(u32),
    slots: std.ArrayListUnmanaged(HeapSlot(V)),
    free_head: u32,

    /// Initializes an empty `IndexedMinHeap<V>`.
    pub static fn init(allocator: std.mem.Allocator): IndexedMinHeap<V> {
        return IndexedMinHeap<V>{
            allocator: allocator,
            heap: std.ArrayListUnmanaged(u32).empty,
            slots: std.ArrayListUnmanaged(HeapSlot(V)).empty,
            free_head: NULL_SLOT,
        };
    }

    /// Releases all heap and slot memory.
    pub fn deinit(*_) {
        _.heap.deinit(_.allocator);
        _.slots.deinit(_.allocator);
        _.free_head = NULL_SLOT;
    }

    static fn makeHandle(slot_idx: u32, gen: u32): u32 {
        return ((gen & GEN_MASK) << SLOT_BITS) | (slot_idx & SLOT_MASK);
    }

    static fn nextGen(gen: u32): u32 {
        const next = (gen + 1) & GEN_MASK;
        return if (next == 0) 1 else next;
    }

    fn resolveSlot(handle: u32): u32? {
        const slot_idx = handle & SLOT_MASK;
        if (slot_idx >= _.slots.items.len) return null;
        const slot = _.slots.items[slot_idx];
        if (!slot.active) return null;
        const gen = (handle >> SLOT_BITS) & GEN_MASK;
        if (slot.gen != gen) return null;
        return slot_idx;
    }

    fn swapPos(*_, a: usize, b: usize) {
        const ha = _.heap.items[a];
        const hb = _.heap.items[b];
        _.heap.items[a] = hb;
        _.heap.items[b] = ha;
        _.slots.items[ha & SLOT_MASK].heap_pos = b;
        _.slots.items[hb & SLOT_MASK].heap_pos = a;
    }

    fn priorityAt(pos: usize): i64 {
        const h = _.heap.items[pos];
        return _.slots.items[h & SLOT_MASK].priority;
    }

    fn siftUp(*_, start_pos: usize): bool {
        let pos = start_pos;
        let moved = false;
        while (pos > 0) {
            const parent = (pos - 1) / 2;
            if (_.priorityAt(pos) < _.priorityAt(parent)) {
                _.swapPos(pos, parent);
                pos = parent;
                moved = true;
            } else {
                break;
            }
        }
        return moved;
    }

    fn siftDown(*_, start_pos: usize) {
        let pos = start_pos;
        const n = _.heap.items.len;
        while (true) {
            const left: usize = pos * 2 + 1;
            if (left >= n) break;
            let smallest: usize = left;
            const right: usize = left + 1;
            if (right < n && _.priorityAt(right) < _.priorityAt(left)) {
                smallest = right;
            }
            if (_.priorityAt(smallest) < _.priorityAt(pos)) {
                _.swapPos(pos, smallest);
                pos = smallest;
            } else {
                break;
            }
        }
    }

    fn releaseSlot(*_, slot_idx: u32) {
        _.slots.items[slot_idx].active = false;
        _.slots.items[slot_idx].gen = IndexedMinHeap<V>.nextGen(_.slots.items[slot_idx].gen);
        _.slots.items[slot_idx].next_free = _.free_head;
        _.free_head = slot_idx;
    }

    /// Pushes `(priority, value)` into the heap and returns a stable `u32` handle.
    pub fn push(*_, priority: i64, value: V): u32 ! PQueueError {
        let slot_idx: u32 = 0;
        let gen: u32 = 1;

        if (_.free_head != NULL_SLOT) {
            slot_idx = _.free_head;
            _.free_head = _.slots.items[slot_idx].next_free;
            gen = _.slots.items[slot_idx].gen;
            if (gen == 0) gen = 1;
            _.slots.items[slot_idx] = HeapSlot<V>{
                priority: priority,
                value: value,
                heap_pos: _.heap.items.len,
                next_free: NULL_SLOT,
                gen: gen,
                active: true,
            };
        } else {
            if (_.slots.items.len >= (SLOT_MASK as usize)) {
                return PQueueError.OutOfMemory;
            }
            slot_idx = _.slots.items.len as u32;
            _.slots.append(_.allocator, HeapSlot<V>{
                priority: priority,
                value: value,
                heap_pos: _.heap.items.len,
                next_free: NULL_SLOT,
                gen: gen,
                active: true,
            }) catch return PQueueError.OutOfMemory;
        }

        const handle = IndexedMinHeap<V>.makeHandle(slot_idx, gen);
        _.heap.append(_.allocator, handle) catch {
            _.releaseSlot(slot_idx);
            return PQueueError.OutOfMemory;
        };

        const pos = _.heap.items.len - 1;
        * = _.siftUp(pos);
        return handle;
    }

    /// Returns the minimum-priority item without removing it, or `null` if empty.
    pub fn peek(): PriorityItem<V>? {
        if (_.heap.items.len == 0) return null;
        const h = _.heap.items[0];
        const slot = _.slots.items[h & SLOT_MASK];
        return PriorityItem<V>{
            handle: h,
            priority: slot.priority,
            value: slot.value,
        };
    }

    /// Removes and returns the minimum-priority item, or `null` if empty.
    pub fn pop(*_): PriorityItem<V>? {
        if (_.heap.items.len == 0) return null;
        const top_h = _.heap.items[0];
        const top_slot_idx = top_h & SLOT_MASK;
        const top_slot = _.slots.items[top_slot_idx];
        const item = PriorityItem<V>{
            handle: top_h,
            priority: top_slot.priority,
            value: top_slot.value,
        };

        const last_idx = _.heap.items.len - 1;
        if (last_idx > 0) {
            const last_h = _.heap.items[last_idx];
            _.heap.items[0] = last_h;
            _.slots.items[last_h & SLOT_MASK].heap_pos = 0;
            _.heap.items.len = last_idx;
            _.siftDown(0);
        } else {
            _.heap.items.len = 0;
        }

        _.releaseSlot(top_slot_idx);
        return item;
    }

    /// Updates the priority of `handle` in O(log N) time, sifting up or down in place.
    pub fn updatePriority(*_, handle: u32, new_priority: i64): void ! PQueueError {
        const slot_idx = _.resolveSlot(handle) orelse return PQueueError.InvalidHandle;
        const old_priority = _.slots.items[slot_idx].priority;
        _.slots.items[slot_idx].priority = new_priority;
        const pos = _.slots.items[slot_idx].heap_pos;

        if (new_priority < old_priority) {
            * = _.siftUp(pos);
        } else if (new_priority > old_priority) {
            _.siftDown(pos);
        }
    }

    /// Removes the item associated with `handle` in O(log N) time and returns it.
    pub fn removeByHandle(*_, handle: u32): PriorityItem<V> ! PQueueError {
        const slot_idx = _.resolveSlot(handle) orelse return PQueueError.InvalidHandle;
        const slot = _.slots.items[slot_idx];
        const item = PriorityItem<V>{
            handle: handle,
            priority: slot.priority,
            value: slot.value,
        };

        const pos = slot.heap_pos;
        const last_idx = _.heap.items.len - 1;
        if (pos != last_idx) {
            const last_h = _.heap.items[last_idx];
            _.heap.items[pos] = last_h;
            _.slots.items[last_h & SLOT_MASK].heap_pos = pos;
            _.heap.items.len = last_idx;
            if (!_.siftUp(pos)) {
                _.siftDown(pos);
            }
        } else {
            _.heap.items.len = last_idx;
        }

        _.releaseSlot(slot_idx);
        return item;
    }

    /// Returns `true` if `handle` currently refers to an active item in the heap.
    pub fn containsHandle(handle: u32): bool {
        return _.resolveSlot(handle) != null;
    }

    /// Returns the number of active items in the heap.
    pub fn len(): usize {
        return _.heap.items.len;
    }

    /// Returns `true` if the heap is empty.
    pub fn isEmpty(): bool {
        return _.heap.items.len == 0;
    }

    /// Removes all active items and invalidates all outstanding handles.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.heap.items.len) : (i += 1) {
            const slot_idx = _.heap.items[i] & SLOT_MASK;
            _.releaseSlot(slot_idx);
        }
        _.heap.clearRetainingCapacity();
    }
}

/// Streaming fixed-capacity Top-K retainer keeping the K highest-scoring items in O(K) memory.
pub struct BoundedTopK<V> {
    allocator: std.mem.Allocator,
    k: usize,
    heap: std.ArrayListUnmanaged(TopKEntry(V)),

    /// Initializes a `BoundedTopK<V>` with capacity `k` (`k == 0` returns `InvalidCapacity`).
    pub static fn init(allocator: std.mem.Allocator, k: usize): BoundedTopK<V> ! PQueueError {
        if (k == 0) return PQueueError.InvalidCapacity;
        return BoundedTopK<V>{
            allocator: allocator,
            k: k,
            heap: std.ArrayListUnmanaged(TopKEntry(V)).empty,
        };
    }

    /// Releases internal heap storage.
    pub fn deinit(*_) {
        _.heap.deinit(_.allocator);
    }

    fn swapEntries(*_, a: usize, b: usize) {
        const tmp = _.heap.items[a];
        _.heap.items[a] = _.heap.items[b];
        _.heap.items[b] = tmp;
    }

    fn siftUpMin(*_, start_pos: usize) {
        let pos = start_pos;
        while (pos > 0) {
            const parent = (pos - 1) / 2;
            if (_.heap.items[pos].score < _.heap.items[parent].score) {
                _.swapEntries(pos, parent);
                pos = parent;
            } else {
                break;
            }
        }
    }

    fn siftDownMin(*_, start_pos: usize) {
        let pos = start_pos;
        const n = _.heap.items.len;
        while (true) {
            const left: usize = pos * 2 + 1;
            if (left >= n) break;
            let smallest: usize = left;
            const right: usize = left + 1;
            if (right < n && _.heap.items[right].score < _.heap.items[left].score) {
                smallest = right;
            }
            if (_.heap.items[smallest].score < _.heap.items[pos].score) {
                _.swapEntries(pos, smallest);
                pos = smallest;
            } else {
                break;
            }
        }
    }

    /// Pushes `(score, value)` into the streaming retainer, maintaining at most `k` highest-scoring entries.
    pub fn push(*_, score: f64, value: V): void ! PQueueError {
        if (std.math.isNan(score)) return;
        if (_.heap.items.len < _.k) {
            _.heap.append(_.allocator, TopKEntry<V>{
                score: score,
                value: value,
            }) catch return PQueueError.OutOfMemory;
            _.siftUpMin(_.heap.items.len - 1);
        } else if (score > _.heap.items[0].score) {
            _.heap.items[0] = TopKEntry<V>{
                score: score,
                value: value,
            };
            _.siftDownMin(0);
        }
    }

    /// Returns a newly allocated slice of the retained top-K entries sorted in descending order of `score`.
    pub fn topSorted(allocator: std.mem.Allocator): ([]TopKEntry<V>) ! PQueueError {
        const n = _.heap.items.len;
        let out = allocator.alloc(TopKEntry(V), n) catch return PQueueError.OutOfMemory;
        if (n == 0) return out;

        let i: usize = 0;
        while (i < n) : (i += 1) {
            out[i] = _.heap.items[i];
        }

        // Insertion sort descending (K is bounded; O(K^2) worst-case with zero extra allocation)
        let j: usize = 1;
        while (j < n) : (j += 1) {
            const key_item = out[j];
            let idx = j;
            while (idx > 0 && out[idx - 1].score < key_item.score) {
                out[idx] = out[idx - 1];
                idx -= 1;
            }
            out[idx] = key_item;
        }
        return out;
    }

    /// Returns the number of entries currently retained (`<= k`).
    pub fn len(): usize {
        return _.heap.items.len;
    }

    /// Clears all retained entries while preserving capacity.
    pub fn clear(*_) {
        _.heap.clearRetainingCapacity();
    }
}

/// Dual-heap streaming exact median tracker:
/// `low` is a max-heap storing the lower half; `high` is a min-heap storing the upper half.
/// Invariant: `low.len == high.len` or `low.len == high.len + 1`.
pub struct StreamingMedian {
    allocator: std.mem.Allocator,
    low: std.ArrayListUnmanaged(f64),
    high: std.ArrayListUnmanaged(f64),

    /// Initializes an empty `StreamingMedian` tracker.
    pub static fn init(allocator: std.mem.Allocator): StreamingMedian {
        return StreamingMedian{
            allocator: allocator,
            low: std.ArrayListUnmanaged(f64).empty,
            high: std.ArrayListUnmanaged(f64).empty,
        };
    }

    /// Releases both underlying heaps.
    pub fn deinit(*_) {
        _.low.deinit(_.allocator);
        _.high.deinit(_.allocator);
    }

    fn siftUpMax(*_, start_pos: usize) {
        let pos = start_pos;
        while (pos > 0) {
            const parent = (pos - 1) / 2;
            if (_.low.items[pos] > _.low.items[parent]) {
                const tmp = _.low.items[pos];
                _.low.items[pos] = _.low.items[parent];
                _.low.items[parent] = tmp;
                pos = parent;
            } else {
                break;
            }
        }
    }

    fn siftDownMax(*_, start_pos: usize) {
        let pos = start_pos;
        const n = _.low.items.len;
        while (true) {
            const left: usize = pos * 2 + 1;
            if (left >= n) break;
            let largest: usize = left;
            const right: usize = left + 1;
            if (right < n && _.low.items[right] > _.low.items[left]) {
                largest = right;
            }
            if (_.low.items[largest] > _.low.items[pos]) {
                const tmp = _.low.items[pos];
                _.low.items[pos] = _.low.items[largest];
                _.low.items[largest] = tmp;
                pos = largest;
            } else {
                break;
            }
        }
    }

    fn siftUpMin(*_, start_pos: usize) {
        let pos = start_pos;
        while (pos > 0) {
            const parent = (pos - 1) / 2;
            if (_.high.items[pos] < _.high.items[parent]) {
                const tmp = _.high.items[pos];
                _.high.items[pos] = _.high.items[parent];
                _.high.items[parent] = tmp;
                pos = parent;
            } else {
                break;
            }
        }
    }

    fn siftDownMin(*_, start_pos: usize) {
        let pos = start_pos;
        const n = _.high.items.len;
        while (true) {
            const left: usize = pos * 2 + 1;
            if (left >= n) break;
            let smallest: usize = left;
            const right: usize = left + 1;
            if (right < n && _.high.items[right] < _.high.items[left]) {
                smallest = right;
            }
            if (_.high.items[smallest] < _.high.items[pos]) {
                const tmp = _.high.items[pos];
                _.high.items[pos] = _.high.items[smallest];
                _.high.items[smallest] = tmp;
                pos = smallest;
            } else {
                break;
            }
        }
    }

    fn popMax(*_): f64 {
        const top = _.low.items[0];
        const last_idx = _.low.items.len - 1;
        if (last_idx > 0) {
            _.low.items[0] = _.low.items[last_idx];
            _.low.items.len = last_idx;
            _.siftDownMax(0);
        } else {
            _.low.items.len = 0;
        }
        return top;
    }

    fn popMin(*_): f64 {
        const top = _.high.items[0];
        const last_idx = _.high.items.len - 1;
        if (last_idx > 0) {
            _.high.items[0] = _.high.items[last_idx];
            _.high.items.len = last_idx;
            _.siftDownMin(0);
        } else {
            _.high.items.len = 0;
        }
        return top;
    }

    /// Inserts `val` into the dual-heap tracker while preserving the size balance invariant.
    pub fn push(*_, val: f64): void ! PQueueError {
        if (std.math.isNan(val)) return;
        if (_.low.items.len == 0 || val <= _.low.items[0]) {
            _.low.append(_.allocator, val) catch return PQueueError.OutOfMemory;
            _.siftUpMax(_.low.items.len - 1);
        } else {
            _.high.append(_.allocator, val) catch return PQueueError.OutOfMemory;
            _.siftUpMin(_.high.items.len - 1);
        }

        if (_.low.items.len > _.high.items.len + 1) {
            const moved = _.popMax();
            _.high.append(_.allocator, moved) catch return PQueueError.OutOfMemory;
            _.siftUpMin(_.high.items.len - 1);
        } else if (_.high.items.len > _.low.items.len) {
            const moved = _.popMin();
            _.low.append(_.allocator, moved) catch return PQueueError.OutOfMemory;
            _.siftUpMax(_.low.items.len - 1);
        }
    }

    /// Returns the exact median of all pushed values, or `EmptyQueue` if empty.
    pub fn median(): f64 ! PQueueError {
        if (_.low.items.len == 0 && _.high.items.len == 0) {
            return PQueueError.EmptyQueue;
        }
        if (_.low.items.len > _.high.items.len) {
            return _.low.items[0];
        }
        return (_.low.items[0] + _.high.items[0]) * 0.5;
    }

    /// Returns the total number of values pushed so far.
    pub fn count(): usize {
        return _.low.items.len + _.high.items.len;
    }

    /// Clears all values while retaining allocated heap capacity.
    pub fn clear(*_) {
        _.low.clearRetainingCapacity();
        _.high.clearRetainingCapacity();
    }
}
