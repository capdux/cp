// Origin: Pure-CP Stdlib (Zero-Alloc JPEG XL Codestream & Container Parser) | Created: 2026-11-22

/// JPEG XL 封装形态
pub enum JxlContainerKind {
    codestream,
    container,
    unknown,
}

/// JPEG XL 裸码流 `SizeHeader` 解码结果（ISO/IEC 18181-1）
pub struct JxlSizeHeader {
    width: u32,
    height: u32,
    is_small: bool,
    ratio: u8,
    bits_read: u32,
}

/// ISOBMFF 容器单个 Box 零拷贝视图
pub struct JxlBox {
    box_type: [4]u8,
    payload: []const u8,
    offset: usize,
    total_size: usize,
}

/// JPEG XL 统一解析摘要（自动兼容裸码流与 ISOBMFF 容器）
pub struct JxlInfo {
    kind: JxlContainerKind,
    width: u32,
    height: u32,
    has_exif: bool,
    has_xmp: bool,
    is_partial: bool,
}

fn readU32Be(data: []const u8, pos: usize): u32 {
    return (u32(data[pos]) << 24)
        | (u32(data[pos + 1]) << 16)
        | (u32(data[pos + 2]) << 8)
        | u32(data[pos + 3]);
}

fn readU64Be(data: []const u8, pos: usize): u64 {
    const hi = u64(readU32Be(data, pos));
    const lo = u64(readU32Be(data, pos + 4));
    return (hi << 32) | lo;
}

fn fourccEq(tag: [4]u8, s: []const u8): bool {
    if (s.len != 4) return false;
    return tag[0] == s[0] && tag[1] == s[1] && tag[2] == s[2] && tag[3] == s[3];
}

/// 校验 2 字节 JPEG XL 裸码流签名 `0xFF 0x0A`
pub fn isJxlCodestream(data: []const u8): bool {
    return data.len >= 2 && data[0] == 0xFF && data[1] == 0x0A;
}

/// 校验 12 字节 JPEG XL ISOBMFF 容器签名 `00 00 00 0C 4A 58 4C 20 0D 0A 87 0A`
pub fn isJxlContainer(data: []const u8): bool {
    if (data.len < 12) return false;
    return data[0] == 0x00
        && data[1] == 0x00
        && data[2] == 0x00
        && data[3] == 0x0C
        && data[4] == 0x4A
        && data[5] == 0x58
        && data[6] == 0x4C
        && data[7] == 0x20
        && data[8] == 0x0D
        && data[9] == 0x0A
        && data[10] == 0x87
        && data[11] == 0x0A;
}

/// 检测 JPEG XL 封装形态（`codestream` / `container` / `unknown`）
pub fn detectJxlKind(data: []const u8): JxlContainerKind {
    if (isJxlCodestream(data)) return JxlContainerKind.codestream;
    if (isJxlContainer(data)) return JxlContainerKind.container;
    return JxlContainerKind.unknown;
}

/// 快速校验输入是否为合法 JPEG XL 裸码流或容器
pub fn isJxl(data: []const u8): bool {
    return isJxlCodestream(data) || isJxlContainer(data);
}

struct LsbBitReader {
    data: []const u8,
    bit_pos: usize = 0,

    fn readBits(*_, n: u32): u32? {
        if (n == 0) return 0;
        if (n > 32) return null;
        const total_bits = _.data.len * 8;
        if (_.bit_pos > total_bits || usize(n) > total_bits - _.bit_pos) {
            return null;
        }
        let acc: u32 = 0;
        let i: u32 = 0;
        while (i < n) {
            const byte_idx = _.bit_pos / 8;
            const shift = u3(_.bit_pos % 8);
            const bit = u32((_.data[byte_idx] >> shift) & 1);
            acc |= bit << u5(i);
            _.bit_pos += 1;
            i += 1;
        }
        return acc;
    }
}

fn readLargeDim(br: *LsbBitReader): u32? {
    let sel: u32 = 0;
    if (s := br.readBits(2)) {
        sel = s;
    } else {
        return null;
    }
    let bit_width: u32 = 9;
    if (sel == 1) {
        bit_width = 13;
    } else if (sel == 2) {
        bit_width = 18;
    } else if (sel == 3) {
        bit_width = 30;
    }
    if (v := br.readBits(bit_width)) {
        return v + 1;
    }
    return null;
}

fn widthFromRatio(height: u32, ratio: u8): u32? {
    const h = u64(height);
    let w: u64 = 0;
    if (ratio == 1) {
        w = h;
    } else if (ratio == 2) {
        w = (h * 12) / 10;
    } else if (ratio == 3) {
        w = (h * 4) / 3;
    } else if (ratio == 4) {
        w = (h * 3) / 2;
    } else if (ratio == 5) {
        w = (h * 16) / 9;
    } else if (ratio == 6) {
        w = (h * 5) / 4;
    } else if (ratio == 7) {
        w = h * 2;
    } else {
        return null;
    }
    if (w == 0 || w > 0xFFFF_FFFF) {
        return null;
    }
    return u32(w);
}

/// 从 JPEG XL 裸码流（以 `0xFF 0x0A` 开头）解码 LSB-first `SizeHeader`
pub fn parseJxlSizeHeader(codestream: []const u8): JxlSizeHeader? {
    if (!isJxlCodestream(codestream) || codestream.len < 3) {
        return null;
    }
    let br = LsbBitReader {
        data: codestream[2..],
        bit_pos: 0,
    };

    let small_bit: u32 = 0;
    if (sb := br.readBits(1)) {
        small_bit = sb;
    } else {
        return null;
    }
    const is_small = small_bit == 1;

    let height: u32 = 0;
    if (is_small) {
        if (v := br.readBits(5)) {
            height = (v + 1) * 8;
        } else {
            return null;
        }
    } else {
        if (h := readLargeDim(*br)) {
            height = h;
        } else {
            return null;
        }
    }

    let ratio: u8 = 0;
    if (r := br.readBits(3)) {
        ratio = u8(r);
    } else {
        return null;
    }

    let width: u32 = 0;
    if (ratio == 0) {
        if (is_small) {
            if (v := br.readBits(5)) {
                width = (v + 1) * 8;
            } else {
                return null;
            }
        } else {
            if (w := readLargeDim(*br)) {
                width = w;
            } else {
                return null;
            }
        }
    } else {
        if (w := widthFromRatio(height, ratio)) {
            width = w;
        } else {
            return null;
        }
    }

    return JxlSizeHeader {
        width: width,
        height: height,
        is_small: is_small,
        ratio: ratio,
        bits_read: u32(br.bit_pos),
    };
}

/// 零堆分配 JPEG XL ISOBMFF Box 迭代器
pub struct JxlBoxIter {
    data: []const u8,
    pos: usize = 0,
    failed: bool = false,

    static fn init(data: []const u8): JxlBoxIter {
        return JxlBoxIter {
            data: data,
            pos: 0,
            failed: false,
        };
    }

    /// 读取下一个合法 ISOBMFF Box；到达末尾返回 `null`，遇到畸形越界 Box 置 `failed = true` 并返回 `null`
    fn next(*_): JxlBox? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < 8) {
            _.failed = true;
            _.pos = _.data.len;
            return null;
        }

        const start = _.pos;
        const size32 = readU32Be(_.data, start);
        const tag: [4]u8 = [
            _.data[start + 4],
            _.data[start + 5],
            _.data[start + 6],
            _.data[start + 7],
        ];

        let header_len: usize = 8;
        let total_size: usize = 0;
        const rem = _.data.len - start;

        if (size32 == 0) {
            total_size = rem;
        } else if (size32 == 1) {
            if (rem < 16) {
                _.failed = true;
                _.pos = _.data.len;
                return null;
            }
            const size64 = readU64Be(_.data, start + 8);
            if (size64 < 16 || size64 > u64(rem)) {
                _.failed = true;
                _.pos = _.data.len;
                return null;
            }
            header_len = 16;
            total_size = usize(size64);
        } else if (size32 < 8) {
            _.failed = true;
            _.pos = _.data.len;
            return null;
        } else {
            if (usize(size32) > rem) {
                _.failed = true;
                _.pos = _.data.len;
                return null;
            }
            total_size = usize(size32);
        }

        const payload = _.data[start + header_len .. start + total_size];
        _.pos = start + total_size;

        return JxlBox {
            box_type: tag,
            payload: payload,
            offset: start,
            total_size: total_size,
        };
    }
}

/// 统一 JPEG XL 信息解析入口（自动兼容裸码流与 ISOBMFF 容器 `jxlc`/`jxlp`/`Exif`/`xml `/`brob`）
pub fn parseJxlInfo(data: []const u8): JxlInfo? {
    if (isJxlCodestream(data)) {
        if (sz := parseJxlSizeHeader(data)) {
            return JxlInfo {
                kind: JxlContainerKind.codestream,
                width: sz.width,
                height: sz.height,
                has_exif: false,
                has_xmp: false,
                is_partial: false,
            };
        }
        return null;
    }

    if (!isJxlContainer(data)) {
        return null;
    }

    let it = JxlBoxIter.init(data);
    let has_size = false;
    let width: u32 = 0;
    let height: u32 = 0;
    let has_exif = false;
    let has_xmp = false;
    let is_partial = false;

    while (box := it.next()) {
        if (fourccEq(box.box_type, "jxlc")) {
            if (!has_size) {
                if (sz := parseJxlSizeHeader(box.payload)) {
                    width = sz.width;
                    height = sz.height;
                    has_size = true;
                } else {
                    return null;
                }
            }
        } else if (fourccEq(box.box_type, "jxlp")) {
            is_partial = true;
            if (box.payload.len >= 4) {
                const idx = readU32Be(box.payload, 0) & 0x7FFF_FFFF;
                if (idx == 0 && !has_size) {
                    if (sz := parseJxlSizeHeader(box.payload[4..])) {
                        width = sz.width;
                        height = sz.height;
                        has_size = true;
                    } else {
                        return null;
                    }
                }
            } else {
                return null;
            }
        } else if (fourccEq(box.box_type, "Exif")) {
            has_exif = true;
        } else if (fourccEq(box.box_type, "xml ")) {
            has_xmp = true;
        } else if (fourccEq(box.box_type, "brob") && box.payload.len >= 4) {
            const inner: [4]u8 = [box.payload[0], box.payload[1], box.payload[2], box.payload[3]];
            if (fourccEq(inner, "Exif")) {
                has_exif = true;
            } else if (fourccEq(inner, "xml ")) {
                has_xmp = true;
            }
        }
    }

    if (it.failed || !has_size) {
        return null;
    }

    return JxlInfo {
        kind: JxlContainerKind.container,
        width: width,
        height: height,
        has_exif: has_exif,
        has_xmp: has_xmp,
        is_partial: is_partial,
    };
}
