// Source: /data/dev/bun/src/ptr/weak_ptr.cp
// Intrusive weak pointer tracking for objects with deferred destruction
import std from "std";

pub @packed(u32) struct WeakPtrData {
    reference_count: u31 = 0,
    finalized: bool = false,

    pub static fn initEmpty(): WeakPtrData {
        return WeakPtrData{
            reference_count: 0,
            finalized: false,
        };
    }

    pub const empty: WeakPtrData = initEmpty();

    pub fn onFinalize(*_): bool {
        if (_.finalized) {
            @panic("WeakPtrData already finalized");
        }
        _.finalized = true;
        return _.reference_count == 0;
    }
}

pub fn WeakPtr(comptime T: type, comptime data_field: []const u8): type {
    return struct {
        const Self = @This();
        pub const Data = WeakPtrData;

        raw_ptr: (*T)? = null,

        pub static fn initEmpty(): Self {
            return Self{ raw_ptr: null };
        }

        pub const empty: Self = initEmpty();

        pub static fn initRef(req: *T): Self {
            if (data(req).finalized) {
                @panic("Attempted to create WeakPtr from finalized target");
            }
            data(req).reference_count += 1;
            return Self{ raw_ptr: req };
        }

        pub fn deref(*_) {
            if (value := _.raw_ptr) {
                _.derefInternal(value);
            }
        }

        pub fn get(*_): (*T)? {
            if (value := _.raw_ptr) {
                if (!data(value).finalized) {
                    return value;
                }

                _.derefInternal(value);
            }
            return null;
        }

        fn derefInternal(*_, value: *T) {
            _.raw_ptr = null;
            let weak_data = data(value);
            if (weak_data.reference_count == 0) {
                @panic("WeakPtr underflow");
            }
            const count = weak_data.reference_count - 1;
            weak_data.reference_count = count;
            if (weak_data.finalized && count == 0) {
                std.heap.page_allocator.destroy(value);
            }
        }

        static fn data(value: *T): *WeakPtrData {
            return *@field(value, data_field);
        }
    };
}
