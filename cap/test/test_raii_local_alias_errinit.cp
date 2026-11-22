fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        @panic(msg);
    }
}

// 1. 局部类型别名 RAII 自动析构（含 Factory(): type 匿名结构体内部）
let g_alias_drop_count: i32 = 0;

pub struct AliasGuard<T> {
    x: T,
    ~init() {
        g_alias_drop_count += 1;
    }
}

pub fn AliasFactory(): type {
    return struct {
        pub fn run(): i32 {
            const G = AliasGuard(i32);
            const g = G{ x: 42 };
            return g.x;
        }
    };
}

// 2. ~errinit() 仅在错误路径触发，正常返回自动跳过，零手动 disarm
let g_err_drop_count: i32 = 0;

pub struct ErrBufGuard {
    ptr: usize,
    ~errinit() {
        g_err_drop_count += 1;
    }
}

pub fn mayFailFlag(should_fail: bool): !void {
    if (should_fail) {
        return error.Boom;
    }
}

pub fn runErrInitGuard(should_fail: bool): !usize {
    const g = ErrBufGuard{ ptr: 99 };
    mayFailFlag(should_fail)?;
    return g.ptr;
}

// 2b. ~init() 配合 move(g) 撤防
let g_move_drop_count: i32 = 0;

pub struct MoveGuard {
    ptr: usize,
    ~init() {
        g_move_drop_count += 1;
    }
}

pub fn runMoveDisarm(should_fail: bool): !usize {
    const g = MoveGuard{ ptr: 77 };
    mayFailFlag(should_fail)?;
    const res = g.ptr;
    move(g);
    return res;
}

// 3. pub enum 只读实例方法带 *Printer 参数时保持只读 receiver（允许 *const RoundingStrategy 调用）
pub struct CssPrinter {
    last: []const u8,
    pub fn writeStr(*_, s: []const u8) {
        _.last = s;
    }
}

pub enum RoundingStrategy {
    nearest,
    up,

    fn asStr(): []const u8 {
        return switch (_) {
            .nearest => "nearest",
            .up => "up",
        };
    }

    fn toCss(dest: *CssPrinter) {
        dest.writeStr(_.asStr());
    }
}

fn callViaConstPtr(s: *const RoundingStrategy!, dest: *CssPrinter) {
    s.toCss(dest);
}

fn runAliasFactoryTest(): i32 {
    const F = AliasFactory();
    return F.run();
}

fn main() {
    // 验证 1: 局部别名 Guard(i32) 在 Factory() 内触发 ~init()
    const v = runAliasFactoryTest();
    assert(v == 42, "Factory run should return 42");
    assert(g_alias_drop_count == 1, "local alias Guard(i32) must run ~init()");

    // 验证 2a: ~errinit() 正常返回跳过、错误返回触发
    const ok_res = runErrInitGuard(false) catch 0;
    assert(ok_res == 99, "normal path should return 99");
    assert(g_err_drop_count == 0, "~errinit() must NOT run on normal return");

    const err_res = runErrInitGuard(true) catch 123;
    assert(err_res == 123, "error path should trigger catch");
    assert(g_err_drop_count == 1, "~errinit() MUST run on error return");

    // 验证 2b: move(g) 撤防
    const ok_m = runMoveDisarm(false) catch 0;
    assert(ok_m == 77, "move disarm normal path should return 77");
    assert(g_move_drop_count == 0, "move(g) must disarm ~init() on normal path");

    const err_m = runMoveDisarm(true) catch 456;
    assert(err_m == 456, "move disarm error path should catch");
    assert(g_move_drop_count == 1, "~init() must run when failing before move(g)");

    // 验证 3: pub enum 只读方法可由 *const RoundingStrategy 调用
    let p = CssPrinter{ last: "" };
    const strategy = RoundingStrategy.up;
    callViaConstPtr(&strategy, *p);
    assert(p.last.len == 2, "enum toCss via *const should write 'up'");
}
