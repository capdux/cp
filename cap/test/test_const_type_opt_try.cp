fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        @panic(msg);
    }
}

// 1. 顶层 const 类型别名右值使用后缀可选类型 (T)? 与 T?
pub const FnA = (*const fn (_: i32) callconv(.c) i32)?;
pub const FnB = *const fn (_: i32) callconv(.c) i32;
pub const OptFnB = FnB?;
pub const OptPtr = (*u8)?;

// 2. struct 容器内 const 类型别名右值使用后缀可选类型
pub struct ContainerTypes {
    pub const FnA = (*const fn (_: i32) callconv(.c) i32)?;
    pub const OptFnB = FnB?;
    pub const OptPtr = (*u8)?;
    val: i32,
}

fn addOneC(x: i32) callconv(.c): i32 {
    return x + 1;
}

// 3. 函数体内正常 expr? 错误传播零回归
fn mayFailVal(should_fail: bool, v: i32): !i32 {
    if (should_fail) {
        return error.Boom;
    }
    return v * 2;
}

fn propagateErr(should_fail: bool, v: i32): !i32 {
    const r = mayFailVal(should_fail, v)?;
    return r + 1;
}

fn main() {
    // 验证顶层可选函数指针类型别名（Zig 严格校验 @as 目标类型必为 optional）
    const a = @as(FnA, &addOneC);
    const b = @as(OptFnB, null);
    assert(a != null, "top-level FnA should be non-null");
    assert(b == null, "top-level OptFnB should allow null");
    assert(a.?(41) == 42, "top-level FnA call should return 42");

    // 验证顶层可选指针类型别名
    let byte: u8 = 77;
    let p: OptPtr = &byte;
    const p_null = @as(OptPtr, null);
    assert(p != null && p.?.* == 77, "top-level OptPtr should deref to 77");
    assert(p_null == null, "top-level OptPtr should allow null");

    // 验证 struct 容器内可选类型别名
    const ca = @as(ContainerTypes.FnA, &addOneC);
    const cb = @as(ContainerTypes.OptFnB, &addOneC);
    const cb_null = @as(ContainerTypes.OptFnB, null);
    const cp = @as(ContainerTypes.OptPtr, &byte);
    const cp_null = @as(ContainerTypes.OptPtr, null);
    assert(ca.?(9) == 10, "ContainerTypes.FnA call should return 10");
    assert(cb.?(19) == 20, "ContainerTypes.OptFnB call should return 20");
    assert(cb_null == null, "ContainerTypes.OptFnB should allow null");
    assert(cp.?.* == 77, "ContainerTypes.OptPtr should deref to 77");
    assert(cp_null == null, "ContainerTypes.OptPtr should allow null");

    // 验证函数体内 expr? 错误传播零回归
    const ok_res = propagateErr(false, 10) catch -1;
    assert(ok_res == 21, "expr? ok path should return 21");

    const err_res = propagateErr(true, 10) catch -99;
    assert(err_res == -99, "expr? err path should propagate error");
}
