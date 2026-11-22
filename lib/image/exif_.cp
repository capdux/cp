// Reference Only: Functional overlap with lib/image/exif.cp | Kept for comparison/analysis
// Source: Absorbed from /data/dev/bun/src/runtime/image/exif.cp | Synced: 2026-11-21T12:31:33Z
//! EXIF Orientation reader (JPEG APP1/Exif + raw TIFF IFD0 tag 0x0112).
//!
//! Layout:
//!   JPEG = FF D8 (SOI) · marker* · FF DA (SOS) · scan · FF D9 (EOI)
//!   marker = FF xx · be16 length · payload[length-2]
//!   APP1/Exif payload = "Exif\0\0" · TIFF
//!   TIFF = byte-order ("II"=LE | "MM"=BE) · u16 magic 42 · u32 IFD0-offset
//!   IFD0 = u16 entry-count · entry[count] · u32 next-IFD-offset
//!   entry = u16 tag · u16 type · u32 count · u32 value-or-offset
//!
//! Permissive by design: any malformation returns `null` / `.normal`.

export struct Transform {
    flop: bool,
    flip: bool,
    rotate: u16,

    pub fn isIdentity(): bool {
        return !_.flop && !_.flip && _.rotate == 0;
    }

    pub fn swapsDimensions(): bool {
        return _.rotate == 90 || _.rotate == 270;
    }
}

export enum Orientation: u8 {
    normal = 1,
    flop = 2, // mirror horizontal
    rotate180 = 3,
    flip = 4, // mirror vertical
    flop_rotate90 = 5,
    rotate90 = 6,
    flop_rotate270 = 7,
    rotate270 = 8,

    /// The (mirror?, cw-degrees) transform that turns the stored pixels upright.
    pub fn transform(): Transform {
        return switch (_) {
            .normal => Transform{ flop: false, flip: false, rotate: 0 },
            .flop => Transform{ flop: true, flip: false, rotate: 0 },
            .rotate180 => Transform{ flop: false, flip: false, rotate: 180 },
            .flip => Transform{ flop: false, flip: true, rotate: 0 },
            // 5 = transpose, 7 = transverse. With flop applied BEFORE a CW
            // rotate, flop∘rot270 = transpose and flop∘rot90 = transverse.
            .flop_rotate90 => Transform{ flop: true, flip: false, rotate: 270 },
            .rotate90 => Transform{ flop: false, flip: false, rotate: 90 },
            .flop_rotate270 => Transform{ flop: true, flip: false, rotate: 90 },
            .rotate270 => Transform{ flop: false, flip: false, rotate: 270 },
        };
    }

    pub fn toU8(): u8 {
        return @intFromEnum(_);
    }

    pub fn swapsDimensions(): bool {
        return _.transform().swapsDimensions();
    }

    pub static fn fromU16(v: u16): Orientation? {
        if (v < 1 || v > 8) return null;
        return @enumFromInt(@as(u8, @intCast(v)));
    }
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

inline fn rd16(b: []const u8, off: usize, big: bool): u16? {
    if (off > b.len || b.len - off < 2) return null;
    if (big) {
        return (@as(u16, b[off]) << 8) | @as(u16, b[off + 1]);
    }
    return @as(u16, b[off]) | (@as(u16, b[off + 1]) << 8);
}

inline fn rd32(b: []const u8, off: usize, big: bool): u32? {
    if (off > b.len || b.len - off < 4) return null;
    if (big) {
        return (@as(u32, b[off]) << 24)
            | (@as(u32, b[off + 1]) << 16)
            | (@as(u32, b[off + 2]) << 8)
            | @as(u32, b[off + 3]);
    }
    return @as(u32, b[off])
        | (@as(u32, b[off + 1]) << 8)
        | (@as(u32, b[off + 2]) << 16)
        | (@as(u32, b[off + 3]) << 24);
}

/// Parse a raw TIFF payload (starting with "II" or "MM") and extract IFD0 Orientation tag (0x0112).
export fn parseTiff(tiff: []const u8): Orientation? {
    if (tiff.len < 8) return null;
    const big = tiff[0] == 'M' && tiff[1] == 'M';
    const little = tiff[0] == 'I' && tiff[1] == 'I';
    if (!big && !little) return null;

    const magic_opt = rd16(tiff, 2, big);
    if (magic_opt == null || magic_opt.? != 42) return null;

    const ifd0_opt = rd32(tiff, 4, big);
    if (ifd0_opt == null) return null;
    const ifd0: usize = @intCast(ifd0_opt.?);

    const count_opt = rd16(tiff, ifd0, big);
    if (count_opt == null) return null;
    const count = count_opt.?;

    let e: usize = ifd0 + 2;
    let n: u16 = 0;
    while (n < count) : (n += 1) {
        const tag_opt = rd16(tiff, e, big);
        if (tag_opt == null) return null;
        if (tag_opt.? == 0x0112) {
            const ty_opt = rd16(tiff, e + 2, big);
            const cnt_opt = rd32(tiff, e + 4, big);
            if (ty_opt == null || cnt_opt == null) return null;
            if (ty_opt.? != 3 || cnt_opt.? != 1) return null;
            const v_opt = rd16(tiff, e + 8, big);
            if (v_opt == null) return null;
            return Orientation.fromU16(v_opt.?);
        }
        if (tiff.len - e < 12) return null;
        e += 12;
    }
    return null;
}

/// Walk JPEG markers up to SOS/EOI looking for an APP1/Exif segment,
/// then parse IFD0 tag 0x0112. Returns `.normal` on missing/malformed EXIF.
export fn readJpeg(bytes: []const u8): Orientation {
    if (bytes.len < 4 || bytes[0] != 0xFF || bytes[1] != 0xD8) return .normal;
    const exif_hdr: [6]u8 = ['E', 'x', 'i', 'f', 0, 0];
    let i: usize = 2;
    while (i + 4 <= bytes.len) {
        if (bytes[i] != 0xFF) return .normal;
        const marker = bytes[i + 1];
        if (marker == 0xFF) {
            i += 1;
            continue;
        }
        if (marker >= 0xD0 && marker <= 0xD8) {
            i += 2;
            continue;
        }
        if (marker == 0xDA || marker == 0xD9) {
            return .normal;
        }
        const seglen = (@as(usize, bytes[i + 2]) << 8) | @as(usize, bytes[i + 3]);
        if (seglen < 2 || i + 2 + seglen > bytes.len) return .normal;
        if (marker == 0xE1 && seglen >= 8) {
            const seg = bytes[i + 4 .. i + 2 + seglen];
            if (seg.len >= 6 && eqlBytes(seg[0..6], exif_hdr[0..6])) {
                return parseTiff(seg[6..]) ?? .normal;
            }
        }
        i += 2 + seglen;
    }
    return .normal;
}
