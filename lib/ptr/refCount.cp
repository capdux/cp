// Source: Absorbed from /data/dev/bun/src/ptr/ref_count.cp | Synced: 2026-11-22T12:49:17Z
// Intrusive single-threaded and thread-safe reference counting mixin & RefPtr smart pointer
import std from "std";
import { PtrThreadLock } from "./rawRefCount.cp";

pub struct RefCountOptions {
    debug_name: string? = null,
    destructor_ctx: type? = null,
}

pub fn RefCount(comptime T: type, comptime field_name: string, comptime destructor: auto, comptime options: RefCountOptions): type {
    const Ctx = options.destructor_ctx ?? void;
    const Destructor = if (options.destructor_ctx != null) fn (ptr: *T, ctx: Ctx): void else fn (ptr: *T): void;

    return struct {
        const Self = @This();

        raw_count: u32 = 1,
        thread: PtrThreadLock = PtrThreadLock.initLockedIfNonComptime(),

        const typed_destructor: Destructor = destructor;

        pub static fn init(): Self {
            return Self.initExactRefs(1);
        }

        pub static fn initExactRefs(count: u32): Self {
            if (count == 0) {
                @panic("RefCount count must be > 0");
            }
            return Self{
                raw_count: count,
                thread: PtrThreadLock.initLockedIfNonComptime(),
            };
        }

        pub static fn ref(target: *T) {
            let count = getRefCount(target);
            count.thread.lockOrAssert();
            count.raw_count += 1;
        }

        pub static fn unref(target: *T) {
            Self.deref(target);
        }

        pub static fn deref(target: *T) {
            let count = getRefCount(target);
            count.thread.lockOrAssert();
            if (count.raw_count == 0) {
                @panic("RefCount underflow");
            }
            count.raw_count -= 1;
            if (count.raw_count == 0) {
                if (comptime options.destructor_ctx != null) {
                    @compileError("destructor requires context, use derefWithContext");
                } else {
                    typed_destructor(target);
                }
            }
        }

        pub static fn derefWithContext(target: *T, ctx: Ctx) {
            let count = getRefCount(target);
            count.thread.lockOrAssert();
            if (count.raw_count == 0) {
                @panic("RefCount underflow");
            }
            count.raw_count -= 1;
            if (count.raw_count == 0) {
                if (comptime options.destructor_ctx != null) {
                    typed_destructor(target, ctx);
                } else {
                    typed_destructor(target);
                }
            }
        }

        pub fn get(): u32 {
            return _.raw_count;
        }

        pub fn hasOneRef(): bool {
            return _.get() == 1;
        }

        static fn getRefCount(target: *T): *Self {
            return *@field(target, field_name);
        }

        pub const is_ref_count = true;
        pub const ref_count_options = options;
    };
}

pub fn ThreadSafeRefCount(comptime T: type, comptime field_name: string, comptime destructor: auto, comptime options: RefCountOptions): type {
    const Ctx = options.destructor_ctx ?? void;
    const Destructor = if (options.destructor_ctx != null) fn (ptr: *T, ctx: Ctx): void else fn (ptr: *T): void;

    return struct {
        const Self = @This();

        raw_count: std.atomic.Value(u32),

        const typed_destructor: Destructor = destructor;

        pub static fn init(): Self {
            return Self.initExactRefs(1);
        }

        pub static fn initExactRefs(count: u32): Self {
            if (count == 0) {
                @panic("ThreadSafeRefCount count must be > 0");
            }
            return Self{
                raw_count: std.atomic.Value(u32).init(count),
            };
        }

        pub static fn ref(target: *T) {
            const count = getRefCount(target);
            const old = count.raw_count.fetchAdd(1, .monotonic);
            if (old == std.math.maxInt(u32)) {
                @panic("ThreadSafeRefCount overflow");
            }
        }

        pub static fn unref(target: *T) {
            Self.deref(target);
        }

        pub static fn deref(target: *T) {
            const count = getRefCount(target);
            const old = count.raw_count.fetchSub(1, .acq_rel);
            if (old == 0) {
                @panic("ThreadSafeRefCount underflow");
            }
            if (old == 1) {
                if (comptime options.destructor_ctx != null) {
                    @compileError("destructor requires context, use derefWithContext");
                } else {
                    typed_destructor(target);
                }
            }
        }

        pub static fn derefWithContext(target: *T, ctx: Ctx) {
            const count = getRefCount(target);
            const old = count.raw_count.fetchSub(1, .acq_rel);
            if (old == 0) {
                @panic("ThreadSafeRefCount underflow");
            }
            if (old == 1) {
                if (comptime options.destructor_ctx != null) {
                    typed_destructor(target, ctx);
                } else {
                    typed_destructor(target);
                }
            }
        }

        pub fn get(): u32 {
            return _.raw_count.load(.monotonic);
        }

        pub fn hasOneRef(): bool {
            return _.get() == 1;
        }

        static fn getRefCount(target: *T): *Self {
            return *@field(target, field_name);
        }

        pub const is_ref_count = true;
        pub const ref_count_options = options;
    };
}

pub fn RefPtr(comptime T: type): type {
    const RefCountMixin = @FieldType(T, "ref_count");
    const options = RefCountMixin.ref_count_options;
    const Ctx = options.destructor_ctx ?? void;

    return struct {
        const Self = @This();

        data: *T,

        pub static fn initRef(raw_ptr: *T): Self {
            RefCountMixin.ref(raw_ptr);
            return Self{ data: raw_ptr };
        }

        pub static fn adoptRef(raw_ptr: *T): Self {
            return Self{ data: raw_ptr };
        }

        pub static fn takeRef(raw_ptr: *T): Self {
            return Self{ data: raw_ptr };
        }

        pub fn deref(*_) {
            if (comptime options.destructor_ctx != null) {
                @compileError("destructor requires context, use derefWithContext");
            } else {
                RefCountMixin.deref(_.data);
            }
        }

        pub fn derefWithContext(*_, ctx: Ctx) {
            if (comptime options.destructor_ctx != null) {
                RefCountMixin.derefWithContext(_.data, ctx);
            } else {
                RefCountMixin.deref(_.data);
            }
        }

        pub fn dupeRef(): Self {
            return Self.initRef(_.data);
        }

        pub fn leak(*_): *T {
            const ptr = _.data;
            return ptr;
        }

        pub fn get(): *T {
            return _.data;
        }

        pub static fn new(init_data: T): Self {
            let ptr = std.heap.page_allocator.create(T) catch unreachable;
            ptr.* = init_data;
            return Self.adoptRef(ptr);
        }
    };
}
