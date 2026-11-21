// Source: Absorbed from /data/dev/bun/src/threading/unbounded_queue.cp

/// 当前目标架构缓存行大小（字节）
pub const CACHE_LINE_LENGTH: usize = std.atomic.cache_line;
/// 生产端/消费端防伪共享填充字节数
pub const QUEUE_PADDING_LENGTH: usize = if (CACHE_LINE_LENGTH >= 64) CACHE_LINE_LENGTH else 64;

/// 侵入式无锁无界并发队列（支持单元素 push/pop 与批量 pushBatch/popBatch，缓存行隔离防伪共享）。
/// `T` 为节点结构体，`next_field` 为其指向下一节点的侵入式指针字段（类型为 `(*T!)?` 或实现自定义原子访问器的紧凑指针类型）。
pub fn UnboundedQueue(comptime T: type, comptime next_field: std.meta.FieldEnum(T)): type {
    const field_info = std.meta.fieldInfo(T, next_field);
    const next_name = field_info.name;
    const FieldType = field_info.type;

    const has_custom_accessors = (@typeInfo(FieldType) != .optional)
        && @hasDecl(FieldType, "getPtr")
        && @hasDecl(FieldType, "setPtr")
        && @hasDecl(FieldType, "atomicLoadPtr")
        && @hasDecl(FieldType, "atomicStorePtr");

    return struct {
        const Self = @This();

        @inline fn ptrToAddr(ptr: (*T!)?): usize {
            if (p := ptr) {
                return @intFromPtr(p);
            }
            return 0;
        }

        @inline fn addrToPtr(addr: usize): (*T!)? {
            if (addr == 0) {
                return null;
            }
            const raw: *T! = @ptrFromInt(addr);
            return raw;
        }

        @inline fn getNext(item: *T!): (*T!)? {
            if (comptime has_custom_accessors) {
                return @field(item, next_name).getPtr();
            } else {
                return @field(item, next_name);
            }
        }

        @inline fn setNext(item: *T!, ptr: (*T!)?) {
            if (comptime has_custom_accessors) {
                const field_ptr: *FieldType! = &@field(item, next_name);
                field_ptr.setPtr(ptr);
            } else {
                @field(item, next_name) = ptr;
            }
        }

        @inline fn atomicLoadNext(item: *T!, ordering: std.builtin.AtomicOrder): (*T!)? {
            if (comptime has_custom_accessors) {
                const field_ptr: *FieldType! = &@field(item, next_name);
                return field_ptr.atomicLoadPtr(ordering);
            } else {
                return @atomicLoad(FieldType, &@field(item, next_name), ordering);
            }
        }

        @inline fn atomicStoreNext(item: *T!, ptr: (*T!)?, ordering: std.builtin.AtomicOrder) {
            if (comptime has_custom_accessors) {
                const field_ptr: *FieldType! = &@field(item, next_name);
                field_ptr.atomicStorePtr(ptr, ordering);
            } else {
                @atomicStore(FieldType, &@field(item, next_name), ptr, ordering);
            }
        }

        pub const Batch = struct {
            pub const Iterator = struct {
                batch: Self.Batch,

                fn next(*_): (*T!)? {
                    if (_.batch.count == 0) {
                        return null;
                    }
                    if (front_node := _.batch.front) {
                        _.batch.front = Self.getNext(front_node);
                        _.batch.count -= 1;
                        return front_node;
                    }
                    return null;
                }
            };

            front: (*T!)? = null,
            last: (*T!)? = null,
            count: usize = 0,

            fn iterator(): Self.Batch.Iterator {
                return Self.Batch.Iterator{ batch: _ };
            }

            fn isEmpty(): bool {
                return _.count == 0;
            }

            fn len(): usize {
                return _.count;
            }
        };

        back: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
        _pad_cache_line: [QUEUE_PADDING_LENGTH]u8 = std.mem.zeroes([QUEUE_PADDING_LENGTH]u8),
        front: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),

        static fn init(): Self {
            return Self{};
        }

        /// 入队单个节点（无锁，多线程安全）
        fn push(*_, item: *T!) {
            _.pushBatch(item, item);
        }

        /// 批量入队已链接的节点段 `[first .. last]`（无锁，多线程安全）
        fn pushBatch(*_, first: *T!, last: *T!) {
            Self.setNext(last, null);
            const old_back_addr = _.back.swap(@intFromPtr(last), .acq_rel);
            if (old_back := Self.addrToPtr(old_back_addr)) {
                Self.atomicStoreNext(old_back, first, .release);
            } else {
                _.front.store(@intFromPtr(first), .release);
            }
        }

        /// 出队单个节点，队列为空时返回 `null`
        fn pop(*_): (*T!)? {
            let first_addr = _.front.load(.acquire);
            if (first_addr == 0) {
                return null;
            }
            let first: *T! = @ptrFromInt(first_addr);
            let next_item: (*T!)? = null;
            while (true) {
                const next_ptr = Self.atomicLoadNext(first, .acquire);
                const next_addr = Self.ptrToAddr(next_ptr);
                if (maybe_first_addr := _.front.cmpxchgWeak(first_addr, next_addr, .release, .acquire)) {
                    if (maybe_first_addr == 0) {
                        return null;
                    }
                    first_addr = maybe_first_addr;
                    first = @ptrFromInt(first_addr);
                } else {
                    next_item = next_ptr;
                    break;
                }
            }
            if (next_item != null) {
                return first;
            }

            // `first` 为队列唯一节点，尝试原子清空 `back`
            if (_.back.cmpxchgStrong(first_addr, 0, .monotonic, .monotonic) == null) {
                return first;
            }

            // 存在并发生产者刚更新 `back`，自旋等待其发布 `first.next`
            while (true) {
                if (new_first := Self.atomicLoadNext(first, .acquire)) {
                    _.front.store(@intFromPtr(new_first), .release);
                    return first;
                }
                std.atomic.spinLoopHint();
            }
        }

        /// 一次性原子摘取当前队列全部节点为 `Batch`（适合高吞吐单消费者批处理）
        fn popBatch(*_): Self.Batch {
            let batch: Self.Batch = Self.Batch{ front: null, last: null, count: 0 };
            const first_addr = _.front.swap(0, .acquire);
            if (first_addr == 0) {
                return batch;
            }
            const first: *T! = @ptrFromInt(first_addr);
            batch.count = 1;

            const last_addr = _.back.swap(0, .monotonic);
            const last: *T! = @ptrFromInt(last_addr);
            let cur: *T! = first;
            while (cur != last) {
                while (true) {
                    if (nxt := Self.atomicLoadNext(cur, .acquire)) {
                        cur = nxt;
                        batch.count += 1;
                        break;
                    }
                    std.atomic.spinLoopHint();
                }
            }

            batch.front = first;
            batch.last = last;
            return batch;
        }

        /// 判断当前队列是否为空（基于 `back` 原子 Acquire 读取）
        fn isEmpty(): bool {
            return _.back.load(.acquire) == 0;
        }
    };
}
