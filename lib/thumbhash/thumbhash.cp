// Source: Absorbed from /data/dev/bun/src/runtime/image/thumbhash.cp | Synced: 2026-11-22T02:33:15Z
//! ThumbHash — compact (~21–25 byte) blurry image placeholder (LQIP) encoder & decoder
//! with approximate aspect ratio and average RGBA extraction.

import std from "std";

export const MAX_LEN: usize = 25;

export err ThumbhashError {
    InvalidDimensions,
    DecodeFailed,
    OutOfMemory,
}

export struct DecodedImage {
    allocator: std.mem.Allocator,
    rgba: []u8,
    width: u32,
    height: u32,

    ~init() {
        if (_.rgba.len > 0) {
            _.allocator.free(_.rgba);
            _.rgba = &[];
        }
    }
}

struct Channel {
    dc: f32 = 0.0,
    scale: f32 = 0.0,
    ac: [49]f32 = [0.0; 49],
    n: usize = 0,
}

inline fn toF32(v: u32): f32 {
    return @floatFromInt(v);
}

inline fn toF32Usize(v: usize): f32 {
    return @floatFromInt(v);
}

inline fn clamp8(v: f32): u8 {
    const c: f32 = if (v < 0.0) 0.0 else if (v > 1.0) 1.0 else v;
    return @intFromFloat(@round(c * 255.0));
}

fn dct(chan: []const f32, w: u32, h: u32, nx: u32, ny: u32): Channel {
    let c = Channel{};
    let fx: [100]f32 = undefined;
    let cy: u32 = 0;
    while (cy < ny) : (cy += 1) {
        let cx: u32 = 0;
        while (cx * ny < nx * (ny - cy)) : (cx += 1) {
            let xi: usize = 0;
            while (xi < @as(usize, w)) : (xi += 1) {
                fx[xi] = @cos(std.math.pi / toF32(w) * toF32(cx) * (toF32Usize(xi) + 0.5));
            }
            let f: f32 = 0.0;
            let yi: usize = 0;
            while (yi < @as(usize, h)) : (yi += 1) {
                const fy = @cos(std.math.pi / toF32(h) * toF32(cy) * (toF32Usize(yi) + 0.5));
                let xk: usize = 0;
                while (xk < @as(usize, w)) : (xk += 1) {
                    f += chan[xk + yi * @as(usize, w)] * fx[xk] * fy;
                }
            }
            f /= toF32(w * h);
            if (cx == 0 && cy == 0) {
                c.dc = f;
            } else {
                c.ac[c.n] = f;
                c.n += 1;
                const af = @abs(f);
                if (af > c.scale) c.scale = af;
            }
        }
    }
    if (c.scale > 0.0) {
        let fi: usize = 0;
        while (fi < c.n) : (fi += 1) {
            c.ac[fi] = 0.5 + 0.5 / c.scale * c.ac[fi];
        }
    }
    return c;
}

/// Encode a `w x h` RGBA8 image (`1 <= w, h <= 100`) into `out` (returns written slice `<= 25` bytes).
export fn encode(out: []u8, w: u32, h: u32, rgba: []const u8): ([]u8)! {
    if (out.len < MAX_LEN) return err.InvalidDimensions;
    if (w == 0 || w > 100 || h == 0 || h > 100) return err.InvalidDimensions;
    if (rgba.len != @as(usize, w) * @as(usize, h) * 4) return err.InvalidDimensions;

    let avg: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    let i: usize = 0;
    while (i < rgba.len) : (i += 4) {
        const a: f32 = toF32(@as(u32, rgba[i + 3])) / 255.0;
        avg[0] += a / 255.0 * toF32(@as(u32, rgba[i + 0]));
        avg[1] += a / 255.0 * toF32(@as(u32, rgba[i + 1]));
        avg[2] += a / 255.0 * toF32(@as(u32, rgba[i + 2]));
        avg[3] += a;
    }
    if (avg[3] > 0.0) {
        avg[0] /= avg[3];
        avg[1] /= avg[3];
        avg[2] /= avg[3];
    }

    const npix: f32 = toF32(w * h);
    const has_alpha = avg[3] < npix;
    const l_limit: f32 = if (has_alpha) 5.0 else 7.0;
    const max_dim: u32 = if (w > h) w else h;
    const lx_raw: u32 = @intFromFloat(@round(l_limit * toF32(w) / toF32(max_dim)));
    const ly_raw: u32 = @intFromFloat(@round(l_limit * toF32(h) / toF32(max_dim)));
    const lx: u32 = if (lx_raw < 1) 1 else lx_raw;
    const ly: u32 = if (ly_raw < 1) 1 else ly_raw;

    let l_buf: [10000]f32 = undefined;
    let p_buf: [10000]f32 = undefined;
    let q_buf: [10000]f32 = undefined;
    let a_buf: [10000]f32 = undefined;
    i = 0;
    let px: usize = 0;
    while (i < rgba.len) {
        const al: f32 = toF32(@as(u32, rgba[i + 3])) / 255.0;
        const r = avg[0] * (1.0 - al) + al / 255.0 * toF32(@as(u32, rgba[i + 0]));
        const g = avg[1] * (1.0 - al) + al / 255.0 * toF32(@as(u32, rgba[i + 1]));
        const b = avg[2] * (1.0 - al) + al / 255.0 * toF32(@as(u32, rgba[i + 2]));
        l_buf[px] = (r + g + b) / 3.0;
        p_buf[px] = (r + g) / 2.0 - b;
        q_buf[px] = r - g;
        a_buf[px] = al;
        i += 4;
        px += 1;
    }

    const lc_nx: u32 = if (lx < 3) 3 else lx;
    const lc_ny: u32 = if (ly < 3) 3 else ly;
    const lc = dct(l_buf[0..px], w, h, lc_nx, lc_ny);
    const pc = dct(p_buf[0..px], w, h, 3, 3);
    const qc = dct(q_buf[0..px], w, h, 3, 3);
    const ac = if (has_alpha) dct(a_buf[0..px], w, h, 5, 5) else Channel{ dc: 1.0, scale: 1.0 };

    const land = w > h;
    const h24: u32 = @as(u32, @intFromFloat(@round(63.0 * lc.dc)))
        | (@as(u32, @intFromFloat(@round(31.5 + 31.5 * pc.dc))) << 6)
        | (@as(u32, @intFromFloat(@round(31.5 + 31.5 * qc.dc))) << 12)
        | (@as(u32, @intFromFloat(@round(31.0 * lc.scale))) << 18)
        | (@as(u32, @intFromBool(has_alpha)) << 23);
    const h16: u16 = @as(u16, @intCast(if (land) ly else lx))
        | (@as(u16, @intFromFloat(@round(63.0 * pc.scale))) << 3)
        | (@as(u16, @intFromFloat(@round(63.0 * qc.scale))) << 9)
        | (@as(u16, @intFromBool(land)) << 15);

    out[0] = @truncate(h24);
    out[1] = @truncate(h24 >> 8);
    out[2] = @truncate(h24 >> 16);
    out[3] = @truncate(h16);
    out[4] = @truncate(h16 >> 8);
    let n: usize = 5;
    if (has_alpha) {
        out[5] = @as(u8, @intFromFloat(@round(15.0 * ac.dc)))
            | (@as(u8, @intFromFloat(@round(15.0 * ac.scale))) << 4);
        n = 6;
    }

    let odd = false;
    const chs: [4]Channel = [lc, pc, qc, ac];
    let ci: usize = 0;
    while (ci < 4) : (ci += 1) {
        const ch = chs[ci];
        let k: usize = 0;
        while (k < ch.n) : (k += 1) {
            const u: u8 = @intFromFloat(@round(15.0 * ch.ac[k]));
            if (odd) {
                out[n - 1] |= u << 4;
            } else {
                out[n] = u;
                n += 1;
            }
            odd = !odd;
        }
    }
    return out[0..n];
}

struct NibbleReader {
    src: []const u8,
    i: usize,
    hi: bool = false,

    fn next(*_): u8! {
        if (_.i >= _.src.len) return err.DecodeFailed;
        const v: u8 = if (_.hi) _.src[_.i] >> 4 else _.src[_.i] & 15;
        if (_.hi) _.i += 1;
        _.hi = !_.hi;
        return v;
    }

    fn channel(*_, out: []f32, nx: u32, ny: u32, scale: f32): usize! {
        let n: usize = 0;
        let cy: u32 = 0;
        while (cy < ny) : (cy += 1) {
            let cx: u32 = if (cy > 0) @as(u32, 0) else @as(u32, 1);
            while (cx * ny < nx * (ny - cy)) : (cx += 1) {
                const nib = _.next()?;
                out[n] = (toF32(@as(u32, nib)) / 7.5 - 1.0) * scale;
                n += 1;
            }
        }
        return n;
    }
}

inline fn idct(ac: []const f32, nx: u32, ny: u32, fx: []const f32, fy: []const f32): f32 {
    let v: f32 = 0.0;
    let j: usize = 0;
    let cy: u32 = 0;
    while (cy < ny) : (cy += 1) {
        let cx: u32 = if (cy > 0) @as(u32, 0) else @as(u32, 1);
        const fy2 = fy[cy] * 2.0;
        while (cx * ny < nx * (ny - cy)) : (cx += 1) {
            v += ac[j] * fx[cx] * fy2;
            j += 1;
        }
    }
    return v;
}

/// Extract the approximate aspect ratio (`width / height`) from a ThumbHash without decoding pixels.
export fn approxAspectRatio(hash: []const u8): f32! {
    if (hash.len < 5) return err.DecodeFailed;
    const h24: u32 = @as(u32, hash[0]) | (@as(u32, hash[1]) << 8) | (@as(u32, hash[2]) << 16);
    const h16: u16 = @as(u16, hash[3]) | (@as(u16, hash[4]) << 8);
    const has_alpha = (h24 >> 23) != 0;
    const land = (h16 >> 15) != 0;
    const l_max: u32 = if (has_alpha) 5 else 7;
    const lx_raw: u32 = if (land) l_max else @as(u32, h16 & 7);
    const ly_raw: u32 = if (land) @as(u32, h16 & 7) else l_max;
    const lx: u32 = if (lx_raw < 3) 3 else lx_raw;
    const ly: u32 = if (ly_raw < 3) 3 else ly_raw;
    return toF32(lx) / toF32(ly);
}

/// Extract the average RGBA8 color directly from the 5–6 byte ThumbHash header (zero heap allocation).
export fn averageRGBA(hash: []const u8): [4]u8! {
    if (hash.len < 5) return err.DecodeFailed;
    const h24: u32 = @as(u32, hash[0]) | (@as(u32, hash[1]) << 8) | (@as(u32, hash[2]) << 16);
    const l_dc: f32 = toF32(h24 & 63) / 63.0;
    const p_dc: f32 = toF32((h24 >> 6) & 63) / 31.5 - 1.0;
    const q_dc: f32 = toF32((h24 >> 12) & 63) / 31.5 - 1.0;
    const has_alpha = (h24 >> 23) != 0;
    let a_dc: f32 = 1.0;
    if (has_alpha) {
        if (hash.len < 6) return err.DecodeFailed;
        a_dc = toF32(@as(u32, hash[5] & 15)) / 15.0;
    }
    const b: f32 = l_dc - (2.0 / 3.0) * p_dc;
    const r: f32 = (3.0 * l_dc - b + q_dc) / 2.0;
    const g: f32 = r - q_dc;
    return [clamp8(r), clamp8(g), clamp8(b), clamp8(a_dc)];
}

/// Decode a ThumbHash into a `<= 32px` RGBA8 image with deterministic `~init()` RAII cleanup.
export fn decode(allocator: std.mem.Allocator, hash: []const u8): DecodedImage! {
    if (hash.len < 5) return err.DecodeFailed;
    const h24: u32 = @as(u32, hash[0]) | (@as(u32, hash[1]) << 8) | (@as(u32, hash[2]) << 16);
    const h16: u16 = @as(u16, hash[3]) | (@as(u16, hash[4]) << 8);
    const l_dc: f32 = toF32(h24 & 63) / 63.0;
    const p_dc: f32 = toF32((h24 >> 6) & 63) / 31.5 - 1.0;
    const q_dc: f32 = toF32((h24 >> 12) & 63) / 31.5 - 1.0;
    const l_scale: f32 = toF32((h24 >> 18) & 31) / 31.0;
    const has_alpha = (h24 >> 23) != 0;
    const p_scale: f32 = toF32(@as(u32, (h16 >> 3) & 63)) / 63.0;
    const q_scale: f32 = toF32(@as(u32, (h16 >> 9) & 63)) / 63.0;
    const land = (h16 >> 15) != 0;
    const l_max: u32 = if (has_alpha) 5 else 7;
    const lx_raw: u32 = if (land) l_max else @as(u32, h16 & 7);
    const ly_raw: u32 = if (land) @as(u32, h16 & 7) else l_max;
    const lx: u32 = if (lx_raw < 3) 3 else lx_raw;
    const ly: u32 = if (ly_raw < 3) 3 else ly_raw;

    let a_dc: f32 = 1.0;
    let a_scale: f32 = 1.0;
    let off: usize = 5;
    if (has_alpha) {
        if (hash.len < 6) return err.DecodeFailed;
        a_dc = toF32(@as(u32, hash[5] & 15)) / 15.0;
        a_scale = toF32(@as(u32, hash[5] >> 4)) / 15.0;
        off = 6;
    }

    let nibbles = NibbleReader{ src: hash, i: off, hi: false };
    let l_ac: [49]f32 = [0.0; 49];
    let p_ac: [5]f32 = [0.0; 5];
    let q_ac: [5]f32 = [0.0; 5];
    let a_ac: [14]f32 = [0.0; 14];
    const ln = nibbles.channel(l_ac[0..], lx, ly, l_scale)?;
    const pn = nibbles.channel(p_ac[0..], 3, 3, p_scale * 1.25)?;
    const qn = nibbles.channel(q_ac[0..], 3, 3, q_scale * 1.25)?;
    const an: usize = if (has_alpha) (nibbles.channel(a_ac[0..], 5, 5, a_scale)?) else 0;

    const ratio = toF32(lx) / toF32(ly);
    const w: u32 = if (ratio > 1.0) 32 else @intFromFloat(@round(32.0 * ratio));
    const h: u32 = if (ratio > 1.0) @intFromFloat(@round(32.0 / ratio)) else 32;
    let rgba = allocator.alloc(u8, @as(usize, w) * @as(usize, h) * 4)?;
    errdefer allocator.free(rgba);

    let fx: [7]f32 = [0.0; 7];
    let fy: [7]f32 = [0.0; 7];
    const nf: u32 = if (has_alpha && lx < 5) 5 else if (lx < 3) 3 else lx;
    const my: u32 = if (has_alpha && ly < 5) 5 else if (ly < 3) 3 else ly;

    let y: u32 = 0;
    while (y < h) : (y += 1) {
        let cy: usize = 0;
        while (cy < @as(usize, my)) : (cy += 1) {
            fy[cy] = @cos(std.math.pi / toF32(h) * (toF32(y) + 0.5) * toF32Usize(cy));
        }
        let x: u32 = 0;
        while (x < w) : (x += 1) {
            let cx: usize = 0;
            while (cx < @as(usize, nf)) : (cx += 1) {
                fx[cx] = @cos(std.math.pi / toF32(w) * (toF32(x) + 0.5) * toF32Usize(cx));
            }
            let lv = l_dc + idct(l_ac[0..ln], lx, ly, fx[0..], fy[0..]);
            let pv = p_dc + idct(p_ac[0..pn], 3, 3, fx[0..], fy[0..]);
            let qv = q_dc + idct(q_ac[0..qn], 3, 3, fx[0..], fy[0..]);
            let av = a_dc;
            if (has_alpha) {
                av += idct(a_ac[0..an], 5, 5, fx[0..], fy[0..]);
            }
            const b: f32 = lv - (2.0 / 3.0) * pv;
            const r: f32 = (3.0 * lv - b + qv) / 2.0;
            const g: f32 = r - qv;
            const o: usize = (@as(usize, y) * @as(usize, w) + @as(usize, x)) * 4;
            rgba[o + 0] = clamp8(r);
            rgba[o + 1] = clamp8(g);
            rgba[o + 2] = clamp8(b);
            rgba[o + 3] = clamp8(av);
        }
    }

    return DecodedImage{
        allocator: allocator,
        rgba: rgba,
        width: w,
        height: h,
    };
}
