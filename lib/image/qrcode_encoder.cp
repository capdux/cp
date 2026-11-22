// Source: cp/lib/qrcode/encoder.cp (ISO/IEC 18004 QR Code Model 2 Encoder Versions 1..10)
// Pure CP ISO/IEC 18004 QR Code Model 2 encoder supporting Versions 1..10 in Byte mode,
// multi-block Reed-Solomon interleaving, all function/alignment/version patterns,
// 15-bit BCH format info, and 8-mask penalty minimization.

const std = @import("std");
import { ReedSolomon } from "./qrcode_ecc.cp";

pub const QR_MIN_VERSION: u8 = 1;
pub const QR_MAX_VERSION: u8 = 10;

const MAX_CODEWORDS: usize = 352;
const MAX_BLOCKS: usize = 8;
const MAX_EC_PER_BLOCK: usize = 32;
const MAX_MODULES: usize = 57 * 57;

/// Error set for QR Code encoding and rendering.
pub err QrError {
    DataTooLong,
    InvalidVersion,
    OutOfMemory,
}

/// ISO/IEC 18004 QR Code error correction levels.
pub enum QrEccLevel: u2 {
    Low = 0,
    Medium = 1,
    Quartile = 2,
    High = 3,

    /// Returns the 2-bit format indicator defined in ISO/IEC 18004 Table 12.
    fn formatBits(): u2 {
        return switch (_) {
            .Low => 0b01,
            .Medium => 0b00,
            .Quartile => 0b11,
            .High => 0b10,
        };
    }

    fn index(): usize {
        return switch (_) {
            .Low => 0,
            .Medium => 1,
            .Quartile => 2,
            .High => 3,
        };
    }
}

struct VersionEccSpec {
    total_codewords: u16,
    ec_per_block: u8,
    g1_blocks: u8,
    g1_data: u8,
    g2_blocks: u8,
    g2_data: u8,

    fn totalDataCodewords(): usize {
        return usize(_.g1_blocks) * usize(_.g1_data) + usize(_.g2_blocks) * usize(_.g2_data);
    }
}

const VERSION_ECC_TABLE: [40]VersionEccSpec = [
    // Version 1 (21x21, total = 26)
    VersionEccSpec{ total_codewords: 26, ec_per_block: 7, g1_blocks: 1, g1_data: 19, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 26, ec_per_block: 10, g1_blocks: 1, g1_data: 16, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 26, ec_per_block: 13, g1_blocks: 1, g1_data: 13, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 26, ec_per_block: 17, g1_blocks: 1, g1_data: 9, g2_blocks: 0, g2_data: 0 },
    // Version 2 (25x25, total = 44)
    VersionEccSpec{ total_codewords: 44, ec_per_block: 10, g1_blocks: 1, g1_data: 34, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 44, ec_per_block: 16, g1_blocks: 1, g1_data: 28, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 44, ec_per_block: 22, g1_blocks: 1, g1_data: 22, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 44, ec_per_block: 28, g1_blocks: 1, g1_data: 16, g2_blocks: 0, g2_data: 0 },
    // Version 3 (29x29, total = 70)
    VersionEccSpec{ total_codewords: 70, ec_per_block: 15, g1_blocks: 1, g1_data: 55, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 70, ec_per_block: 26, g1_blocks: 1, g1_data: 44, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 70, ec_per_block: 18, g1_blocks: 2, g1_data: 17, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 70, ec_per_block: 22, g1_blocks: 2, g1_data: 13, g2_blocks: 0, g2_data: 0 },
    // Version 4 (33x33, total = 100)
    VersionEccSpec{ total_codewords: 100, ec_per_block: 20, g1_blocks: 1, g1_data: 80, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 100, ec_per_block: 18, g1_blocks: 2, g1_data: 32, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 100, ec_per_block: 26, g1_blocks: 2, g1_data: 24, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 100, ec_per_block: 16, g1_blocks: 4, g1_data: 9, g2_blocks: 0, g2_data: 0 },
    // Version 5 (37x37, total = 134)
    VersionEccSpec{ total_codewords: 134, ec_per_block: 26, g1_blocks: 1, g1_data: 108, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 134, ec_per_block: 24, g1_blocks: 2, g1_data: 43, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 134, ec_per_block: 18, g1_blocks: 2, g1_data: 15, g2_blocks: 2, g2_data: 16 },
    VersionEccSpec{ total_codewords: 134, ec_per_block: 22, g1_blocks: 2, g1_data: 11, g2_blocks: 2, g2_data: 12 },
    // Version 6 (41x41, total = 172)
    VersionEccSpec{ total_codewords: 172, ec_per_block: 18, g1_blocks: 2, g1_data: 68, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 172, ec_per_block: 16, g1_blocks: 4, g1_data: 27, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 172, ec_per_block: 24, g1_blocks: 4, g1_data: 19, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 172, ec_per_block: 28, g1_blocks: 4, g1_data: 15, g2_blocks: 0, g2_data: 0 },
    // Version 7 (45x45, total = 196)
    VersionEccSpec{ total_codewords: 196, ec_per_block: 20, g1_blocks: 2, g1_data: 78, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 196, ec_per_block: 18, g1_blocks: 4, g1_data: 31, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 196, ec_per_block: 18, g1_blocks: 2, g1_data: 14, g2_blocks: 4, g2_data: 15 },
    VersionEccSpec{ total_codewords: 196, ec_per_block: 26, g1_blocks: 4, g1_data: 13, g2_blocks: 1, g2_data: 14 },
    // Version 8 (49x49, total = 242)
    VersionEccSpec{ total_codewords: 242, ec_per_block: 24, g1_blocks: 2, g1_data: 97, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 242, ec_per_block: 22, g1_blocks: 2, g1_data: 38, g2_blocks: 2, g2_data: 39 },
    VersionEccSpec{ total_codewords: 242, ec_per_block: 22, g1_blocks: 4, g1_data: 18, g2_blocks: 2, g2_data: 19 },
    VersionEccSpec{ total_codewords: 242, ec_per_block: 26, g1_blocks: 4, g1_data: 14, g2_blocks: 2, g2_data: 15 },
    // Version 9 (53x53, total = 292)
    VersionEccSpec{ total_codewords: 292, ec_per_block: 30, g1_blocks: 2, g1_data: 116, g2_blocks: 0, g2_data: 0 },
    VersionEccSpec{ total_codewords: 292, ec_per_block: 22, g1_blocks: 3, g1_data: 36, g2_blocks: 2, g2_data: 37 },
    VersionEccSpec{ total_codewords: 292, ec_per_block: 20, g1_blocks: 4, g1_data: 16, g2_blocks: 4, g2_data: 17 },
    VersionEccSpec{ total_codewords: 292, ec_per_block: 24, g1_blocks: 4, g1_data: 12, g2_blocks: 4, g2_data: 13 },
    // Version 10 (57x57, total = 346)
    VersionEccSpec{ total_codewords: 346, ec_per_block: 18, g1_blocks: 2, g1_data: 68, g2_blocks: 2, g2_data: 69 },
    VersionEccSpec{ total_codewords: 346, ec_per_block: 26, g1_blocks: 4, g1_data: 43, g2_blocks: 1, g2_data: 44 },
    VersionEccSpec{ total_codewords: 346, ec_per_block: 24, g1_blocks: 6, g1_data: 19, g2_blocks: 2, g2_data: 20 },
    VersionEccSpec{ total_codewords: 346, ec_per_block: 28, g1_blocks: 6, g1_data: 15, g2_blocks: 2, g2_data: 16 },
];

fn getSpec(version: u8, ecc: QrEccLevel): VersionEccSpec {
    return VERSION_ECC_TABLE[usize(version - 1) * 4 + ecc.index()];
}

fn alignmentPositions(version: u8, out: *[3]usize): usize {
    return switch (version) {
        1 => 0,
        2 => { out[0] = 6; out[1] = 18; return 2; },
        3 => { out[0] = 6; out[1] = 22; return 2; },
        4 => { out[0] = 6; out[1] = 26; return 2; },
        5 => { out[0] = 6; out[1] = 30; return 2; },
        6 => { out[0] = 6; out[1] = 34; return 2; },
        7 => { out[0] = 6; out[1] = 22; out[2] = 38; return 3; },
        8 => { out[0] = 6; out[1] = 24; out[2] = 42; return 3; },
        9 => { out[0] = 6; out[1] = 26; out[2] = 46; return 3; },
        10 => { out[0] = 6; out[1] = 28; out[2] = 50; return 3; },
        else => 0,
    };
}

/// Encoded QR Code matrix (`size x size` row-major boolean modules; `true` = dark, `false` = light).
pub struct QrMatrix {
    version: u8 = 1,
    size: usize = 21,
    ecc: QrEccLevel = .Medium,
    mask: u8 = 0,
    modules: []bool = [],
    alloc: std.mem.Allocator = undefined,

    /// Returns `true` if module at `(x, y)` is dark; out-of-bounds returns `false`.
    fn get(x: usize, y: usize): bool {
        if (x >= _.size || y >= _.size) return false;
        return _.modules[y * _.size + x];
    }

    /// Releases heap memory owned by this QR matrix.
    fn deinit(*_) {
        if (_.modules.len > 0) {
            _.alloc.free(_.modules);
            _.modules = [];
        }
    }
}

fn setModule(modules: []bool, is_func: []bool, size: usize, x: usize, y: usize, val: bool, mark_func: bool) {
    const idx = y * size + x;
    modules[idx] = val;
    if (mark_func) {
        is_func[idx] = true;
    }
}

fn drawFinderWithSeparator(modules: []bool, is_func: []bool, size: usize, ox: isize, oy: isize) {
    let dy: isize = -1;
    while (dy <= 7) : (dy += 1) {
        const y = oy + dy;
        if (y < 0 || y >= (size as isize)) continue;
        let dx: isize = -1;
        while (dx <= 7) : (dx += 1) {
            const x = ox + dx;
            if (x < 0 || x >= (size as isize)) continue;
            const in_box = dx >= 0 && dx <= 6 && dy >= 0 && dy <= 6;
            const dark = in_box && (
                dx == 0 || dx == 6 || dy == 0 || dy == 6 ||
                (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4)
            );
            setModule(modules, is_func, size, x as usize, y as usize, dark, true);
        }
    }
}

fn drawAlignmentPattern(modules: []bool, is_func: []bool, size: usize, cx: usize, cy: usize) {
    let dy: isize = -2;
    while (dy <= 2) : (dy += 1) {
        let dx: isize = -2;
        while (dx <= 2) : (dx += 1) {
            const x = ((cx as isize) + dx) as usize;
            const y = ((cy as isize) + dy) as usize;
            const adx = if (dx < 0) -dx else dx;
            const ady = if (dy < 0) -dy else dy;
            const dark = adx == 2 || ady == 2 || (dx == 0 && dy == 0);
            setModule(modules, is_func, size, x, y, dark, true);
        }
    }
}

fn computeFormatBch(ecc: QrEccLevel, mask_id: u8): u16 {
    const data5: u16 = ((ecc.formatBits() as u16) << 3) | ((mask_id as u16) & 0x07);
    let rem: u16 = data5 << 10;
    let step: usize = 0;
    while (step < 5) : (step += 1) {
        if ((rem & 0x4000) != 0) {
            rem = rem ^ (0x537 << 4);
        }
        rem = rem << 1;
    }
    return ((data5 << 10) | (rem >> 5)) ^ 0x5412;
}

fn computeVersionBch(version: u8): u32 {
    const data6: u32 = version as u32;
    let rem: u32 = data6 << 12;
    let step: usize = 0;
    while (step < 6) : (step += 1) {
        if ((rem & 0x20000) != 0) {
            rem = rem ^ (0x1F25 << 5);
        }
        rem = rem << 1;
    }
    return (data6 << 12) | ((rem >> 6) & 0xFFF);
}

fn drawFunctionPatterns(modules: []bool, is_func: []bool, version: u8, size: usize) {
    // 1. Three 7x7 finder patterns + 1-module separators
    drawFinderWithSeparator(modules, is_func, size, 0, 0);
    drawFinderWithSeparator(modules, is_func, size, (size as isize) - 7, 0);
    drawFinderWithSeparator(modules, is_func, size, 0, (size as isize) - 7);

    // 2. Timing patterns (row 6 and column 6)
    let t: usize = 8;
    while (t < size - 8) : (t += 1) {
        const dark = (t % 2) == 0;
        setModule(modules, is_func, size, t, 6, dark, true);
        setModule(modules, is_func, size, 6, t, dark, true);
    }

    // 3. Alignment patterns (5x5)
    let pos_buf: [3]usize = [0; 3];
    const pos_count = alignmentPositions(version, &pos_buf);
    let i: usize = 0;
    while (i < pos_count) : (i += 1) {
        let j: usize = 0;
        while (j < pos_count) : (j += 1) {
            const cx = pos_buf[i];
            const cy = pos_buf[j];
            if ((cx == 6 && cy == 6) || (cx == 6 && cy == size - 7) || (cx == size - 7 && cy == 6)) {
                continue;
            }
            drawAlignmentPattern(modules, is_func, size, cx, cy);
        }
    }

    // 4. Always-dark module at (8, size - 8)
    setModule(modules, is_func, size, 8, size - 8, true, true);

    // 5. Reserve format information areas (filled per mask later)
    let fx: usize = 0;
    while (fx <= 8) : (fx += 1) {
        if (fx != 6) {
            is_func[8 * size + fx] = true;
        }
    }
    let fy: usize = 0;
    while (fy <= 7) : (fy += 1) {
        if (fy != 6) {
            is_func[fy * size + 8] = true;
        }
    }
    let bfy: usize = size - 7;
    while (bfy < size) : (bfy += 1) {
        is_func[bfy * size + 8] = true;
    }
    let rfx: usize = size - 8;
    while (rfx < size) : (rfx += 1) {
        is_func[8 * size + rfx] = true;
    }

    // 6. Version information for versions 7..10
    if (version >= 7) {
        const ver_bits = computeVersionBch(version);
        let v_mask: u32 = 1;
        let bit_idx: usize = 0;
        while (bit_idx < 18) : (bit_idx += 1) {
            const bit = (ver_bits & v_mask) != 0;
            v_mask = v_mask << 1;
            const a = bit_idx / 3;
            const b = size - 11 + (bit_idx % 3);
            setModule(modules, is_func, size, a, b, bit, true);
            setModule(modules, is_func, size, b, a, bit, true);
        }
    }
}

fn writeFormatInfo(modules: []bool, size: usize, ecc: QrEccLevel, mask_id: u8) {
    const bits = computeFormatBch(ecc, mask_id);

    // Top-left copy: bit 14 -> 0
    // Horizontal (y = 8): x = 0..5 (bits 14..9), x = 7 (bit 8), x = 8 (bit 7)
    modules[8 * size + 0] = ((bits >> 14) & 1) != 0;
    modules[8 * size + 1] = ((bits >> 13) & 1) != 0;
    modules[8 * size + 2] = ((bits >> 12) & 1) != 0;
    modules[8 * size + 3] = ((bits >> 11) & 1) != 0;
    modules[8 * size + 4] = ((bits >> 10) & 1) != 0;
    modules[8 * size + 5] = ((bits >> 9) & 1) != 0;
    modules[8 * size + 7] = ((bits >> 8) & 1) != 0;
    modules[8 * size + 8] = ((bits >> 7) & 1) != 0;

    // Vertical (x = 8): y = 7 (bit 6), y = 5..0 (bits 5..0)
    modules[7 * size + 8] = ((bits >> 6) & 1) != 0;
    modules[5 * size + 8] = ((bits >> 5) & 1) != 0;
    modules[4 * size + 8] = ((bits >> 4) & 1) != 0;
    modules[3 * size + 8] = ((bits >> 3) & 1) != 0;
    modules[2 * size + 8] = ((bits >> 2) & 1) != 0;
    modules[1 * size + 8] = ((bits >> 1) & 1) != 0;
    modules[0 * size + 8] = (bits & 1) != 0;

    // Split copy: bottom-left (x = 8, y = size-1..size-7 = bits 14..8)
    let f_mask: u16 = 0x4000;
    let k: usize = 0;
    while (k < 7) : (k += 1) {
        modules[(size - 1 - k) * size + 8] = (bits & f_mask) != 0;
        f_mask = f_mask >> 1;
    }
    // Top-right (y = 8, x = size-8..size-1 = bits 7..0)
    let m: usize = 0;
    while (m < 8) : (m += 1) {
        modules[8 * size + (size - 8 + m)] = (bits & f_mask) != 0;
        f_mask = f_mask >> 1;
    }
}

fn maskMatches(mask_id: u8, x: usize, y: usize): bool {
    return switch (mask_id) {
        0 => ((y + x) % 2) == 0,
        1 => (y % 2) == 0,
        2 => (x % 3) == 0,
        3 => ((y + x) % 3) == 0,
        4 => (((y / 2) + (x / 3)) % 2) == 0,
        5 => (((y * x) % 2) + ((y * x) % 3)) == 0,
        6 => ((((y * x) % 2) + ((y * x) % 3)) % 2) == 0,
        7 => ((((y + x) % 2) + ((y * x) % 3)) % 2) == 0,
        else => false,
    };
}

const BIT_MASKS: [8]u8 = [0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01];

fn placeCodewords(modules: []bool, is_func: []const bool, size: usize, codewords: []const u8) {
    const total_bits = codewords.len * 8;
    let bit_idx: usize = 0;
    let upward = true;
    let right_col: isize = (size as isize) - 1;

    while (right_col >= 1) {
        if (right_col == 6) {
            right_col = 5;
        }
        let step: usize = 0;
        while (step < size) : (step += 1) {
            const y: usize = if (upward) (size - 1 - step) else step;
            let c_off: usize = 0;
            while (c_off < 2) : (c_off += 1) {
                const x = (right_col as usize) - c_off;
                const idx = y * size + x;
                if (!is_func[idx]) {
                    let bit = false;
                    if (bit_idx < total_bits) {
                        const byte_val = codewords[bit_idx >> 3];
                        bit = (byte_val & BIT_MASKS[bit_idx & 7]) != 0;
                        bit_idx += 1;
                    }
                    modules[idx] = bit;
                }
            }
        }
        upward = !upward;
        right_col -= 2;
    }
}

fn penaltyLineRunAndFinder(modules: []const bool, size: usize, is_row: bool): usize {
    let score: usize = 0;
    let outer: usize = 0;
    while (outer < size) : (outer += 1) {
        // Rule 1: runs of >= 5 same-color modules
        let run_color = false;
        let run_len: usize = 0;
        let inner: usize = 0;
        while (inner < size) : (inner += 1) {
            const idx = if (is_row) (outer * size + inner) else (inner * size + outer);
            const c = modules[idx];
            if (inner == 0 || c != run_color) {
                if (run_len >= 5) {
                    score += run_len - 2;
                }
                run_color = c;
                run_len = 1;
            } else {
                run_len += 1;
            }
        }
        if (run_len >= 5) {
            score += run_len - 2;
        }

        // Rule 3: 1:1:3:1:1 finder-like pattern with 4 light modules on either side
        if (size >= 11) {
            let start: usize = 0;
            while (start + 11 <= size) : (start += 1) {
                let pat: u16 = 0;
                let k: usize = 0;
                while (k < 11) : (k += 1) {
                    const pos = start + k;
                    const idx = if (is_row) (outer * size + pos) else (pos * size + outer);
                    const bit_val: u16 = if (modules[idx]) 1 else 0;
                    pat = (pat << 1) | bit_val;
                }
                if (pat == 0b10111010000 || pat == 0b00001011101) {
                    score += 40;
                }
            }
        }
    }
    return score;
}

fn evaluatePenalty(modules: []const bool, size: usize): usize {
    let penalty: usize = 0;

    // Rules 1 & 3 (rows + columns)
    penalty += penaltyLineRunAndFinder(modules, size, true);
    penalty += penaltyLineRunAndFinder(modules, size, false);

    // Rule 2: 2x2 blocks of same color
    let y: usize = 0;
    while (y + 1 < size) : (y += 1) {
        let x: usize = 0;
        while (x + 1 < size) : (x += 1) {
            const c = modules[y * size + x];
            if (modules[y * size + (x + 1)] == c &&
                modules[(y + 1) * size + x] == c &&
                modules[(y + 1) * size + (x + 1)] == c)
            {
                penalty += 3;
            }
        }
    }

    // Rule 4: proportion of dark modules vs 50%
    let dark_count: usize = 0;
    const total = size * size;
    let i: usize = 0;
    while (i < total) : (i += 1) {
        if (modules[i]) dark_count += 1;
    }
    const percent = (dark_count * 100) / total;
    const prev5 = (percent / 5) * 5;
    const next5 = prev5 + 5;
    const d1 = if (prev5 >= 50) (prev5 - 50) else (50 - prev5);
    const d2 = if (next5 >= 50) (next5 - 50) else (50 - next5);
    const min_d = if (d1 < d2) d1 else d2;
    penalty += (min_d / 5) * 10;

    return penalty;
}

struct BitWriter {
    buf: *[MAX_CODEWORDS]u8,
    bit_len: usize = 0,

    fn pushBits(*_, value: u32, count: usize) {
        if (count == 0) return;
        let bit_mask: u32 = 1;
        let s: usize = 1;
        while (s < count) : (s += 1) {
            bit_mask = bit_mask << 1;
        }
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if ((value & bit_mask) != 0) {
                const byte_idx = _.bit_len >> 3;
                _.buf[byte_idx] = _.buf[byte_idx] | BIT_MASKS[_.bit_len & 7];
            }
            bit_mask = bit_mask >> 1;
            _.bit_len += 1;
        }
    }
}

fn encodeDataCodewords(data: []const u8, version: u8, total_data: usize, out_data: *[MAX_CODEWORDS]u8) {
    let i: usize = 0;
    while (i < total_data) : (i += 1) {
        out_data[i] = 0;
    }

    let bw = BitWriter{ buf: out_data, bit_len: 0 };
    // Mode indicator: Byte mode = 0100 (4 bits)
    bw.pushBits(0b0100, 4);

    // Character count indicator: 8 bits for V1..9, 16 bits for V10..26
    const count_bits: usize = if (version <= 9) 8 else 16;
    bw.pushBits(data.len as u32, count_bits);

    for (b of data) {
        bw.pushBits(b as u32, 8);
    }

    const cap_bits = total_data * 8;
    const rem_bits = cap_bits - bw.bit_len;
    const term_bits = if (rem_bits < 4) rem_bits else 4;
    if (term_bits > 0) {
        bw.pushBits(0, term_bits);
    }

    while ((bw.bit_len & 7) != 0) {
        bw.pushBits(0, 1);
    }

    let byte_len = bw.bit_len >> 3;
    let pad_toggle = false;
    while (byte_len < total_data) {
        out_data[byte_len] = if (pad_toggle) 0x11 else 0xEC;
        pad_toggle = !pad_toggle;
        byte_len += 1;
    }
}

fn buildInterleavedCodewords(
    data_cw: []const u8,
    spec: VersionEccSpec,
    out_all: *[MAX_CODEWORDS]u8,
): usize {
    const g1 = usize(spec.g1_blocks);
    const g2 = usize(spec.g2_blocks);
    const num_blocks = g1 + g2;
    const d1 = usize(spec.g1_data);
    const d2 = usize(spec.g2_data);
    const ec_len = usize(spec.ec_per_block);

    let block_starts: [MAX_BLOCKS]usize = [0; MAX_BLOCKS];
    let block_lens: [MAX_BLOCKS]usize = [0; MAX_BLOCKS];
    let ecc_blocks: [MAX_BLOCKS][MAX_EC_PER_BLOCK]u8 = [[0; MAX_EC_PER_BLOCK]; MAX_BLOCKS];

    let offset: usize = 0;
    let b: usize = 0;
    while (b < num_blocks) : (b += 1) {
        const blen = if (b < g1) d1 else d2;
        block_starts[b] = offset;
        block_lens[b] = blen;
        ReedSolomon.computeEcc(data_cw[offset .. offset + blen], ec_len, ecc_blocks[b][0..ec_len]);
        offset += blen;
    }

    let out_pos: usize = 0;
    const max_data_len = if (g2 > 0) d2 else d1;

    // Interleave data codewords
    let col: usize = 0;
    while (col < max_data_len) : (col += 1) {
        let bi: usize = 0;
        while (bi < num_blocks) : (bi += 1) {
            if (col < block_lens[bi]) {
                out_all[out_pos] = data_cw[block_starts[bi] + col];
                out_pos += 1;
            }
        }
    }

    // Interleave EC codewords
    let ecol: usize = 0;
    while (ecol < ec_len) : (ecol += 1) {
        let bi: usize = 0;
        while (bi < num_blocks) : (bi += 1) {
            out_all[out_pos] = ecc_blocks[bi][ecol];
            out_pos += 1;
        }
    }

    return out_pos;
}

/// Pure CP ISO/IEC 18004 QR Code Model 2 encoder (Versions 1..10, Byte mode).
pub static struct QrCode {
    /// Returns the maximum Byte-mode payload length in bytes for `version` (`1..=10`) and `ecc`.
    static fn maxByteCapacity(version: u8, ecc: QrEccLevel): usize {
        if (version < QR_MIN_VERSION || version > QR_MAX_VERSION) return 0;
        const spec = getSpec(version, ecc);
        const total_data = spec.totalDataCodewords();
        const header_bits: usize = if (version <= 9) 12 else 20;
        return (total_data * 8 - header_bits) / 8;
    }

    /// Selects the smallest QR Version (`1..=10`) fitting `data` at `ecc` and encodes it.
    static fn encode(alloc: std.mem.Allocator, data: []const u8, ecc: QrEccLevel): QrMatrix ! QrError {
        let v: u8 = QR_MIN_VERSION;
        while (v <= QR_MAX_VERSION) : (v += 1) {
            if (data.len <= QrCode.maxByteCapacity(v, ecc)) {
                return QrCode.encodeVersion(alloc, data, v, ecc);
            }
        }
        return QrError.DataTooLong;
    }

    /// Encodes `data` at an explicit QR Version (`1..=10`) and error correction level `ecc`.
    static fn encodeVersion(
        alloc: std.mem.Allocator,
        data: []const u8,
        version: u8,
        ecc: QrEccLevel,
    ): QrMatrix ! QrError {
        if (version < QR_MIN_VERSION || version > QR_MAX_VERSION) {
            return QrError.InvalidVersion;
        }
        if (data.len > QrCode.maxByteCapacity(version, ecc)) {
            return QrError.DataTooLong;
        }

        const spec = getSpec(version, ecc);
        const total_data = spec.totalDataCodewords();

        let data_cw: [MAX_CODEWORDS]u8 = [0; MAX_CODEWORDS];
        encodeDataCodewords(data, version, total_data, &data_cw);

        let interleaved: [MAX_CODEWORDS]u8 = [0; MAX_CODEWORDS];
        const total_cw = buildInterleavedCodewords(data_cw[0..total_data], spec, &interleaved);

        const size = 17 + 4 * usize(version);
        const mod_count = size * size;

        let base_modules: [MAX_MODULES]bool = [false; MAX_MODULES];
        let is_func: [MAX_MODULES]bool = [false; MAX_MODULES];
        drawFunctionPatterns(base_modules[0..mod_count], is_func[0..mod_count], version, size);
        placeCodewords(base_modules[0..mod_count], is_func[0..mod_count], size, interleaved[0..total_cw]);

        // Evaluate all 8 mask patterns and select the lowest-penalty mask
        let best_mask: u8 = 0;
        let best_penalty: usize = std.math.maxInt(usize);
        let candidate: [MAX_MODULES]bool = [false; MAX_MODULES];

        let mask_id: u8 = 0;
        while (mask_id < 8) : (mask_id += 1) {
            let y: usize = 0;
            while (y < size) : (y += 1) {
                let x: usize = 0;
                while (x < size) : (x += 1) {
                    const idx = y * size + x;
                    let val = base_modules[idx];
                    if (!is_func[idx] && maskMatches(mask_id, x, y)) {
                        val = !val;
                    }
                    candidate[idx] = val;
                }
            }
            writeFormatInfo(candidate[0..mod_count], size, ecc, mask_id);
            const p = evaluatePenalty(candidate[0..mod_count], size);
            if (p < best_penalty) {
                best_penalty = p;
                best_mask = mask_id;
            }
        }

        // Materialize final winning mask into heap-allocated QrMatrix
        let out_modules = alloc.alloc(bool, mod_count) catch return QrError.OutOfMemory;
        let y: usize = 0;
        while (y < size) : (y += 1) {
            let x: usize = 0;
            while (x < size) : (x += 1) {
                const idx = y * size + x;
                let val = base_modules[idx];
                if (!is_func[idx] && maskMatches(best_mask, x, y)) {
                    val = !val;
                }
                out_modules[idx] = val;
            }
        }
        writeFormatInfo(out_modules, size, ecc, best_mask);

        return QrMatrix{
            version: version,
            size: size,
            ecc: ecc,
            mask: best_mask,
            modules: out_modules,
            alloc: alloc,
        };
    }
}
