// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Standard RLE Suite: PackBits, PCX, TGA, Zero-RLE) | Created: 2026-11-22

// PackBits / PCX / TGA Constants
pub const PACKBITS_NOP: u8 = 0x80;
pub const PCX_RUN_MASK: u8 = 0xC0;
pub const PCX_MAX_RUN: usize = 63;
pub const TGA_RLE_FLAG: u8 = 0x80;
pub const TGA_MAX_PACKET: usize = 128;
pub const ZERO_RLE_MAX_RUN: usize = 255;

/// Computes the worst-case encoded buffer size for PackBits (`src_len + ceil(src_len / 128)`).
pub fn packbitsMaxEncodedSize(src_len: usize): usize {
    if (src_len == 0) {
        return 0;
    }
    return src_len + ((src_len + 127) / 128);
}

/// Encodes `src` using Apple / TIFF / PDF / PSD PackBits into `dst`.
/// Returns bytes written, or `null` if `dst` is too small.
pub fn packbitsEncode(src: []const u8, dst: []u8): usize? {
    let i: usize = 0;
    let out: usize = 0;

    while (i < src.len) {
        // Check for a run of >= 2 identical bytes
        let run: usize = 1;
        while (i + run < src.len && run < 128 && src[i + run] == src[i]) {
            run += 1;
        }
        if (run >= 2) {
            if (dst.len - out < 2) {
                return null;
            }
            dst[out] = ((257 as usize) - run) as u8;
            dst[out + 1] = src[i];
            out += 2;
            i += run;
        } else {
            // Collect literal bytes until next run of >= 2 (or 128 bytes max)
            const lit_start = i;
            let lit_len: usize = 1;
            i += 1;
            while (i < src.len && lit_len < 128) {
                if (i + 1 < src.len && src[i] == src[i + 1]) {
                    break;
                }
                i += 1;
                lit_len += 1;
            }
            if (dst.len - out < (1 as usize) + lit_len) {
                return null;
            }
            dst[out] = (lit_len - 1) as u8;
            out += 1;
            let k: usize = 0;
            while (k < lit_len) {
                dst[out + k] = src[lit_start + k];
                k += 1;
            }
            out += lit_len;
        }
    }
    return out;
}

/// Decodes Apple / TIFF / PDF / PSD PackBits from `src` into `dst`.
/// Skips `0x80` NOP control bytes and strictly guards against truncated input and output overflow.
pub fn packbitsDecode(src: []const u8, dst: []u8): usize? {
    let i: usize = 0;
    let out: usize = 0;

    while (i < src.len) {
        const ctrl = src[i];
        i += 1;
        if (ctrl == PACKBITS_NOP) {
            continue;
        }
        if (ctrl <= 0x7F) {
            const count = (ctrl as usize) + 1;
            if (src.len - i < count || dst.len - out < count) {
                return null;
            }
            let k: usize = 0;
            while (k < count) {
                dst[out + k] = src[i + k];
                k += 1;
            }
            i += count;
            out += count;
        } else {
            const count = (257 as usize) - (ctrl as usize);
            if (i >= src.len || dst.len - out < count) {
                return null;
            }
            const val = src[i];
            i += 1;
            let k: usize = 0;
            while (k < count) {
                dst[out + k] = val;
                k += 1;
            }
            out += count;
        }
    }
    return out;
}

/// Encodes `src` using ZSoft PCX RLE into `dst`.
/// Bytes `>= 0xC0` or runs of `>= 2` identical bytes are encoded as `[0xC0 | count, byte]` (max count 63).
pub fn pcxEncode(src: []const u8, dst: []u8): usize? {
    let i: usize = 0;
    let out: usize = 0;

    while (i < src.len) {
        const val = src[i];
        let run: usize = 1;
        while (i + run < src.len && run < PCX_MAX_RUN && src[i + run] == val) {
            run += 1;
        }
        if (run >= 2 || val >= PCX_RUN_MASK) {
            if (dst.len - out < 2) {
                return null;
            }
            dst[out] = PCX_RUN_MASK | (run as u8);
            dst[out + 1] = val;
            out += 2;
        } else {
            if (dst.len - out < 1) {
                return null;
            }
            dst[out] = val;
            out += 1;
        }
        i += run;
    }
    return out;
}

/// Decodes ZSoft PCX RLE from `src` into `dst`.
/// Strictly validates that every `0xC0..0xFF` run byte is followed by a data byte and output does not overflow.
pub fn pcxDecode(src: []const u8, dst: []u8): usize? {
    let i: usize = 0;
    let out: usize = 0;

    while (i < src.len) {
        const b = src[i];
        i += 1;
        if ((b & PCX_RUN_MASK) == PCX_RUN_MASK) {
            const count = (b & 0x3F) as usize;
            if (i >= src.len || dst.len - out < count) {
                return null;
            }
            const val = src[i];
            i += 1;
            let k: usize = 0;
            while (k < count) {
                dst[out + k] = val;
                k += 1;
            }
            out += count;
        } else {
            if (dst.len - out < 1) {
                return null;
            }
            dst[out] = b;
            out += 1;
        }
    }
    return out;
}

fn pixelEq(buf: []const u8, a: usize, b: usize, bpp: usize): bool {
    let k: usize = 0;
    while (k < bpp) {
        if (buf[a + k] != buf[b + k]) {
            return false;
        }
        k += 1;
    }
    return true;
}

/// Encodes pixel buffer `src` using Truevision TGA RLE (`bpp_bytes` in `1..=4`) into `dst`.
/// Returns bytes written, or `null` on invalid `bpp_bytes`, unaligned `src.len`, or `dst` overflow.
pub fn tgaRleEncode(src: []const u8, bpp_bytes: usize, dst: []u8): usize? {
    if (bpp_bytes < 1 || bpp_bytes > 4) {
        return null;
    }
    if ((src.len % bpp_bytes) != 0) {
        return null;
    }
    const total_pixels = src.len / bpp_bytes;
    let px: usize = 0;
    let out: usize = 0;

    while (px < total_pixels) {
        const base = px * bpp_bytes;
        let run: usize = 1;
        while (px + run < total_pixels && run < TGA_MAX_PACKET && pixelEq(src, base, (px + run) * bpp_bytes, bpp_bytes)) {
            run += 1;
        }
        if (run >= 2) {
            if (dst.len - out < (1 as usize) + bpp_bytes) {
                return null;
            }
            dst[out] = TGA_RLE_FLAG | ((run - 1) as u8);
            out += 1;
            let k: usize = 0;
            while (k < bpp_bytes) {
                dst[out + k] = src[base + k];
                k += 1;
            }
            out += bpp_bytes;
            px += run;
        } else {
            const raw_start_px = px;
            let raw_cnt: usize = 1;
            px += 1;
            while (px < total_pixels && raw_cnt < TGA_MAX_PACKET) {
                if (px + 1 < total_pixels && pixelEq(src, px * bpp_bytes, (px + 1) * bpp_bytes, bpp_bytes)) {
                    break;
                }
                px += 1;
                raw_cnt += 1;
            }
            const raw_bytes = raw_cnt * bpp_bytes;
            if (dst.len - out < (1 as usize) + raw_bytes) {
                return null;
            }
            dst[out] = (raw_cnt - 1) as u8;
            out += 1;
            const src_off = raw_start_px * bpp_bytes;
            let k: usize = 0;
            while (k < raw_bytes) {
                dst[out + k] = src[src_off + k];
                k += 1;
            }
            out += raw_bytes;
        }
    }
    return out;
}

/// Decodes Truevision TGA RLE pixel packets (`bpp_bytes` in `1..=4`) from `src` into `dst`.
pub fn tgaRleDecode(src: []const u8, bpp_bytes: usize, dst: []u8): usize? {
    if (bpp_bytes < 1 || bpp_bytes > 4) {
        return null;
    }
    let i: usize = 0;
    let out: usize = 0;

    while (i < src.len) {
        const hdr = src[i];
        i += 1;
        const count = ((hdr & 0x7F) as usize) + 1;
        const out_bytes = count * bpp_bytes;

        if ((hdr & TGA_RLE_FLAG) != 0) {
            if (src.len - i < bpp_bytes || dst.len - out < out_bytes) {
                return null;
            }
            let p: usize = 0;
            while (p < count) {
                let k: usize = 0;
                while (k < bpp_bytes) {
                    dst[out + k] = src[i + k];
                    k += 1;
                }
                out += bpp_bytes;
                p += 1;
            }
            i += bpp_bytes;
        } else {
            if (src.len - i < out_bytes || dst.len - out < out_bytes) {
                return null;
            }
            let k: usize = 0;
            while (k < out_bytes) {
                dst[out + k] = src[i + k];
                k += 1;
            }
            i += out_bytes;
            out += out_bytes;
        }
    }
    return out;
}

/// Encodes `src` using Zero-Byte RLE (RLE0): consecutive `0x00` bytes become `[0x00, count]` (1..=255).
pub fn zeroRleEncode(src: []const u8, dst: []u8): usize? {
    let i: usize = 0;
    let out: usize = 0;

    while (i < src.len) {
        if (src[i] == 0) {
            let run: usize = 1;
            while (i + run < src.len && run < ZERO_RLE_MAX_RUN && src[i + run] == 0) {
                run += 1;
            }
            if (dst.len - out < 2) {
                return null;
            }
            dst[out] = 0;
            dst[out + 1] = run as u8;
            out += 2;
            i += run;
        } else {
            if (dst.len - out < 1) {
                return null;
            }
            dst[out] = src[i];
            out += 1;
            i += 1;
        }
    }
    return out;
}

/// Decodes Zero-Byte RLE (RLE0) from `src` into `dst`.
/// Strictly rejects truncated `[0x00]` escape, zero count (`[0x00, 0x00]`), and output overflow.
pub fn zeroRleDecode(src: []const u8, dst: []u8): usize? {
    let i: usize = 0;
    let out: usize = 0;

    while (i < src.len) {
        if (src[i] == 0) {
            i += 1;
            if (i >= src.len) {
                return null;
            }
            const count = src[i] as usize;
            i += 1;
            if (count == 0 || dst.len - out < count) {
                return null;
            }
            let k: usize = 0;
            while (k < count) {
                dst[out + k] = 0;
                k += 1;
            }
            out += count;
        } else {
            if (dst.len - out < 1) {
                return null;
            }
            dst[out] = src[i];
            out += 1;
            i += 1;
        }
    }
    return out;
}
