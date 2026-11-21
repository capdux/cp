import { test, expect_eq } from "cap:test";

let g_struct_drops: i32 = 0;
let g_class_drops: i32 = 0;
let g_bun_handler_drops: i32 = 0;
let g_multi_arg_release_calls: i32 = 0;

struct FileHandle {
    fd: i32,

    pub static fn open(fd: i32): FileHandle {
        return FileHandle { fd: fd };
    }

    pub fn getFd(): i32 {
        return _.fd;
    }

    pub fn setFd(*_, fd: i32) {
        _.fd = fd;
    }

    ~init() {
        g_struct_drops = g_struct_drops + 1;
        _.fd = -1;
    }
}

class HeapResource {
    id: i32;

    fn init(id: i32) {
        _.id = id;
    }

    pub fn getId(): i32 {
        return _.id;
    }

    pub fn setId(*_, id: i32) {
        _.id = id;
    }

    ~init() {
        g_class_drops = g_class_drops + 1;
    }
}

// Repro 1: Bun-style struct with legacy business method `release(*_)` inside container
struct BunHandlers {
    tag: i32,

    pub fn release(*_) {
        g_bun_handler_drops = g_bun_handler_drops + 1;
    }
}

struct BunServer {
    handlers: BunHandlers,
    port: i32,

    pub fn readHandlersTag(): i32 {
        // Field read WITHOUT `move`: MUST NOT auto-drop `_.handlers`!
        const h = _.handlers;
        return h.tag;
    }
}

// Repro 2: Multi-arg `release(*_, slot: i32)` is a pool method, NOT a destructor!
struct SlotPool {
    count: i32,

    pub fn release(*_, slot: i32) {
        _.count = _.count - slot;
        g_multi_arg_release_calls = g_multi_arg_release_calls + 1;
    }
}

struct PodPair {
    x: i32,
    y: i32,
}

struct InnerVal {
    v: i32,
}

struct TwoFields {
    left: InnerVal,
    right: InnerVal,
    label: i32,
}

struct ResourceHolder {
    res: FileHandle,
    tag: i32,
}

fn passRawPtr(p: *FileHandle!): i32 {
    // Raw pointer *T! MUST NEVER auto-drop when function returns!
    unsafe {
        return p.*.fd;
    }
}

fn runAutoDropOnce(): i32 {
    let f = FileHandle.open(7);
    let r = new HeapResource(99);
    return f.getFd() + r.getId();
}

fn runIdempotentManualDeinit(): i32 {
    let f = FileHandle.open(10);
    f.~init();
    f.~init();
    f.~init();

    const cf = FileHandle.open(11);
    cf.~init();
    cf.~init();

    let r = new HeapResource(42);
    r.~init();
    r.~init();
    return g_struct_drops * 10 + g_class_drops;
}

fn runDropBuiltin(): i32 {
    let f = FileHandle.open(20);
    drop(f);

    let r = new HeapResource(21);
    drop(move r);
    return g_struct_drops * 10 + g_class_drops;
}

fn runBuiltinContainersIdempotent(): i32 {
    let m = new Map();
    m.set("k", 123);
    let ms = m.size;
    m.release();
    m.release();

    let s = new Set();
    s.add("a");
    let ss = s.size;
    s.release();
    s.release();

    let q = new VecDeque();
    q.push_back(42);
    let qs = q.size();
    q.release();
    q.release();
    return ms + ss + qs;
}

fn runBunRepros(): i32 {
    let srv = BunServer { handlers: BunHandlers { tag: 77 }, port: 3000 };
    let t = srv.readHandlersTag();

    let fh = FileHandle.open(55);
    let raw_p: *FileHandle! = &fh;
    let got = passRawPtr(raw_p);

    let pool = SlotPool { count: 10 };
    // Verify multi-arg `release` is an ordinary method and NOT truncated or auto-dropped!
    pool.release(3);
    // Verify 0-arg `release` on user struct is ALSO an ordinary method (NOT auto-dropped on scope exit!)
    let bh = BunHandlers { tag: 1 };
    return t + got + pool.count + bh.tag;
}

fn runDirectAssign3Mode(): i32 {
    // POD value copy
    let p1 = PodPair { x: 1, y: 2 };
    let p2 = p1;
    p2.x = 100;

    // Default read-only borrow on RAII class (does NOT move `a`, does NOT drop `b`!)
    let a = new HeapResource(50);
    const b = a;
    let v1 = b.getId() + a.getId();

    // Mutable borrow (*a) mutates in place, does NOT drop `m`!
    const m = *a;
    m.setId(60);
    let v2 = a.getId();

    // Explicit ownership transfer (move a) transfers single drop to `c`
    const c = move a;
    let v3 = c.getId();
    return p1.x + p2.x + v1 + v2 + v3;
}

fn runDestructuringMoveWhole(): i32 {
    let h1 = ResourceHolder { res: FileHandle.open(88), tag: 1 };
    const { res: moved_r1, tag: t1 } = move h1;
    return moved_r1.getFd() + t1;
}

fn runDestructuringMoveField(): i32 {
    let h2 = ResourceHolder { res: FileHandle.open(89), tag: 2 };
    const { move res: moved_r2, tag: t2 } = h2;
    return moved_r2.getFd() + t2;
}

fn runDestructuringBorrowsSync(): i32 {
    let box = TwoFields {
        left: InnerVal { v: 10 },
        right: InnerVal { v: 20 },
        label: 1,
    };
    {
        const { left: ml, right: mr } = *box;
        ml.v = 11;
        mr.v = 22;
    }
    {
        const { *left: dl, *right: dr, label } = box;
        dl.v = 100 + label;
        dr.v = 200 + label;
    }
    let p = PodPair { x: 3, y: 4 };
    {
        const { *x: mx, *y: my } = p;
        mx.* = 30;
        my.* = 40;
    }
    return box.left.v + box.right.v + p.x + p.y;
}

fn consumeOwnHandle(own fh: FileHandle): i32 {
    return fh.getFd() * 2;
}

fn runOwnAutoDrop(): i32 {
    let f1 = FileHandle.open(33);
    let doubled = consumeOwnHandle(own f1);

    let f2 = FileHandle.open(44);
    let owned_b = own f2;
    return doubled + owned_b.getFd();
}

fn main() {
    test("1. ~init() auto-drop on struct and class (exactly once)", () => {
        g_struct_drops = 0;
        g_class_drops = 0;
        let sum = runAutoDropOnce();
        expect_eq(sum, 106);
        expect_eq(g_struct_drops, 1);
        expect_eq(g_class_drops, 1);
    });

    test("2. ~init() double idempotency (manual ~init() x3 + scope exit == 1 drop)", () => {
        g_struct_drops = 0;
        g_class_drops = 0;
        let mid = runIdempotentManualDeinit();
        expect_eq(mid, 21);
        expect_eq(g_struct_drops, 2);
        expect_eq(g_class_drops, 1);
    });

    test("3. drop(x) consumes ownership and prevents double-drop", () => {
        g_struct_drops = 0;
        g_class_drops = 0;
        let mid = runDropBuiltin();
        expect_eq(mid, 11);
        expect_eq(g_struct_drops, 1);
        expect_eq(g_class_drops, 1);
    });

    test("4. Builtin Map, Set, VecDeque manual release() idempotency", () => {
        let total = runBuiltinContainersIdempotent();
        expect_eq(total, 3);
    });

    test("5. Bun repros: field read, raw pointer *T!, multi-arg release() never false-drop", () => {
        g_bun_handler_drops = 0;
        g_struct_drops = 0;
        g_multi_arg_release_calls = 0;
        let sum = runBunRepros();
        expect_eq(sum, 140);
        expect_eq(g_bun_handler_drops, 0);
        expect_eq(g_struct_drops, 1);
        expect_eq(g_multi_arg_release_calls, 1);
    });

    test("6. Direct assignment 3-mode (read-only borrow, *mut borrow, explicit move) + POD copy", () => {
        g_class_drops = 0;
        let sum = runDirectAssign3Mode();
        expect_eq(sum, 321);
        expect_eq(g_class_drops, 1);
    });

    test("7. Object destructuring 5-mode & disjoint member mutable borrow (sync + state machine)", () => {
        // Sync function destructuring
        expect_eq(runDestructuringBorrowsSync(), 372);

        g_struct_drops = 0;
        expect_eq(runDestructuringMoveWhole(), 89);
        expect_eq(g_struct_drops, 1);

        g_struct_drops = 0;
        expect_eq(runDestructuringMoveField(), 91);
        expect_eq(g_struct_drops, 1);

        // State machine destructuring (inside test lambda)
        let box = TwoFields {
            left: InnerVal { v: 10 },
            right: InnerVal { v: 20 },
            label: 5,
        };
        const { left: ml, right: mr } = *box;
        ml.v = 15;
        mr.v = 25;
        expect_eq(box.left.v, 15);
        expect_eq(box.right.v, 25);
    });

    test("8. own exclusive ownership with ~init() auto-drops exactly once per instance", () => {
        g_struct_drops = 0;
        let res = runOwnAutoDrop();
        expect_eq(res, 110);
        expect_eq(g_struct_drops, 2);
    });
}
