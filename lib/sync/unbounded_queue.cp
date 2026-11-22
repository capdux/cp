// Source: Absorbed from /data/dev/bun/src/threading/unbounded_queue.cp | Synced: 2026-11-22T07:42:34Z
// ============================================================================
// Absorbed from Bun: bun/src/threading/unbounded_queue.cp
// Upstream Sync: Refer to bun/src/threading/unbounded_queue.zig for future upstream updates.
// ============================================================================
// Intrusive lock-free unbounded queue in CP
// Ported from Bun.js core threading architecture

pub fn UnboundedQueue(comptime T: type, comptime next_field: std.meta.FieldEnum(T)): type {
    const field_info = std.meta.fieldInfo(T, next_field);
    const next_name = field_info.name;

    return struct {
        const Self = @This();

        pub const Batch = struct {
            pub const Iterator = struct {
                batch: Self.Batch,

                pub fn next(*_): (*T)? {
                    if (_.batch.count == 0) return null;
                    if (front := _.batch.front) {
                        _.batch.front = @field(front, next_name);
                        _.batch.count -= 1;
                        return front;
                    }
                    return null;
                }
            };

            front: (*T)? = null,
            last: (*T)? = null,
            count: usize = 0,

            pub fn iterator(*_): Self.Batch.Iterator {
                return .{ .batch = _.* };
            }
        };

        zig {
            back: std.atomic.Value(?*T) = .{ .raw = null },
            front: std.atomic.Value(?*T) = .{ .raw = null },
        }

        pub fn push(*_, item: *T) {
            _.pushBatch(item, item);
        }

        pub fn pushBatch(*_, first: *T, last: *T) {
            zig {
                @field(last, next_name) = null;
                if (self.back.swap(last, .acq_rel)) |old_back| {
                    @atomicStore(?*T, &@field(old_back, next_name), first, .release);
                } else {
                    self.front.store(first, .release);
                }
            }
        }

        pub fn pop(*_): (*T)? {
            zig {
                var first = self.front.load(.acquire) orelse return null;
                const next_item = while (true) {
                    const next_ptr = @atomicLoad(?*T, &@field(first, next_name), .acquire);
                    const maybe_first = self.front.cmpxchgWeak(
                        first,
                        next_ptr,
                        .acquire,
                        .monotonic,
                    );
                    if (maybe_first) |actual_first| {
                        first = actual_first orelse return null;
                    } else {
                        break next_ptr;
                    }
                };

                if (next_item == null) {
                    _ = self.back.cmpxchgStrong(first, null, .acq_rel, .monotonic);
                }

                return first;
            }
        }

        pub fn popBatch(*_): (Self.Batch)? {
            zig {
                var first = self.front.load(.acquire) orelse return null;
                while (true) {
                    const maybe_first = self.front.cmpxchgWeak(
                        first,
                        null,
                        .acquire,
                        .monotonic,
                    );
                    if (maybe_first) |actual_first| {
                        first = actual_first orelse return null;
                    } else {
                        break;
                    }
                }

                const last = self.back.swap(null, .acq_rel) orelse unreachable;
                var count: usize = 1;
                var curr = first;
                while (curr != last) {
                    count += 1;
                    while (@atomicLoad(?*T, &@field(curr, next_name), .acquire)) |next_item| {
                        curr = next_item;
                        break;
                    }
                }

                return Self.Batch{
                    .front = first,
                    .last = last,
                    .count = count,
                };
            }
        }
    };
}
