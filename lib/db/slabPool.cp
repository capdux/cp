// Source: native-cp-stdlib (Generational O(1) Slab Object Pool)
const std = @import("std");

const INVALID_INDEX: u32 = std.math.maxInt(u32);

pub const SlabError = error {
    InvalidCapacity,
    PoolFull,
    InvalidHandle,
    StaleHandle,
    OutOfMemory,
};

pub struct Handle {
    index: u32,
    generation: u32,

    pub fn eql(other: Handle): bool {
        return _.index == other.index && _.generation == other.generation;
    }
}

pub fn SlabPool(comptime T: type): type {
    const Slot = struct {
        generation: u32,
        next_free: u32,
        occupied: bool,
        value: T,
    };

    return struct {
        const Self = @This();

        allocator: std.mem.Allocator,
        slots: []Slot,
        free_head: u32,
        active_count: usize,
        cap_size: usize,

        pub fn init(alloc: std.mem.Allocator, cap_limit: usize): SlabError!Self {
            if (cap_limit == 0 || cap_limit >= INVALID_INDEX) {
                return SlabError.InvalidCapacity;
            }
            let buf = alloc.alloc(Slot, cap_limit) catch return SlabError.OutOfMemory;
            let i: usize = 0;
            while (i < cap_limit) {
                buf[i].generation = 1;
                buf[i].occupied = false;
                buf[i].value = undefined;
                if (i + 1 < cap_limit) {
                    buf[i].next_free = @intCast(i + 1);
                } else {
                    buf[i].next_free = INVALID_INDEX;
                }
                i += 1;
            }
            return Self {
                allocator: alloc,
                slots: buf,
                free_head: 0,
                active_count: 0,
                cap_size: cap_limit,
            };
        }

        pub fn deinit(*_) {
            _.allocator.free(_.slots);
            _.slots = &.{};
            _.free_head = INVALID_INDEX;
            _.active_count = 0;
            _.cap_size = 0;
        }

        fn validateHandle(h: Handle): SlabError!usize {
            if (h.generation == 0) {
                return SlabError.InvalidHandle;
            }
            const idx: usize = @intCast(h.index);
            if (idx >= _.cap_size) {
                return SlabError.InvalidHandle;
            }
            if (!_.slots[idx].occupied) {
                return SlabError.StaleHandle;
            }
            if (_.slots[idx].generation != h.generation) {
                return SlabError.StaleHandle;
            }
            return idx;
        }

        pub fn insert(*_, val: T): SlabError!Handle {
            if (_.free_head == INVALID_INDEX) {
                return SlabError.PoolFull;
            }
            const slot_idx_u32: u32 = _.free_head;
            const idx: usize = @intCast(slot_idx_u32);
            _.free_head = _.slots[idx].next_free;
            _.slots[idx].occupied = true;
            _.slots[idx].value = val;
            _.active_count += 1;
            return Handle {
                index: slot_idx_u32,
                generation: _.slots[idx].generation,
            };
        }

        pub fn get(*_, h: Handle): SlabError!*T {
            const idx: usize = _.validateHandle(h)?;
            return &_.slots[idx].value;
        }

        pub fn getConst(h: Handle): SlabError!T {
            const idx: usize = _.validateHandle(h)?;
            return _.slots[idx].value;
        }

        pub fn remove(*_, h: Handle): SlabError!T {
            const idx: usize = _.validateHandle(h)?;
            const removed_val: T = _.slots[idx].value;
            _.slots[idx].occupied = false;
            _.slots[idx].value = undefined;
            let next_gen: u32 = _.slots[idx].generation +% 1;
            if (next_gen == 0) {
                next_gen = 1;
            }
            _.slots[idx].generation = next_gen;
            _.slots[idx].next_free = _.free_head;
            _.free_head = h.index;
            _.active_count -= 1;
            return removed_val;
        }

        pub fn isValid(h: Handle): bool {
            if (h.generation == 0) {
                return false;
            }
            const idx: usize = @intCast(h.index);
            if (idx >= _.cap_size) {
                return false;
            }
            return _.slots[idx].occupied && _.slots[idx].generation == h.generation;
        }

        pub fn len(): usize {
            return _.active_count;
        }

        pub fn capacity(): usize {
            return _.cap_size;
        }

        pub fn clear(*_) {
            let i: usize = 0;
            while (i < _.cap_size) {
                if (_.slots[i].occupied) {
                    _.slots[i].occupied = false;
                    _.slots[i].value = undefined;
                    let next_gen: u32 = _.slots[i].generation +% 1;
                    if (next_gen == 0) {
                        next_gen = 1;
                    }
                    _.slots[i].generation = next_gen;
                }
                if (i + 1 < _.cap_size) {
                    _.slots[i].next_free = @intCast(i + 1);
                } else {
                    _.slots[i].next_free = INVALID_INDEX;
                }
                i += 1;
            }
            _.free_head = 0;
            _.active_count = 0;
        }

        pub fn reset(*_) {
            _.clear();
        }

        pub fn forEachActive(*_, ctx: anytype, cb: anytype) {
            let i: usize = 0;
            while (i < _.cap_size) {
                if (_.slots[i].occupied) {
                    const h = Handle {
                        index: @intCast(i),
                        generation: _.slots[i].generation,
                    };
                    cb(ctx, h, &_.slots[i].value);
                }
                i += 1;
            }
        }
    };
}
