// @cap/color-quant — Zero-allocation pure CP 256-color palette quantizer
// Parity with image-rs crates.io:color_quant (MIT)

/// 24-bit RGB pixel color.
pub struct Rgb8 {
    r: u8;
    g: u8;
    b: u8;
}

/// Fixed-capacity 256-entry RGB color palette with nearest-color lookup.
pub struct ColorPalette {
    colors: [256]Rgb8;
    len: usize;

    /// Returns the palette index with minimum squared Euclidean RGB distance to `px`.
    fn nearestIndex(px: Rgb8): u8 {
        if (_.len == 0) {
            return 0;
        }
        const limit: usize = if (_.len <= 256) _.len else 256;
        let bestIdx: usize = 0;
        let bestDist: i32 = 2147483647;
        let i: usize = 0;
        while (i < limit) {
            const c = _.colors[i];
            const dr: i32 = (px.r as i32) - (c.r as i32);
            const dg: i32 = (px.g as i32) - (c.g as i32);
            const db: i32 = (px.b as i32) - (c.b as i32);
            const dist: i32 = dr * dr + dg * dg + db * db;
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = i;
                if (dist == 0) {
                    return bestIdx as u8;
                }
            }
            i += 1;
        }
        return bestIdx as u8;
    }
}

fn clqtEmptyPalette(): ColorPalette {
    let colors: [256]Rgb8 = undefined;
    const black = Rgb8{ r: 0, g: 0, b: 0 };
    let i: usize = 0;
    while (i < 256) {
        colors[i] = black;
        i += 1;
    }
    return ColorPalette{ colors: colors, len: 0 };
}

fn clqtBucketIndex(px: Rgb8): usize {
    const r3: usize = (px.r >> 5) as usize;
    const g3: usize = (px.g >> 5) as usize;
    const b2: usize = (px.b >> 6) as usize;
    return (r3 << 5) | (g3 << 2) | b2;
}

/// Build a standard 256-color uniform palette (6x6x6 Web-safe cube + 40 grayscale ramp entries).
pub fn buildUniformPalette(): ColorPalette {
    let pal = clqtEmptyPalette();
    let idx: usize = 0;
    let ri: usize = 0;
    while (ri < 6) {
        const rv: u8 = (ri * 51) as u8;
        let gi: usize = 0;
        while (gi < 6) {
            const gv: u8 = (gi * 51) as u8;
            let bi: usize = 0;
            while (bi < 6) {
                const bv: u8 = (bi * 51) as u8;
                pal.colors[idx] = Rgb8{ r: rv, g: gv, b: bv };
                idx += 1;
                bi += 1;
            }
            gi += 1;
        }
        ri += 1;
    }
    // 40 intermediate grayscale entries (excluding 0 and 255 already present in cube)
    let gi2: usize = 0;
    while (gi2 < 40) {
        const gray: u8 = (((gi2 + 1) * 255) / 41) as u8;
        pal.colors[idx] = Rgb8{ r: gray, g: gray, b: gray };
        idx += 1;
        gi2 += 1;
    }
    pal.len = 256;
    return pal;
}

/// Quantize `pixels` into a compact palette of at most `max_colors` (<= 256) colors
/// using 3-3-2 histogram bucketing and per-bucket mean representative color extraction.
pub fn quantizeHistogram(pixels: []const Rgb8, max_colors: usize): ColorPalette {
    let pal = clqtEmptyPalette();
    if (pixels.len == 0 || max_colors == 0) {
        return pal;
    }
    const limit: usize = if (max_colors <= 256) max_colors else 256;

    let counts: [256]u32 = undefined;
    let sumR: [256]u64 = undefined;
    let sumG: [256]u64 = undefined;
    let sumB: [256]u64 = undefined;
    let used: [256]bool = undefined;
    let bi: usize = 0;
    while (bi < 256) {
        counts[bi] = 0;
        sumR[bi] = 0;
        sumG[bi] = 0;
        sumB[bi] = 0;
        used[bi] = false;
        bi += 1;
    }

    let pi: usize = 0;
    while (pi < pixels.len) {
        const px = pixels[pi];
        const bIdx = clqtBucketIndex(px);
        counts[bIdx] += 1;
        sumR[bIdx] += px.r as u64;
        sumG[bIdx] += px.g as u64;
        sumB[bIdx] += px.b as u64;
        pi += 1;
    }

    while (pal.len < limit) {
        let bestBucket: usize = 0;
        let bestCount: u32 = 0;
        let b: usize = 0;
        while (b < 256) {
            if (!used[b] && counts[b] > bestCount) {
                bestCount = counts[b];
                bestBucket = b;
            }
            b += 1;
        }
        if (bestCount == 0) {
            break;
        }
        used[bestBucket] = true;
        const cnt: u64 = bestCount as u64;
        const half: u64 = cnt / 2;
        const mr: u8 = ((sumR[bestBucket] + half) / cnt) as u8;
        const mg: u8 = ((sumG[bestBucket] + half) / cnt) as u8;
        const mb: u8 = ((sumB[bestBucket] + half) / cnt) as u8;
        pal.colors[pal.len] = Rgb8{ r: mr, g: mg, b: mb };
        pal.len += 1;
    }

    return pal;
}

/// Map each pixel in `pixels` to its nearest palette index in `pal`, writing to `out_indices`.
/// Returns the number of pixels mapped.
pub fn mapPixelsToPalette(pixels: []const Rgb8, pal: ColorPalette, out_indices: []u8): usize {
    if (pal.len == 0) {
        return 0;
    }
    const n: usize = if (pixels.len < out_indices.len) pixels.len else out_indices.len;
    let i: usize = 0;
    while (i < n) {
        out_indices[i] = pal.nearestIndex(pixels[i]);
        i += 1;
    }
    return n;
}
