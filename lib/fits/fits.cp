// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc NASA FITS 2880B HDU & 80B Card Parser)

/// FITS 逻辑块固定大小（2880 字节）
pub const FITS_BLOCK_SIZE: usize = 2880;
/// FITS 单张 Header Keyword Card 固定大小（80 字节）
pub const FITS_CARD_SIZE: usize = 80;
/// 每个 2880B 块包含的 Card 数量（36 张）
pub const FITS_CARDS_PER_BLOCK: usize = 36;

/// FITS 80 字节 Keyword Card（所有切片零拷贝指向原始输入）
pub struct FitsCard {
    keyword: []const u8,
    has_value: bool,
    value: []const u8,
    comment: ([]const u8)?,
}

/// FITS Primary HDU 核心头摘要
pub struct FitsHeader {
    simple: bool,
    bitpix: i32,
    naxis: u32,
    naxis1: u64,
    naxis2: u64,
    naxis3: u64,
    header_bytes: usize,
    data_bytes: u64,
    card_count: u32,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn trimRightSpace(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && s[end - 1] == ' ') {
        end -= 1;
    }
    return s[0..end];
}

/// 返回 FITS `BITPIX` 对应的每像素字节数（`8->1, 16->2, 32->4, 64->8, -32->4, -64->8`）
pub fn fitsBytesPerPixel(bitpix: i32): u8? {
    if (bitpix == 8) return 1;
    if (bitpix == 16) return 2;
    if (bitpix == 32 || bitpix == -32) return 4;
    if (bitpix == 64 || bitpix == -64) return 8;
    return null;
}

/// 解析 FITS 布尔值（`"T"` -> `true`, `"F"` -> `false`）
pub fn parseFitsBool(val: []const u8): bool? {
    const t = trimAscii(val);
    if (t.len == 1 && t[0] == 'T') return true;
    if (t.len == 1 && t[0] == 'F') return false;
    return null;
}

/// 解析 FITS 有符号十进制整数（支持前导 `+`/`-` 与溢出校验）
pub fn parseFitsInt(val: []const u8): i64? {
    const t = trimAscii(val);
    if (t.len == 0) return null;

    let i: usize = 0;
    let neg = false;
    if (t[0] == '-') {
        neg = true;
        i = 1;
    } else if (t[0] == '+') {
        i = 1;
    }
    if (i >= t.len) return null;

    let acc: u64 = 0;
    const limit: u64 = if (neg) 9223372036854775808 else 9223372036854775807;
    while (i < t.len) {
        if (!isDigit(t[i])) return null;
        const d = u64(t[i] - '0');
        if (acc > (limit - d) / 10) {
            return null;
        }
        acc = acc * 10 + d;
        i += 1;
    }

    if (neg) {
        if (acc == 9223372036854775808) {
            return -9223372036854775807 - 1;
        }
        const signed_v = i64(acc);
        return -signed_v;
    }
    return i64(acc);
}

/// 快速校验是否为合法 FITS 文件（长度 >= 2880 且首张 Card 以 `"SIMPLE  ="` 开头）
pub fn isFits(data: []const u8): bool {
    if (data.len < FITS_BLOCK_SIZE) {
        return false;
    }
    return eqlBytes(data[0..9], "SIMPLE  =");
}

fn parseSingleCard(raw: []const u8): FitsCard {
    const keyword = trimRightSpace(raw[0..8]);
    const has_value = raw[8] == '=' && raw[9] == ' ';

    if (!has_value) {
        const rest = trimAscii(raw[8..80]);
        return FitsCard {
            keyword: keyword,
            has_value: false,
            value: "",
            comment: if (rest.len > 0) rest else null,
        };
    }

    // 扫描 raw[10..80]，识别单引号字符串（`''` 为转义）与 `/` 注释分隔符
    const field = raw[10..80];
    let i: usize = 0;
    while (i < field.len && field[i] == ' ') {
        i += 1;
    }

    let slash_pos: usize? = null;
    if (i < field.len && field[i] == '\'') {
        i += 1;
        while (i < field.len) {
            if (field[i] == '\'') {
                if (i + 1 < field.len && field[i + 1] == '\'') {
                    i += 2;
                    continue;
                }
                i += 1;
                break;
            }
            i += 1;
        }
    }

    while (i < field.len) {
        if (field[i] == '/') {
            slash_pos = i;
            break;
        }
        i += 1;
    }

    if (sp := slash_pos) {
        const val_slice = trimAscii(field[0..sp]);
        const com_slice = trimAscii(field[sp + 1 ..]);
        return FitsCard {
            keyword: keyword,
            has_value: true,
            value: val_slice,
            comment: if (com_slice.len > 0) com_slice else null,
        };
    }

    return FitsCard {
        keyword: keyword,
        has_value: true,
        value: trimAscii(field),
        comment: null,
    };
}

/// 零堆分配 FITS 80 字节 Card 流式迭代器（遇 `"END"` 卡片自动停止）
pub struct FitsCardIter {
    data: []const u8,
    pos: usize = 0,
    done: bool = false,

    static fn init(data: []const u8): FitsCardIter {
        return FitsCardIter {
            data: data,
            pos: 0,
            done: false,
        };
    }

    /// 读取下一张 `FitsCard`；遇到 `"END"` 或不足 80 字节返回 `null`
    fn next(*_): FitsCard? {
        if (_.done || _.pos + FITS_CARD_SIZE > _.data.len) {
            _.done = true;
            return null;
        }
        const raw = _.data[_.pos .. _.pos + FITS_CARD_SIZE];
        _.pos += FITS_CARD_SIZE;

        const card = parseSingleCard(raw);
        if (!card.has_value && eqlBytes(card.keyword, "END")) {
            _.done = true;
            return null;
        }
        return card;
    }
}

/// 在 FITS 头中按关键字查找首张匹配的 `FitsCard`（零堆分配）
pub fn findFitsCard(data: []const u8, keyword: []const u8): FitsCard? {
    let it = FitsCardIter.init(data);
    while (card := it.next()) {
        if (eqlBytes(card.keyword, keyword)) {
            return card;
        }
    }
    return null;
}

/// 解析 FITS Primary HDU 核心头（校验 `SIMPLE`、`BITPIX`、`NAXIS`、`END` 及 2880B 头块对齐边界）
pub fn parseFitsHeader(data: []const u8): FitsHeader? {
    if (!isFits(data)) {
        return null;
    }

    let pos: usize = 0;
    let found_end = false;
    let end_after_pos: usize = 0;

    let has_simple = false;
    let simple = false;
    let has_bitpix = false;
    let bitpix: i32 = 0;
    let has_naxis = false;
    let naxis: u32 = 0;

    let naxis1: u64 = 0;
    let naxis2: u64 = 0;
    let naxis3: u64 = 0;
    let has_naxis1 = false;
    let has_naxis2 = false;
    let has_naxis3 = false;

    let card_count: u32 = 0;
    let extra_axis_prod: u64 = 1;
    const max_u64: u64 = 0xFFFF_FFFF_FFFF_FFFF;

    while (pos + FITS_CARD_SIZE <= data.len) {
        const raw = data[pos .. pos + FITS_CARD_SIZE];
        pos += FITS_CARD_SIZE;
        const card = parseSingleCard(raw);

        if (!card.has_value && eqlBytes(card.keyword, "END")) {
            found_end = true;
            end_after_pos = pos;
            break;
        }

        card_count += 1;
        if (!card.has_value) {
            continue;
        }

        if (eqlBytes(card.keyword, "SIMPLE")) {
            if (b := parseFitsBool(card.value)) {
                simple = b;
                has_simple = true;
            } else {
                return null;
            }
        } else if (eqlBytes(card.keyword, "BITPIX")) {
            if (v := parseFitsInt(card.value)) {
                if (v < -64 || v > 64) return null;
                const bp = i32(v);
                if (fitsBytesPerPixel(bp) == null) return null;
                bitpix = bp;
                has_bitpix = true;
            } else {
                return null;
            }
        } else if (eqlBytes(card.keyword, "NAXIS")) {
            if (v := parseFitsInt(card.value)) {
                if (v < 0 || v > 999) return null;
                naxis = u32(v);
                has_naxis = true;
            } else {
                return null;
            }
        } else if (eqlBytes(card.keyword, "NAXIS1")) {
            if (v := parseFitsInt(card.value)) {
                if (v < 0) return null;
                naxis1 = u64(v);
                has_naxis1 = true;
            } else {
                return null;
            }
        } else if (eqlBytes(card.keyword, "NAXIS2")) {
            if (v := parseFitsInt(card.value)) {
                if (v < 0) return null;
                naxis2 = u64(v);
                has_naxis2 = true;
            } else {
                return null;
            }
        } else if (eqlBytes(card.keyword, "NAXIS3")) {
            if (v := parseFitsInt(card.value)) {
                if (v < 0) return null;
                naxis3 = u64(v);
                has_naxis3 = true;
            } else {
                return null;
            }
        } else if (card.keyword.len > 5 && eqlBytes(card.keyword[0..5], "NAXIS")) {
            if (v := parseFitsInt(card.value)) {
                if (v < 0) return null;
                const dim = u64(v);
                if (dim == 0) {
                    extra_axis_prod = 0;
                } else if (extra_axis_prod > max_u64 / dim) {
                    return null;
                } else {
                    extra_axis_prod *= dim;
                }
            } else {
                return null;
            }
        }
    }

    if (!found_end || !has_simple || !has_bitpix || !has_naxis) {
        return null;
    }

    // 按 2880 字节块对齐头长度
    const blocks = (end_after_pos + FITS_BLOCK_SIZE - 1) / FITS_BLOCK_SIZE;
    const header_bytes = blocks * FITS_BLOCK_SIZE;
    if (header_bytes > data.len) {
        return null;
    }

    let bpp: u64 = 0;
    if (b := fitsBytesPerPixel(bitpix)) {
        bpp = u64(b);
    } else {
        return null;
    }

    let data_bytes: u64 = 0;
    if (naxis > 0) {
        if (naxis >= 1 && !has_naxis1) return null;
        if (naxis >= 2 && !has_naxis2) return null;
        if (naxis >= 3 && !has_naxis3) return null;

        let pixels: u64 = naxis1;
        if (naxis >= 2) {
            if (naxis2 == 0) {
                pixels = 0;
            } else if (pixels > max_u64 / naxis2) {
                return null;
            } else {
                pixels *= naxis2;
            }
        }
        if (naxis >= 3) {
            if (naxis3 == 0) {
                pixels = 0;
            } else if (pixels > max_u64 / naxis3) {
                return null;
            } else {
                pixels *= naxis3;
            }
        }
        if (naxis > 3) {
            if (extra_axis_prod == 0) {
                pixels = 0;
            } else if (pixels > max_u64 / extra_axis_prod) {
                return null;
            } else {
                pixels *= extra_axis_prod;
            }
        }
        if (pixels > max_u64 / bpp) {
            return null;
        }
        data_bytes = pixels * bpp;
    }

    return FitsHeader {
        simple: simple,
        bitpix: bitpix,
        naxis: naxis,
        naxis1: naxis1,
        naxis2: naxis2,
        naxis3: naxis3,
        header_bytes: header_bytes,
        data_bytes: data_bytes,
        card_count: card_count,
    };
}
