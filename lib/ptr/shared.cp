// Source: /data/dev/bun/src/ptr/shared.cp
// Thread-safe and single-threaded reference counted shared pointer体系 (Shared<T>, AtomicShared<T>, Weak<T>)
import std from "std";
import { PointerInfo, PointerKind } from "./meta.cp";
import { PtrThreadLock } from "./raw_ref_count.cp";

fn parsePointer(comptime Pointer: type): PointerInfo {
    const parse_opts = PointerInfo.ParseOptions{
        allow_const: false,
        allow_slices: false,
    };
    return PointerInfo.parse(Pointer, parse_opts);
}

pub struct AtomicCount {
    value: std.atomic.Value(u32),

    pub static fn init(val: u32): AtomicCount {
        return AtomicCount{ value: std.atomic.Value(u32).init(val) };
    }

    pub fn get(): u32 {
        return _.value.load(.acquire);
    }

    pub fn increment(*_) {
        const old = _.value.fetchAdd(1, .monotonic);
        if (old == std.math.maxInt(u32)) {
            @panic("overflow of atomic ref count");
        }
    }

    pub fn tryIncrement(*_): bool {
        let current = _.value.load(.monotonic);
        while (true) {
            if (current == 0) return false;
            if (next := _.value.cmpxchgWeak(current, current + 1, .monotonic, .monotonic)) {
                current = next;
            } else {
                return true;
            }
        }
    }

    pub fn decrement(*_): u32 {
        const old = _.value.fetchSub(1, .acq_rel);
        if (old == 0) {
            @panic("underflow of atomic ref count");
        }
        return old - 1;
    }
}

pub struct NonAtomicCount {
    value: u32 = 0,

    pub static fn init(val: u32): NonAtomicCount {
        return NonAtomicCount{ value: val };
    }

    pub fn get(): u32 {
        return _.value;
    }

    pub fn increment(*_) {
        _.value += 1;
    }

    pub fn tryIncrement(*_): bool {
        if (_.value == 0) return false;
        _.value += 1;
        return true;
    }

    pub fn decrement(*_): u32 {
        if (_.value == 0) {
            @panic("underflow of non-atomic ref count");
        }
        _.value -= 1;
        return _.value;
    }
}

pub struct SharedOptions {
    Allocator: type = std.mem.Allocator,
    atomic: bool = false,
    allow_weak: bool = false,
    deinit: bool = true,
}

pub fn FullData(comptime Child: type, comptime options: SharedOptions): type {
    const Allocator = options.Allocator;
    const Count = if (options.atomic) AtomicCount else NonAtomicCount;
    const WeakCountField = if (options.allow_weak) Count else void;
    const ThreadLockField = if (options.atomic) void else PtrThreadLock;

    return struct {
        const Self = @This();

        value: Child,
        strong_count: Count,
        weak_count: WeakCountField,
        allocator: Allocator,
        thread_lock: ThreadLockField,

        pub static fn fromValuePtrOpt(ptr: (*Child)?): (*Self)? {
            if (non_opt_ptr := ptr) {
                return @alignCast(@fieldParentPtr("value", non_opt_ptr));
            }
            return null;
        }

        pub static fn fromValuePtrNonOpt(ptr: *Child): *Self {
            return @alignCast(@fieldParentPtr("value", ptr));
        }

        pub fn valuePtr(*_): *Child {
            return *_.value;
        }

        pub static fn alloc(allocator: Allocator, val: Child): !(*Self) {
            let ptr = try allocator.create(Self);
            ptr.* = Self{
                value: val,
                strong_count: Count.init(1),
                weak_count: if (options.allow_weak) Count.init(1) else {},
                allocator: allocator,
                thread_lock: if (comptime !options.atomic) PtrThreadLock.initLockedIfNonComptime() else {},
            };
            return ptr;
        }

        pub fn strongCount(): usize {
            return _.strong_count.get() as usize;
        }

        pub fn rawWeakCount(): usize {
            return if (comptime options.allow_weak) (_.weak_count.get() as usize) else 1;
        }

        pub fn incrementStrong(*_) {
            _.assertThreadSafety();
            _.strong_count.increment();
        }

        pub fn tryIncrementStrong(*_): bool {
            return _.strong_count.tryIncrement();
        }

        pub fn decrementStrong(*_) {
            _.assertThreadSafety();
            if (_.strong_count.decrement() == 0) {
                _.deinitValue();
                _.decrementWeak();
            }
        }

        pub fn incrementWeak(*_) {
            _.assertThreadSafety();
            if (comptime options.allow_weak) {
                _.weak_count.increment();
            }
        }

        pub fn decrementWeak(*_) {
            _.assertThreadSafety();
            if ((comptime !options.allow_weak) || _.weak_count.decrement() == 0) {
                _.destroy();
            }
        }

        fn deinitValue(*_) {
            if (comptime options.deinit) {
                if (comptime (@typeInfo(Child) == .@"struct" or @typeInfo(Child) == .@"union") and @hasDecl(Child, "deinit")) {
                    _.value.deinit();
                }
            }
        }

        fn destroy(*_) {
            const alloc_inst = _.allocator;
            alloc_inst.destroy(_);
        }

        fn assertThreadSafety() {
            if (comptime !options.atomic) {
                _.thread_lock.assertLocked();
            }
        }
    };
}

pub fn WithOptions(comptime Pointer: type, comptime options: SharedOptions): type {
    const info = parsePointer(Pointer);
    const Child = info.Child;
    const NonOptionalPointer = info.NonOptionalPointer;
    const Data = FullData(Child, options);
    const Allocator = options.Allocator;

    return struct {
        const Self = @This();

        ptr: Pointer,

        pub const Weak = if (options.allow_weak) WeakWithOptions(Pointer, options) else void;

        pub static fn alloc(value: Child): !Self {
            return Self.allocIn(value, std.heap.page_allocator);
        }

        pub static fn allocIn(value: Child, allocator: Allocator): !Self {
            const data = try Data.alloc(allocator, value);
            return Self{ ptr: data.valuePtr() };
        }

        pub static fn new(value: Child): Self {
            return Self.alloc(value) catch unreachable;
        }

        pub static fn newIn(value: Child, allocator: Allocator): Self {
            return Self.allocIn(value, allocator) catch unreachable;
        }

        pub fn get(): Pointer {
            return _.ptr;
        }

        pub fn clone(): Self {
            let data: *Data = undefined;
            if (comptime info.isOptional()) {
                if (d := _.getData()) {
                    data = d;
                } else {
                    return Self.initNull();
                }
            } else {
                data = _.getData();
            }
            data.incrementStrong();
            return Self{ ptr: data.valuePtr() };
        }

        pub fn cloneWeak(): Self.Weak {
            let data: *Data = undefined;
            if (comptime info.isOptional()) {
                if (d := _.getData()) {
                    data = d;
                } else {
                    return Self.Weak.initNull();
                }
            } else {
                data = _.getData();
            }
            data.incrementWeak();
            return Self.Weak{ ptr: data.valuePtr() };
        }

        pub fn deinit(*_) {
            let data: *Data = undefined;
            if (comptime info.isOptional()) {
                if (d := _.getData()) {
                    data = d;
                } else {
                    return;
                }
            } else {
                data = _.getData();
            }
            data.decrementStrong();
        }

        pub fn strongCount(): usize {
            if (comptime info.isOptional()) {
                if (d := _.getData()) {
                    return d.strongCount();
                }
                return 0;
            } else {
                return _.getData().strongCount();
            }
        }

        pub fn weakCount(): usize {
            if (comptime options.allow_weak) {
                if (comptime info.isOptional()) {
                    if (d := _.getData()) {
                        const raw = d.rawWeakCount();
                        return if (d.strongCount() > 0) raw - 1 else raw;
                    }
                    return 0;
                } else {
                    const d = _.getData();
                    const raw = d.rawWeakCount();
                    return if (d.strongCount() > 0) raw - 1 else raw;
                }
            }
            return 0;
        }

        pub fn isUnique(): bool {
            return _.strongCount() == 1;
        }

        pub fn leak(*_): Pointer {
            const p = _.ptr;
            return p;
        }

        pub static fn adoptRawUnsafe(p: Pointer): Self {
            return Self{ ptr: p };
        }

        pub static fn cloneFromRawUnsafe(p: Pointer): Self {
            const temp = Self{ ptr: p };
            return temp.clone();
        }

        static fn initNullImpl(): Self {
            return Self{ ptr: null };
        }

        pub const initNull = if (info.isOptional()) initNullImpl;

        fn getDataOpt(): (*Data)? {
            return Data.fromValuePtrOpt(_.ptr);
        }

        fn getDataNonOpt(): *Data {
            return Data.fromValuePtrNonOpt(_.ptr);
        }

        const getData = if (info.isOptional()) getDataOpt else getDataNonOpt;
    };
}

pub fn WeakWithOptions(comptime Pointer: type, comptime options: SharedOptions): type {
    const info = parsePointer(Pointer);
    const Child = info.Child;
    const NonOptionalPointer = info.NonOptionalPointer;
    const Data = FullData(Child, options);
    const SharedNonOptional = WithOptions(NonOptionalPointer, options);

    return struct {
        const Self = @This();

        ptr: Pointer,

        pub fn upgrade(): SharedNonOptional? {
            let data: *Data = undefined;
            if (comptime info.isOptional()) {
                if (d := _.getData()) {
                    data = d;
                } else {
                    return null;
                }
            } else {
                data = _.getData();
            }
            if (!data.tryIncrementStrong()) {
                return null;
            }
            return SharedNonOptional{ ptr: data.valuePtr() };
        }

        pub fn clone(): Self {
            let data: *Data = undefined;
            if (comptime info.isOptional()) {
                if (d := _.getData()) {
                    data = d;
                } else {
                    return Self.initNull();
                }
            } else {
                data = _.getData();
            }
            data.incrementWeak();
            return Self{ ptr: data.valuePtr() };
        }

        pub fn deinit(*_) {
            let data: *Data = undefined;
            if (comptime info.isOptional()) {
                if (d := _.getData()) {
                    data = d;
                } else {
                    return;
                }
            } else {
                data = _.getData();
            }
            data.decrementWeak();
        }

        static fn initNullImpl(): Self {
            return Self{ ptr: null };
        }

        pub const initNull = if (info.isOptional()) initNullImpl;

        fn getDataOpt(): (*Data)? {
            return Data.fromValuePtrOpt(_.ptr);
        }

        fn getDataNonOpt(): *Data {
            return Data.fromValuePtrNonOpt(_.ptr);
        }

        const getData = if (info.isOptional()) getDataOpt else getDataNonOpt;
    };
}

pub fn Shared(comptime Pointer: type): type {
    return WithOptions(Pointer, SharedOptions{
        Allocator: std.mem.Allocator,
        atomic: false,
        allow_weak: true,
        deinit: true,
    });
}

pub fn AtomicShared(comptime Pointer: type): type {
    return WithOptions(Pointer, SharedOptions{
        Allocator: std.mem.Allocator,
        atomic: true,
        allow_weak: true,
        deinit: true,
    });
}

pub fn Weak(comptime Pointer: type): type {
    return WeakWithOptions(Pointer, SharedOptions{
        Allocator: std.mem.Allocator,
        atomic: false,
        allow_weak: true,
        deinit: true,
    });
}

pub fn AtomicWeak(comptime Pointer: type): type {
    return WeakWithOptions(Pointer, SharedOptions{
        Allocator: std.mem.Allocator,
        atomic: true,
        allow_weak: true,
        deinit: true,
    });
}
