// Source: cp/lib/tui/layout.cp (Flex Rect Layout & Margin Solver)
// Pure CP 2D rectangle geometry and deterministic flex layout constraint solver.

const std = @import("std");

/// Error taxonomy for `cap:tui` layout and rendering operations.
pub err TuiError {
    OutOfMemory,
    InvalidDimensions,
    InvalidUtf8,
}

/// 2D terminal cell rectangle (`x`, `y` origin, `w` width, `h` height).
pub struct Rect {
    x: u16 = 0;
    y: u16 = 0;
    w: u16 = 0;
    h: u16 = 0;

    /// Constructs a `Rect` from origin `(x, y)` and dimensions `(w, h)`.
    static fn init(x: u16, y: u16, w: u16, h: u16): Rect {
        return Rect{ x: x, y: y, w: w, h: h };
    }

    /// Returns total cell area `w * h` without overflow.
    fn area(): u32 {
        return u32(_.w) * u32(_.h);
    }

    /// Returns `true` when either width or height is zero.
    fn isEmpty(): bool {
        return _.w == 0 || _.h == 0;
    }

    /// Returns an inset rectangle shrunk by `pad_x` on left/right and `pad_y` on top/bottom.
    /// Clamps safely to zero dimensions without integer underflow.
    fn inner(pad_x: u16, pad_y: u16): Rect {
        const dx = u32(pad_x) * 2;
        const dy = u32(pad_y) * 2;

        const new_w: u16 = if (dx >= u32(_.w)) 0 else (_.w - u16(dx));
        const new_h: u16 = if (dy >= u32(_.h)) 0 else (_.h - u16(dy));
        const sum_x = u32(_.x) + u32(pad_x);
        const sum_y = u32(_.y) + u32(pad_y);
        const new_x: u16 = if (new_w == 0) _.x else (if (sum_x > 65535) 65535 else u16(sum_x));
        const new_y: u16 = if (new_h == 0) _.y else (if (sum_y > 65535) 65535 else u16(sum_y));

        return Rect{ x: new_x, y: new_y, w: new_w, h: new_h };
    }

    /// Returns `true` if cell coordinate `(px, py)` lies inside `[x, x+w) x [y, y+h)`.
    fn contains(px: u16, py: u16): bool {
        if (_.w == 0 || _.h == 0) return false;
        const rx = u32(_.x) + u32(_.w);
        const by = u32(_.y) + u32(_.h);
        return u32(px) >= u32(_.x) && u32(px) < rx && u32(py) >= u32(_.y) && u32(py) < by;
    }

    /// Returns `true` if all four coordinates and dimensions match `other`.
    fn eql(other: Rect): bool {
        return _.x == other.x && _.y == other.y && _.w == other.w && _.h == other.h;
    }
}

/// Flex layout sizing constraint along the split axis.
pub enum Constraint {
    Fixed: u16,
    Percent: u8,
    Min: u16,
    Fill: u16,

    /// Constructs a `Constraint.Fixed` slot.
    static fn fixed(v: u16): Constraint {
        return Constraint.Fixed(v);
    }

    /// Constructs a `Constraint.Percent` slot.
    static fn percent(p: u8): Constraint {
        return Constraint.Percent(p);
    }

    /// Constructs a `Constraint.Min` slot.
    static fn min(m: u16): Constraint {
        return Constraint.Min(m);
    }

    /// Constructs a `Constraint.Fill` slot.
    static fn fill(w: u16): Constraint {
        return Constraint.Fill(w);
    }
}

fn solveSizes(alloc: std.mem.Allocator, total: u16, constraints: []const Constraint): ([]u16) ! TuiError {
    let sizes = alloc.alloc(u16, constraints.len) catch return TuiError.OutOfMemory;
    if (constraints.len == 0) return sizes;

    let remaining: u32 = u32(total);
    let fill_weight_sum: u32 = 0;
    let fill_count: usize = 0;

    // Pass 1: allocate Fixed, Percent, and Min slots without overflowing parent container
    let i: usize = 0;
    while (i < constraints.len) : (i += 1) {
        sizes[i] = 0;
        switch (constraints[i]) {
            .Fixed v => {
                const take: u32 = if (u32(v) <= remaining) u32(v) else remaining;
                sizes[i] = u16(take);
                remaining -= take;
            },
            .Percent p => {
                const pct: u32 = if (p > 100) 100 else u32(p);
                const want: u32 = (u32(total) * pct) / 100;
                const take: u32 = if (want <= remaining) want else remaining;
                sizes[i] = u16(take);
                remaining -= take;
            },
            .Min m => {
                const take: u32 = if (u32(m) <= remaining) u32(m) else remaining;
                sizes[i] = u16(take);
                remaining -= take;
            },
            .Fill weight => {
                fill_count += 1;
                const eff_w: u32 = if (weight == 0) 1 else u32(weight);
                fill_weight_sum += eff_w;
            },
        }
    }

    // Pass 2: distribute remaining space proportionally across Fill slots and spread remainder fairly
    if (fill_count > 0 && remaining > 0) {
        const pool = remaining;
        let distributed: u32 = 0;

        let j: usize = 0;
        while (j < constraints.len) : (j += 1) {
            switch (constraints[j]) {
                .Fill weight => {
                    const eff_w: u32 = if (weight == 0) 1 else u32(weight);
                    const share = (pool * eff_w) / fill_weight_sum;
                    sizes[j] += u16(share);
                    distributed += share;
                },
                else => {},
            }
        }

        let leftover = pool - distributed;
        let k: usize = 0;
        while (k < constraints.len && leftover > 0) : (k += 1) {
            switch (constraints[k]) {
                .Fill => {
                    sizes[k] += 1;
                    leftover -= 1;
                },
                else => {},
            }
        }
    }

    return sizes;
}

/// Deterministic vertical and horizontal flex container splitter.
pub static struct Layout {
    /// Splits `area` vertically (top-to-bottom along `y`/`h`) according to `constraints`.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    static fn splitVertical(
        alloc: std.mem.Allocator,
        area: Rect,
        constraints: []const Constraint,
    ): ([]Rect) ! TuiError {
        let out = alloc.alloc(Rect, constraints.len) catch return TuiError.OutOfMemory;
        errdefer alloc.free(out);

        const sizes = solveSizes(alloc, area.h, constraints)?;
        defer alloc.free(sizes);

        let cur_y: u16 = area.y;
        let i: usize = 0;
        while (i < constraints.len) : (i += 1) {
            out[i] = Rect{
                x: area.x,
                y: cur_y,
                w: area.w,
                h: sizes[i],
            };
            const ny = u32(cur_y) + u32(sizes[i]);
            cur_y = if (ny > 65535) 65535 else u16(ny);
        }
        return out;
    }

    /// Splits `area` horizontally (left-to-right along `x`/`w`) according to `constraints`.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    static fn splitHorizontal(
        alloc: std.mem.Allocator,
        area: Rect,
        constraints: []const Constraint,
    ): ([]Rect) ! TuiError {
        let out = alloc.alloc(Rect, constraints.len) catch return TuiError.OutOfMemory;
        errdefer alloc.free(out);

        const sizes = solveSizes(alloc, area.w, constraints)?;
        defer alloc.free(sizes);

        let cur_x: u16 = area.x;
        let i: usize = 0;
        while (i < constraints.len) : (i += 1) {
            out[i] = Rect{
                x: cur_x,
                y: area.y,
                w: sizes[i],
                h: area.h,
            };
            const nx = u32(cur_x) + u32(sizes[i]);
            cur_x = if (nx > 65535) 65535 else u16(nx);
        }
        return out;
    }
}
