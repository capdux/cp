// Regression suite for 2026-11-23 6 compiler hotfixes

// --- 1. Passthrough factory lowering (struct / nested struct / enum) ---
fn MakeBox(comptime T: type): type {
    return struct {
        val: T? = null,
        pub fn get(): T? {
            return _.val;
        }
        pub fn getOr(fallback: T): T {
            return _.val ?? fallback;
        }
        pub const Inner = struct {
            item: T,
            pub fn read(): T {
                return _.item;
            }
        };
    };
}

fn MakeKind(): type {
    return enum(u8) {
        a = 0,
        b = 1,
        pub fn isA(): bool {
            return _ == .a;
        }
    };
}

// --- 2. Optional type '?' vs try '?' disambiguation ---
struct Owner {
    x: i32,
}

fn optSizeGeneric(comptime T: type): usize {
    const OptT = T?;
    return @sizeOf(OptT) + @sizeOf((*T)?);
}

fn testOptSize(): usize {
    const Opt = Owner?;
    return @sizeOf((*Owner)?) + @sizeOf(Opt);
}

// --- 3. Addr-of const var vs type & `const *!` single-layer pointer ---
struct File {
    fd: i32,
}

const GLOBAL_FILE: File = File{ fd: 99 };

struct Sink {
    fn setup(p: *const File): i32 {
        return p.fd;
    }
    fn start(f: File): i32 {
        const file = f;
        return _.setup(&file);
    }
    fn readGlobal(): i32 {
        return _.setup(&GLOBAL_FILE);
    }
}

unsafe fn acceptConstRawPtr(p: const *!): usize {
    return @intFromPtr(p);
}

// --- 4. @as(EnumTag, _) on tagged union(EnumTag) ---
enum Tag {
    a,
    b,
}

union(Tag) Val {
    a: i32,
    b: bool,
    fn tag(): Tag {
        return @as(Tag, _);
    }
}

// --- 5. Zero-field struct instance method receiver preservation ---
pub struct FeaturesFormatter {
    pub fn format(writer: auto): i32 {
        * = writer;
        return 77;
    }
    pub static fn staticTag(): i32 {
        return 88;
    }
}

fn assertEq(actual: i32, expected: i32) {
    if (actual != expected) {
        @panic("assertion failed");
    }
}

fn assertTrue(cond: bool) {
    if (!cond) {
        @panic("assertion failed");
    }
}

fn runAll() {
    // 1. Passthrough factory lowering
    const I32Box = MakeBox(i32);
    const b1 = I32Box{ val: 42 };
    const b2 = I32Box{};
    assertEq(b1.getOr(0), 42);
    assertEq(b2.getOr(9), 9);

    const inner = I32Box.Inner{ item: 123 };
    assertEq(inner.read(), 123);

    const K = MakeKind();
    const ka = K.a;
    const kb = K.b;
    assertTrue(ka.isA());
    assertTrue(!kb.isA());

    // 2. Optional type '?' vs try '?' disambiguation
    assertTrue(testOptSize() > 0);
    assertTrue(optSizeGeneric(Owner) == testOptSize());

    // 3. Addr-of const var & const *! single-layer pointer
    const s = Sink{};
    assertEq(s.start(File{ fd: 17 }), 17);
    assertEq(s.readGlobal(), 99);

    const local_val: i32 = 55;
    unsafe {
        const addr = acceptConstRawPtr(@ptrCast(&local_val));
        assertTrue(addr != 0);
    }

    // 4. @as(Tag, _) on tagged union
    const va = Val{ a: 10 };
    const vb = Val{ b: true };
    assertTrue(va.tag() == Tag.a);
    assertTrue(vb.tag() == Tag.b);
    const from_int = @as(Tag, 1);
    assertTrue(from_int == Tag.b);

    // 5. Zero-field struct instance method receiver
    const f = FeaturesFormatter{};
    assertEq(f.format(123), 77);
    assertEq(FeaturesFormatter.staticTag(), 88);
}

fn main() {
    runAll();
}
