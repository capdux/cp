// Source: cp/lib/cel/eval.cp (CEL Sandbox Evaluator & Builtins)
const std = @import("std");
import { CelError, CelMapEntry, CelValue, CelEnv } from "./types.cp";
import { CelUnaryOp, CelBinaryOp, CelMacroKind, CelExpr } from "./parser.cp";

pub struct CelEvalOptions {
    max_steps: usize,

    pub fn defaults(): CelEvalOptions {
        return CelEvalOptions{ .max_steps = 10000 };
    }
}

struct ScopeBinding {
    name: []const u8,
    val: *const CelValue,
}

struct EvalCtx {
    alloc: std.mem.Allocator,
    env: *const CelEnv,
    locals: std.ArrayList(ScopeBinding),
    steps: usize,
    max_steps: usize,

    fn step(*_: *EvalCtx): CelError!void {
        _.steps += 1;
        if (_.steps > _.max_steps) {
            return error.CostLimitExceeded;
        }
    }

    fn lookup(_: *const EvalCtx, name: []const u8): (*const CelValue)? {
        // Search locals from top of stack downwards
        let i: usize = _.locals.items.len;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.locals.items[i].name, name)) {
                return _.locals.items[i].val;
            }
        }
        return _.env.get(name);
    }
}

pub fn evalCel(alloc: std.mem.Allocator, expr: *const CelExpr, env: *const CelEnv): CelError!CelValue {
    return evalCelWithOptions(alloc, expr, env, CelEvalOptions.defaults());
}

pub fn evalCelWithOptions(alloc: std.mem.Allocator, expr: *const CelExpr, env: *const CelEnv, opts: CelEvalOptions): CelError!CelValue {
    let ctx = EvalCtx{
        .alloc = alloc,
        .env = env,
        .locals = std.ArrayList(ScopeBinding).empty,
        .steps = 0,
        .max_steps = opts.max_steps,
    };
    defer ctx.locals.deinit(alloc);
    return evalNode(&ctx, expr);
}

pub fn evalBool(alloc: std.mem.Allocator, expr: *const CelExpr, env: *const CelEnv): CelError!bool {
    return evalBoolWithOptions(alloc, expr, env, CelEvalOptions.defaults());
}

pub fn evalBoolWithOptions(alloc: std.mem.Allocator, expr: *const CelExpr, env: *const CelEnv, opts: CelEvalOptions): CelError!bool {
    let val = try evalCelWithOptions(alloc, expr, env, opts);
    defer val.deinit(alloc);
    return switch (val) {
        .bool_val => |b| b,
        else => error.TypeMismatch,
    };
}

fn evalNode(ctx: *EvalCtx, expr: *const CelExpr): CelError!CelValue {
    try ctx.step();
    switch (expr.*) {
        .null_lit => return .null_val,
        .bool_lit => |b| return CelValue{ .bool_val = b },
        .int_lit => |i| return CelValue{ .int_val = i },
        .uint_lit => |u| return CelValue{ .uint_val = u },
        .double_lit => |d| return CelValue{ .double_val = d },
        .string_lit => |s| {
            const dup = ctx.alloc.dupe(u8, s) catch return error.OutOfMemory;
            return CelValue{ .string_val = dup };
        },
        .ident => |name| {
            if (v := ctx.lookup(name)) {
                return v.clone(ctx.alloc);
            }
            return error.UndeclaredIdentifier;
        },
        .list_lit => |items| {
            let out = std.ArrayList(CelValue).empty;
            errdefer {
                for (*it of out.items) {
                    it.deinit(ctx.alloc);
                }
                out.deinit(ctx.alloc);
            }
            for (item_expr of items) {
                let v = try evalNode(ctx, item_expr);
                out.append(ctx.alloc, v) catch {
                    v.deinit(ctx.alloc);
                    return error.OutOfMemory;
                };
            }
            const slice = out.toOwnedSlice(ctx.alloc) catch return error.OutOfMemory;
            return CelValue{ .list_val = slice };
        },
        .map_lit => |entries| {
            let out = std.ArrayList(CelMapEntry).empty;
            errdefer {
                for (*e of out.items) {
                    ctx.alloc.free(e.key);
                    e.value.deinit(ctx.alloc);
                }
                out.deinit(ctx.alloc);
            }
            for (entry_ast of entries) {
                let kval = try evalNode(ctx, entry_ast.key);
                defer kval.deinit(ctx.alloc);
                const kstr = switch (kval) {
                    .string_val => |s| s,
                    else => return error.TypeMismatch,
                };
                const kdup = ctx.alloc.dupe(u8, kstr) catch return error.OutOfMemory;
                errdefer ctx.alloc.free(kdup);
                let vval = try evalNode(ctx, entry_ast.value);
                out.append(ctx.alloc, CelMapEntry{ .key = kdup, .value = vval }) catch {
                    vval.deinit(ctx.alloc);
                    return error.OutOfMemory;
                };
            }
            const slice = out.toOwnedSlice(ctx.alloc) catch return error.OutOfMemory;
            return CelValue{ .map_val = slice };
        },
        .unary => |u| {
            let v = try evalNode(ctx, u.operand);
            defer v.deinit(ctx.alloc);
            return switch (u.op) {
                .not_op => switch (v) {
                    .bool_val => |b| CelValue{ .bool_val = !b },
                    else => error.TypeMismatch,
                },
                .neg_op => switch (v) {
                    .int_val => |i| blk: {
                        const res = @subWithOverflow(@as(i64, 0), i);
                        if (res[1] != 0) {
                            return error.IntegerOverflow;
                        }
                        break :blk CelValue{ .int_val = res[0] };
                    },
                    .double_val => |d| CelValue{ .double_val = -d },
                    else => error.TypeMismatch,
                },
            };
        },
        .binary => |b| {
            // Short-circuit logical operators
            if (b.op == .and_op) {
                let lv = try evalNode(ctx, b.left);
                defer lv.deinit(ctx.alloc);
                const lb = switch (lv) {
                    .bool_val => |v| v,
                    else => return error.TypeMismatch,
                };
                if (!lb) {
                    return CelValue{ .bool_val = false };
                }
                let rv = try evalNode(ctx, b.right);
                defer rv.deinit(ctx.alloc);
                const rb = switch (rv) {
                    .bool_val => |v| v,
                    else => return error.TypeMismatch,
                };
                return CelValue{ .bool_val = rb };
            }
            if (b.op == .or_op) {
                let lv = try evalNode(ctx, b.left);
                defer lv.deinit(ctx.alloc);
                const lb = switch (lv) {
                    .bool_val => |v| v,
                    else => return error.TypeMismatch,
                };
                if (lb) {
                    return CelValue{ .bool_val = true };
                }
                let rv = try evalNode(ctx, b.right);
                defer rv.deinit(ctx.alloc);
                const rb = switch (rv) {
                    .bool_val => |v| v,
                    else => return error.TypeMismatch,
                };
                return CelValue{ .bool_val = rb };
            }

            let lv = try evalNode(ctx, b.left);
            defer lv.deinit(ctx.alloc);
            let rv = try evalNode(ctx, b.right);
            defer rv.deinit(ctx.alloc);
            return evalBinary(ctx.alloc, b.op, &lv, &rv);
        },
        .ternary => |t| {
            let cv = try evalNode(ctx, t.cond);
            defer cv.deinit(ctx.alloc);
            const cond_b = switch (cv) {
                .bool_val => |b| b,
                else => return error.TypeMismatch,
            };
            if (cond_b) {
                return evalNode(ctx, t.true_expr);
            }
            return evalNode(ctx, t.false_expr);
        },
        .member => |m| {
            let tgt = try evalNode(ctx, m.target);
            defer tgt.deinit(ctx.alloc);
            switch (tgt) {
                .map_val => {
                    if (found := tgt.mapGet(m.field)) {
                        return found.clone(ctx.alloc);
                    }
                    return error.KeyNotFound;
                },
                else => return error.TypeMismatch,
            }
        },
        .index => |ix| {
            let tgt = try evalNode(ctx, ix.target);
            defer tgt.deinit(ctx.alloc);
            let key = try evalNode(ctx, ix.idx);
            defer key.deinit(ctx.alloc);
            switch (tgt) {
                .list_val => |items| {
                    const idx_val: usize = switch (key) {
                        .int_val => |i| blk: {
                            if (i < 0) {
                                return error.IndexOutOfBounds;
                            }
                            break :blk @as(usize, @intCast(i));
                        },
                        .uint_val => |u| @as(usize, @intCast(u)),
                        else => return error.TypeMismatch,
                    };
                    if (idx_val >= items.len) {
                        return error.IndexOutOfBounds;
                    }
                    return items[idx_val].clone(ctx.alloc);
                },
                .map_val => {
                    const kstr = switch (key) {
                        .string_val => |s| s,
                        else => return error.TypeMismatch,
                    };
                    if (found := tgt.mapGet(kstr)) {
                        return found.clone(ctx.alloc);
                    }
                    return error.KeyNotFound;
                },
                else => return error.TypeMismatch,
            }
        },
        .has_field => |h| {
            const exists = evalHasField(ctx, h.target, h.field) catch |e| {
                return switch (e) {
                    error.KeyNotFound, error.UndeclaredIdentifier, error.TypeMismatch => CelValue{ .bool_val = false },
                    else => e,
                };
            };
            return CelValue{ .bool_val = exists };
        },
        .call => |c| {
            return evalCall(ctx, c.target, c.name, c.args);
        },
        .macro_call => |mc| {
            return evalMacro(ctx, mc.kind, mc.target, mc.iter_var, mc.body);
        },
    }
}

fn evalHasField(ctx: *EvalCtx, target: *const CelExpr, field: []const u8): CelError!bool {
    let tgt = try evalNode(ctx, target);
    defer tgt.deinit(ctx.alloc);
    return switch (tgt) {
        .map_val => tgt.mapGet(field) != null,
        else => false,
    };
}

fn evalBinary(alloc: std.mem.Allocator, op: CelBinaryOp, lv: *const CelValue, rv: *const CelValue): CelError!CelValue {
    switch (op) {
        .eq_op => return CelValue{ .bool_val = lv.equals(rv) },
        .ne_op => return CelValue{ .bool_val = !lv.equals(rv) },
        .lt_op, .le_op, .gt_op, .ge_op => {
            const cmp = try compareValues(lv, rv);
            const res = switch (op) {
                .lt_op => cmp < 0,
                .le_op => cmp <= 0,
                .gt_op => cmp > 0,
                .ge_op => cmp >= 0,
                else => false,
            };
            return CelValue{ .bool_val = res };
        },
        .in_op => {
            switch (rv.*) {
                .list_val => |items| {
                    for (it of items) {
                        if (lv.equals(it)) {
                            return CelValue{ .bool_val = true };
                        }
                    }
                    return CelValue{ .bool_val = false };
                },
                .map_val => {
                    const kstr = switch (lv.*) {
                        .string_val => |s| s,
                        else => return error.TypeMismatch,
                    };
                    return CelValue{ .bool_val = rv.mapGet(kstr) != null };
                },
                else => return error.TypeMismatch,
            }
        },
        .add_op => {
            switch (lv.*) {
                .int_val => |a| {
                    return switch (rv.*) {
                        .int_val => |b| blk: {
                            const r = @addWithOverflow(a, b);
                            if (r[1] != 0) {
                                return error.IntegerOverflow;
                            }
                            break :blk CelValue{ .int_val = r[0] };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .uint_val => |a| {
                    return switch (rv.*) {
                        .uint_val => |b| blk: {
                            const r = @addWithOverflow(a, b);
                            if (r[1] != 0) {
                                return error.IntegerOverflow;
                            }
                            break :blk CelValue{ .uint_val = r[0] };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .double_val => |a| {
                    return switch (rv.*) {
                        .double_val => |b| CelValue{ .double_val = a + b },
                        else => error.TypeMismatch,
                    };
                },
                .string_val => |a| {
                    return switch (rv.*) {
                        .string_val => |b| blk: {
                            let joined = alloc.alloc(u8, a.len + b.len) catch return error.OutOfMemory;
                            @memcpy(joined[0..a.len], a);
                            @memcpy(joined[a.len..], b);
                            break :blk CelValue{ .string_val = joined };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .list_val => |a| {
                    return switch (rv.*) {
                        .list_val => |b| blk: {
                            let out = std.ArrayList(CelValue).empty;
                            errdefer {
                                for (*it of out.items) {
                                    it.deinit(alloc);
                                }
                                out.deinit(alloc);
                            }
                            for (it of a) {
                                const c = try it.clone(alloc);
                                out.append(alloc, c) catch return error.OutOfMemory;
                            }
                            for (it of b) {
                                const c = try it.clone(alloc);
                                out.append(alloc, c) catch return error.OutOfMemory;
                            }
                            const slice = out.toOwnedSlice(alloc) catch return error.OutOfMemory;
                            break :blk CelValue{ .list_val = slice };
                        },
                        else => error.TypeMismatch,
                    };
                },
                else => return error.TypeMismatch,
            }
        },
        .sub_op => {
            switch (lv.*) {
                .int_val => |a| {
                    return switch (rv.*) {
                        .int_val => |b| blk: {
                            const r = @subWithOverflow(a, b);
                            if (r[1] != 0) {
                                return error.IntegerOverflow;
                            }
                            break :blk CelValue{ .int_val = r[0] };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .uint_val => |a| {
                    return switch (rv.*) {
                        .uint_val => |b| blk: {
                            const r = @subWithOverflow(a, b);
                            if (r[1] != 0) {
                                return error.IntegerOverflow;
                            }
                            break :blk CelValue{ .uint_val = r[0] };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .double_val => |a| {
                    return switch (rv.*) {
                        .double_val => |b| CelValue{ .double_val = a - b },
                        else => error.TypeMismatch,
                    };
                },
                else => return error.TypeMismatch,
            }
        },
        .mul_op => {
            switch (lv.*) {
                .int_val => |a| {
                    return switch (rv.*) {
                        .int_val => |b| blk: {
                            const r = @mulWithOverflow(a, b);
                            if (r[1] != 0) {
                                return error.IntegerOverflow;
                            }
                            break :blk CelValue{ .int_val = r[0] };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .uint_val => |a| {
                    return switch (rv.*) {
                        .uint_val => |b| blk: {
                            const r = @mulWithOverflow(a, b);
                            if (r[1] != 0) {
                                return error.IntegerOverflow;
                            }
                            break :blk CelValue{ .uint_val = r[0] };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .double_val => |a| {
                    return switch (rv.*) {
                        .double_val => |b| CelValue{ .double_val = a * b },
                        else => error.TypeMismatch,
                    };
                },
                else => return error.TypeMismatch,
            }
        },
        .div_op => {
            switch (lv.*) {
                .int_val => |a| {
                    return switch (rv.*) {
                        .int_val => |b| blk: {
                            if (b == 0) {
                                return error.DivisionByZero;
                            }
                            if (a == std.math.minInt(i64) and b == -1) {
                                return error.IntegerOverflow;
                            }
                            break :blk CelValue{ .int_val = @divTrunc(a, b) };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .uint_val => |a| {
                    return switch (rv.*) {
                        .uint_val => |b| blk: {
                            if (b == 0) {
                                return error.DivisionByZero;
                            }
                            break :blk CelValue{ .uint_val = a / b };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .double_val => |a| {
                    return switch (rv.*) {
                        .double_val => |b| blk: {
                            if (b == 0.0) {
                                return error.DivisionByZero;
                            }
                            break :blk CelValue{ .double_val = a / b };
                        },
                        else => error.TypeMismatch,
                    };
                },
                else => return error.TypeMismatch,
            }
        },
        .mod_op => {
            switch (lv.*) {
                .int_val => |a| {
                    return switch (rv.*) {
                        .int_val => |b| blk: {
                            if (b == 0) {
                                return error.DivisionByZero;
                            }
                            if (a == std.math.minInt(i64) and b == -1) {
                                return error.IntegerOverflow;
                            }
                            break :blk CelValue{ .int_val = @rem(a, b) };
                        },
                        else => error.TypeMismatch,
                    };
                },
                .uint_val => |a| {
                    return switch (rv.*) {
                        .uint_val => |b| blk: {
                            if (b == 0) {
                                return error.DivisionByZero;
                            }
                            break :blk CelValue{ .uint_val = a % b };
                        },
                        else => error.TypeMismatch,
                    };
                },
                else => return error.TypeMismatch,
            }
        },
        else => return error.TypeMismatch,
    }
}

fn compareValues(lv: *const CelValue, rv: *const CelValue): CelError!i32 {
    switch (lv.*) {
        .int_val => |a| {
            return switch (rv.*) {
                .int_val => |b| if (a < b) -1 else if (a > b) 1 else 0,
                else => error.TypeMismatch,
            };
        },
        .uint_val => |a| {
            return switch (rv.*) {
                .uint_val => |b| if (a < b) -1 else if (a > b) 1 else 0,
                else => error.TypeMismatch,
            };
        },
        .double_val => |a| {
            return switch (rv.*) {
                .double_val => |b| if (a < b) -1 else if (a > b) 1 else 0,
                else => error.TypeMismatch,
            };
        },
        .string_val => |a| {
            return switch (rv.*) {
                .string_val => |b| blk: {
                    const ord = std.mem.order(u8, a, b);
                    break :blk switch (ord) {
                        .lt => -1,
                        .gt => 1,
                        .eq => 0,
                    };
                },
                else => error.TypeMismatch,
            };
        },
        else => return error.TypeMismatch,
    }
}

fn evalCall(ctx: *EvalCtx, target_opt: (*const CelExpr)?, name: []const u8, args: []const *CelExpr): CelError!CelValue {
    let tgt_opt: CelValue? = null;
    defer {
        if (*tv := tgt_opt) {
            tv.deinit(ctx.alloc);
        }
    }
    if (te := target_opt) {
        tgt_opt = try evalNode(ctx, te);
    }

    let eval_args = std.ArrayList(CelValue).empty;
    defer {
        for (*av of eval_args.items) {
            av.deinit(ctx.alloc);
        }
        eval_args.deinit(ctx.alloc);
    }
    for (arg_expr of args) {
        let av = try evalNode(ctx, arg_expr);
        eval_args.append(ctx.alloc, av) catch {
            av.deinit(ctx.alloc);
            return error.OutOfMemory;
        };
    }

    if (std.mem.eql(u8, name, "size")) {
        let subject: *const CelValue = undefined;
        if (tgt_opt != null) {
            if (eval_args.items.len != 0) {
                return error.InvalidArgument;
            }
            subject = &(tgt_opt.?);
        } else {
            if (eval_args.items.len != 1) {
                return error.InvalidArgument;
            }
            subject = &eval_args.items[0];
        }
        const len: usize = switch (subject.*) {
            .string_val => |s| s.len,
            .list_val => |l| l.len,
            .map_val => |m| m.len,
            else => return error.TypeMismatch,
        };
        return CelValue{ .int_val = @as(i64, @intCast(len)) };
    }

    if (std.mem.eql(u8, name, "contains") or std.mem.eql(u8, name, "startsWith") or std.mem.eql(u8, name, "endsWith") or std.mem.eql(u8, name, "matches")) {
        const pair = try resolveBinaryMethodArgs(tgt_opt, eval_args.items);
        const s = switch (pair.recv.*) {
            .string_val => |sv| sv,
            else => return error.TypeMismatch,
        };
        const pat = switch (pair.arg.*) {
            .string_val => |pv| pv,
            else => return error.TypeMismatch,
        };
        if (std.mem.eql(u8, name, "contains")) {
            return CelValue{ .bool_val = std.mem.indexOf(u8, s, pat) != null };
        }
        if (std.mem.eql(u8, name, "startsWith")) {
            return CelValue{ .bool_val = std.mem.startsWith(u8, s, pat) };
        }
        if (std.mem.eql(u8, name, "endsWith")) {
            return CelValue{ .bool_val = std.mem.endsWith(u8, s, pat) };
        }
        return CelValue{ .bool_val = try regexMatch(pat, s) };
    }

    if (tgt_opt == null and eval_args.items.len == 1) {
        const v = &eval_args.items[0];
        if (std.mem.eql(u8, name, "int")) {
            return switch (v.*) {
                .int_val => |i| CelValue{ .int_val = i },
                .uint_val => |u| blk: {
                    if (u > @as(u64, @intCast(std.math.maxInt(i64)))) {
                        return error.IntegerOverflow;
                    }
                    break :blk CelValue{ .int_val = @as(i64, @intCast(u)) };
                },
                .double_val => |d| CelValue{ .int_val = @as(i64, @intFromFloat(d)) },
                .string_val => |s| blk: {
                    const parsed = std.fmt.parseInt(i64, s, 10) catch return error.InvalidArgument;
                    break :blk CelValue{ .int_val = parsed };
                },
                else => error.TypeMismatch,
            };
        }
        if (std.mem.eql(u8, name, "uint")) {
            return switch (v.*) {
                .uint_val => |u| CelValue{ .uint_val = u },
                .int_val => |i| blk: {
                    if (i < 0) {
                        return error.IntegerOverflow;
                    }
                    break :blk CelValue{ .uint_val = @as(u64, @intCast(i)) };
                },
                .string_val => |s| blk: {
                    const parsed = std.fmt.parseInt(u64, s, 10) catch return error.InvalidArgument;
                    break :blk CelValue{ .uint_val = parsed };
                },
                else => error.TypeMismatch,
            };
        }
        if (std.mem.eql(u8, name, "double")) {
            return switch (v.*) {
                .double_val => |d| CelValue{ .double_val = d },
                .int_val => |i| CelValue{ .double_val = @as(f64, @floatFromInt(i)) },
                .uint_val => |u| CelValue{ .double_val = @as(f64, @floatFromInt(u)) },
                .string_val => |s| blk: {
                    const parsed = std.fmt.parseFloat(f64, s) catch return error.InvalidArgument;
                    break :blk CelValue{ .double_val = parsed };
                },
                else => error.TypeMismatch,
            };
        }
        if (std.mem.eql(u8, name, "string")) {
            const s = switch (v.*) {
                .string_val => |sv| ctx.alloc.dupe(u8, sv) catch return error.OutOfMemory,
                .int_val => |i| std.fmt.allocPrint(ctx.alloc, "{d}", .{i}) catch return error.OutOfMemory,
                .uint_val => |u| std.fmt.allocPrint(ctx.alloc, "{d}", .{u}) catch return error.OutOfMemory,
                .double_val => |d| std.fmt.allocPrint(ctx.alloc, "{d}", .{d}) catch return error.OutOfMemory,
                .bool_val => |b| ctx.alloc.dupe(u8, if (b) "true" else "false") catch return error.OutOfMemory,
                else => return error.TypeMismatch,
            };
            return CelValue{ .string_val = s };
        }
        if (std.mem.eql(u8, name, "bool")) {
            return switch (v.*) {
                .bool_val => |b| CelValue{ .bool_val = b },
                .string_val => |s| blk: {
                    if (std.mem.eql(u8, s, "true")) {
                        break :blk CelValue{ .bool_val = true };
                    }
                    if (std.mem.eql(u8, s, "false")) {
                        break :blk CelValue{ .bool_val = false };
                    }
                    return error.InvalidArgument;
                },
                else => error.TypeMismatch,
            };
        }
    }

    return error.UnknownFunction;
}

struct BinaryMethodPair {
    recv: *const CelValue,
    arg: *const CelValue,
}

fn resolveBinaryMethodArgs(tgt_opt: CelValue?, args: []const CelValue): CelError!BinaryMethodPair {
    if (*tv := tgt_opt) {
        if (args.len != 1) {
            return error.InvalidArgument;
        }
        return BinaryMethodPair{ .recv = tv, .arg = &args[0] };
    }
    if (args.len != 2) {
        return error.InvalidArgument;
    }
    return BinaryMethodPair{ .recv = &args[0], .arg = &args[1] };
}

fn evalMacro(ctx: *EvalCtx, kind: CelMacroKind, target_expr: *const CelExpr, iter_var: []const u8, body: *const CelExpr): CelError!CelValue {
    let tgt = try evalNode(ctx, target_expr);
    defer tgt.deinit(ctx.alloc);

    const items: []const CelValue = switch (tgt) {
        .list_val => |l| l,
        else => return error.TypeMismatch,
    };

    switch (kind) {
        .all => {
            let mi: usize = 0;
            while (mi < items.len) {
                defer mi += 1;
                try ctx.step();
                ctx.locals.append(ctx.alloc, ScopeBinding{ .name = iter_var, .val = &items[mi] }) catch return error.OutOfMemory;
                defer * = ctx.locals.pop();
                let res = try evalNode(ctx, body);
                defer res.deinit(ctx.alloc);
                const b = switch (res) {
                    .bool_val => |bv| bv,
                    else => return error.TypeMismatch,
                };
                if (!b) {
                    return CelValue{ .bool_val = false };
                }
            }
            return CelValue{ .bool_val = true };
        },
        .exists => {
            let mi: usize = 0;
            while (mi < items.len) {
                defer mi += 1;
                try ctx.step();
                ctx.locals.append(ctx.alloc, ScopeBinding{ .name = iter_var, .val = &items[mi] }) catch return error.OutOfMemory;
                defer * = ctx.locals.pop();
                let res = try evalNode(ctx, body);
                defer res.deinit(ctx.alloc);
                const b = switch (res) {
                    .bool_val => |bv| bv,
                    else => return error.TypeMismatch,
                };
                if (b) {
                    return CelValue{ .bool_val = true };
                }
            }
            return CelValue{ .bool_val = false };
        },
        .exists_one => {
            let count: usize = 0;
            let mi: usize = 0;
            while (mi < items.len) {
                defer mi += 1;
                try ctx.step();
                ctx.locals.append(ctx.alloc, ScopeBinding{ .name = iter_var, .val = &items[mi] }) catch return error.OutOfMemory;
                defer * = ctx.locals.pop();
                let res = try evalNode(ctx, body);
                defer res.deinit(ctx.alloc);
                const b = switch (res) {
                    .bool_val => |bv| bv,
                    else => return error.TypeMismatch,
                };
                if (b) {
                    count += 1;
                    if (count > 1) {
                        return CelValue{ .bool_val = false };
                    }
                }
            }
            return CelValue{ .bool_val = count == 1 };
        },
        .filter => {
            let out = std.ArrayList(CelValue).empty;
            errdefer {
                for (*it of out.items) {
                    it.deinit(ctx.alloc);
                }
                out.deinit(ctx.alloc);
            }
            let mi: usize = 0;
            while (mi < items.len) {
                defer mi += 1;
                try ctx.step();
                ctx.locals.append(ctx.alloc, ScopeBinding{ .name = iter_var, .val = &items[mi] }) catch return error.OutOfMemory;
                defer * = ctx.locals.pop();
                let res = try evalNode(ctx, body);
                defer res.deinit(ctx.alloc);
                const b = switch (res) {
                    .bool_val => |bv| bv,
                    else => return error.TypeMismatch,
                };
                if (b) {
                    let cloned = try items[mi].clone(ctx.alloc);
                    out.append(ctx.alloc, cloned) catch {
                        cloned.deinit(ctx.alloc);
                        return error.OutOfMemory;
                    };
                }
            }
            const slice = out.toOwnedSlice(ctx.alloc) catch return error.OutOfMemory;
            return CelValue{ .list_val = slice };
        },
        .map => {
            let out = std.ArrayList(CelValue).empty;
            errdefer {
                for (*it of out.items) {
                    it.deinit(ctx.alloc);
                }
                out.deinit(ctx.alloc);
            }
            let mi: usize = 0;
            while (mi < items.len) {
                defer mi += 1;
                try ctx.step();
                ctx.locals.append(ctx.alloc, ScopeBinding{ .name = iter_var, .val = &items[mi] }) catch return error.OutOfMemory;
                defer * = ctx.locals.pop();
                let mapped = try evalNode(ctx, body);
                out.append(ctx.alloc, mapped) catch {
                    mapped.deinit(ctx.alloc);
                    return error.OutOfMemory;
                };
            }
            const slice = out.toOwnedSlice(ctx.alloc) catch return error.OutOfMemory;
            return CelValue{ .list_val = slice };
        },
    }
}

// Lightweight regex matcher supporting ^, $, ., *, +, ?, [...], and \ escapes
fn regexMatch(pattern: []const u8, text: []const u8): CelError!bool {
    if (pattern.len > 0 and pattern[0] == '^') {
        return regexMatchHere(pattern[1..], text);
    }
    let i: usize = 0;
    while (i <= text.len) {
        if (try regexMatchHere(pattern, text[i..])) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn regexMatchHere(pat: []const u8, text: []const u8): CelError!bool {
    if (pat.len == 0) {
        return true;
    }
    if (pat.len == 1 and pat[0] == '$') {
        return text.len == 0;
    }
    const atom_len = try regexAtomLen(pat);
    const rest = pat[atom_len..];
    if (rest.len > 0 and (rest[0] == '*' or rest[0] == '+' or rest[0] == '?')) {
        const quant = rest[0];
        const after_quant = rest[1..];
        if (quant == '?') {
            if (text.len > 0 and regexMatchAtom(pat[0..atom_len], text[0])) {
                if (try regexMatchHere(after_quant, text[1..])) {
                    return true;
                }
            }
            return regexMatchHere(after_quant, text);
        }
        let min_count: usize = if (quant == '+') 1 else 0;
        let matched: usize = 0;
        while (matched < text.len and regexMatchAtom(pat[0..atom_len], text[matched])) {
            matched += 1;
        }
        if (matched < min_count) {
            return false;
        }
        let k: usize = matched;
        while (true) {
            if (try regexMatchHere(after_quant, text[k..])) {
                return true;
            }
            if (k == min_count) {
                break;
            }
            k -= 1;
        }
        return false;
    }
    if (text.len > 0 and regexMatchAtom(pat[0..atom_len], text[0])) {
        return regexMatchHere(rest, text[1..]);
    }
    return false;
}

fn regexAtomLen(pat: []const u8): CelError!usize {
    if (pat[0] == '\\') {
        if (pat.len < 2) {
            return error.InvalidArgument;
        }
        return 2;
    }
    if (pat[0] == '[') {
        let i: usize = 1;
        if (i < pat.len and pat[i] == '^') {
            i += 1;
        }
        while (i < pat.len) {
            if (pat[i] == '\\') {
                i += 2;
            } else if (pat[i] == ']') {
                return i + 1;
            } else {
                i += 1;
            }
        }
        return error.InvalidArgument;
    }
    return 1;
}

fn regexMatchAtom(atom: []const u8, ch: u8): bool {
    if (atom.len == 1 and atom[0] == '.') {
        return ch != '\n';
    }
    if (atom[0] == '\\' and atom.len == 2) {
        return switch (atom[1]) {
            'd' => ch >= '0' and ch <= '9',
            'w' => (ch >= 'a' and ch <= 'z') or (ch >= 'A' and ch <= 'Z') or (ch >= '0' and ch <= '9') or ch == '_',
            's' => ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r',
            else => ch == atom[1],
        };
    }
    if (atom[0] == '[' and atom.len >= 2) {
        let neg = false;
        let idx: usize = 1;
        if (idx < atom.len - 1 and atom[idx] == '^') {
            neg = true;
            idx += 1;
        }
        let matched = false;
        while (idx < atom.len - 1) {
            if (idx + 2 < atom.len - 1 and atom[idx + 1] == '-') {
                if (ch >= atom[idx] and ch <= atom[idx + 2]) {
                    matched = true;
                }
                idx += 3;
            } else {
                if (ch == atom[idx]) {
                    matched = true;
                }
                idx += 1;
            }
        }
        return if (neg) !matched else matched;
    }
    return ch == atom[0];
}
