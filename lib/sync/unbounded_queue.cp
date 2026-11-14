// Intrusive lock-free unbounded queue in CP
// Ported from Bun.js core threading architecture

pub fn UnboundedQueue(comptime T: type, comptime next_field: std.meta.FieldEnum(T)): type {
    const field_info = std.meta.fieldInfo(T, next_field);
    const next_name = field_info.name;

    const cache_line_length = switch (@import("builtin").target.cpu.arch) {
        .x86_64, .aarch64, .powerpc64 => 128,
        .arm, .mips, .mips64, .riscv64 => 32,
        .s390x => 256,
        else => 64,
    };

    return struct {
        const Self = @This();

        inline fn getNext(item: *T): (*T)? {
            return @field(item, next_name);
        }

        inline fn setNext(item: *T, ptr: (*T)?) {
            @field(item, next_name) = ptr;
        }

        inline fn atomicLoadNext(item: *T, ordering: std.builtin.AtomicOrder): (*T)? {
            return @atomicLoad((*T)?, &@field(item, next_name), ordering);
        }

        inline fn atomicStoreNext(item: *T, ptr: (*T)?, ordering: std.builtin.AtomicOrder) {
            @atomicStore((*T)?, &@field(item, next_name), ptr, ordering);
        }

        pub const Batch = struct {
            pub const Iterator = struct {
                batch: Self.Batch,

                pub fn next(*_): (*T)? {
                    if (_.batch.count == 0) return null;
                    const front = _.batch.front orelse unreachable;
                    _.batch.front = getNext(front);
                    _.batch.count -= 1;
                    return front;
                }
            };

            front: (*T)? = null,
            last: (*T)? = null,
            count: usize = 0,

            pub fn iterator(): Self.Batch.Iterator {
                return .{ .batch = _ };
            }
        };

        pub const queue_padding_length = cache_line_length / 2;

        back: std.atomic.Value((*T)?) align(queue_padding_length) = .init(null),
        front: std.atomic.Value((*T)?) align(queue_padding_length) = .init(null),

        pub fn push(*_, item: *T) {
            _.pushBatch(item, item);
        }

        pub fn pushBatch(*_, first: *T, last: *T) {
            setNext(last, null);
            if (_.back.swap(last, .acq_rel)) |old_back| {
                atomicStoreNext(old_back, first, .release);
            } else {
                _.front.store(first, .release);
            }
        }

        pub fn pop(*_): (*T)? {
            let first = _.front.load(.acquire) orelse return null;
            const next_item = while (true) {
                const next_ptr = atomicLoadNext(first, .acquire);
                const maybe_first = _.front.cmpxchgWeak(
                    first,
                    next_ptr,
                    .acquire,
                    .monotonic,
                );
                if (actual_first := maybe_first) {
                    first = actual_first;
                } else {
                    break next_ptr;
                }
            };

            if (next_item == null) {
                * = _.back.cmpxchgStrong(first, null, .acq_rel, .monotonic);
            }

            return first;
        }

        pub fn popBatch(*_): Self.Batch? {
            let first = _.front.load(.acquire) orelse return null;
            while (true) {
                const maybe_first = _.front.cmpxchgWeak(
                    first,
                    null,
                    .acquire,
                    .monotonic,
                );
                if (actual_first := maybe_first) {
                    first = actual_first;
                } else {
                    break;
                }
            }

            const last = _.back.swap(null, .acq_rel) orelse unreachable;
            let count: usize = 1;
            let curr = first;
            while (curr != last) {
                count += 1;
                while (atomicLoadNext(curr, .acquire)) |next_item| {
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
    };
}
