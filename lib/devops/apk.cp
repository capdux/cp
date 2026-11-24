// Origin: Pure-CP Stdlib (Zero-Alloc Android APK ZIP EOCD & Signing Block v2/v3 Parser) | Created: 2026-11-22

/// ZIP End of Central Directory (EOCD) 签名（小端序 "PK\x05\x06"）
pub const ZIP_EOCD_SIG: u32 = 0x06054B50;

/// ZIP EOCD 最小长度（不含注释）
pub const ZIP_EOCD_MIN_SIZE: usize = 22;

/// ZIP EOCD 注释最大长度
pub const ZIP_MAX_COMMENT_LEN: usize = 65535;

/// Android APK Signing Block v2/v3 尾部 16 字节魔数
pub const APK_SIG_BLOCK_MAGIC: []const u8 = "APK Sig Block 42";

/// APK Signature Scheme v2 Block ID
pub const APK_SIG_V2_BLOCK_ID: u32 = 0x7109871A;

/// APK Signature Scheme v3 Block ID
pub const APK_SIG_V3_BLOCK_ID: u32 = 0xF05368C0;

/// APK Signature Scheme v3.1 Block ID
pub const APK_SIG_V3_1_BLOCK_ID: u32 = 0x1B93AD61;

/// APK Verity Padding Block ID (4096-byte page alignment)
pub const APK_VERITY_PADDING_BLOCK_ID: u32 = 0x42726577;

/// APK Dependency Info Block ID
pub const APK_DEPENDENCY_INFO_BLOCK_ID: u32 = 0x504B4453;

/// ZIP End of Central Directory 定位结果
pub struct ApkZipEocd {
    eocd_offset: usize,
    cd_entries: u16,
    cd_size: u32,
    cd_offset: u32,
    comment_len: u16,
}

/// Android APK Signing Block 定位与零拷贝切片视图
pub struct ApkSigningBlock {
    block_offset: usize,
    block_size: u64,
    pairs_slice: []const u8,
    cd_offset: usize,
}

/// APK Signing Block 中的单个 ID-Value Pair（零拷贝切片）
pub struct ApkSigPair {
    id: u32,
    value: []const u8,
}

/// APK 签名块摘要信息
pub struct ApkSigSummary {
    has_v2: bool,
    has_v3: bool,
    has_v3_1: bool,
    has_verity_padding: bool,
    pair_count: u32,
}

// ---------------------------------------------------------------------------
// 内部小端序读取与字节比较辅助函数
// ---------------------------------------------------------------------------

fn readU16LE(data: []const u8, offset: usize): u16 {
    const lo = u16(data[offset]);
    const hi = u16(data[offset + 1]);
    return lo | (hi << 8);
}

fn readU32LE(data: []const u8, offset: usize): u32 {
    const b0 = u32(data[offset]);
    const b1 = u32(data[offset + 1]);
    const b2 = u32(data[offset + 2]);
    const b3 = u32(data[offset + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn readU64LE(data: []const u8, offset: usize): u64 {
    const lo = u64(readU32LE(data, offset));
    const hi = u64(readU32LE(data, offset + 4));
    return lo | (hi << 32);
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 1. ZIP EOCD 反向定位
// ---------------------------------------------------------------------------

/// 从文件尾反向搜索 ZIP EOCD（`PK\x05\x06`），校验 `comment_len` 精确到达 EOF 且 `cd_offset + cd_size <= eocd_offset`
pub fn findZipEocd(data: []const u8): ApkZipEocd? {
    if (data.len < ZIP_EOCD_MIN_SIZE) {
        return null;
    }
    const max_search = data.len - ZIP_EOCD_MIN_SIZE;
    const max_comment = if (max_search < ZIP_MAX_COMMENT_LEN) max_search else ZIP_MAX_COMMENT_LEN;
    const min_pos = max_search - max_comment;

    let pos = max_search;
    while (true) {
        if (readU32LE(data, pos) == ZIP_EOCD_SIG) {
            const comment_len = readU16LE(data, pos + 20);
            if (pos + ZIP_EOCD_MIN_SIZE + usize(comment_len) == data.len) {
                const cd_entries = readU16LE(data, pos + 10);
                const cd_size = readU32LE(data, pos + 12);
                const cd_offset = readU32LE(data, pos + 16);
                if (u64(cd_offset) + u64(cd_size) <= u64(pos)) {
                    return ApkZipEocd {
                        eocd_offset: pos,
                        cd_entries: cd_entries,
                        cd_size: cd_size,
                        cd_offset: cd_offset,
                        comment_len: comment_len,
                    };
                }
            }
        }
        if (pos == min_pos) {
            break;
        }
        pos -= 1;
    }
    return null;
}

// ---------------------------------------------------------------------------
// 2. APK Signing Block 定位与完整性校验
// ---------------------------------------------------------------------------

/// 定位并校验紧邻 Central Directory 之前的 Android APK Signing Block：
/// 1. 定位 ZIP EOCD 获取 `cd_offset`
/// 2. 校验 `cd_offset - 16 .. cd_offset` 为 `"APK Sig Block 42"`
/// 3. 读取底部 8B `size_of_block_bottom` 并防下溢/越界计算 `block_offset = cd_offset - (size + 8)`
/// 4. 校验顶部 8B `size_of_block_top == size_of_block_bottom`
/// 5. 返回零拷贝 `pairs_slice`（`block_offset + 8 .. cd_offset - 24`）
pub fn findApkSigningBlock(data: []const u8): ApkSigningBlock? {
    const eocd = findZipEocd(data) ?? return null;
    const cd_offset = usize(eocd.cd_offset);

    // 最小 Signing Block 为 32 字节（8B top + 0B pairs + 8B bottom + 16B magic）
    if (cd_offset < 32 || cd_offset > data.len) {
        return null;
    }

    // 校验 16 字节尾部魔数 "APK Sig Block 42"
    if (!eqlBytes(data[cd_offset - 16 .. cd_offset], APK_SIG_BLOCK_MAGIC)) {
        return null;
    }

    // 读取底部 8 字节 block size（不含顶部 8 字节，至少 24 字节）
    const size_bottom = readU64LE(data, cd_offset - 24);
    if (size_bottom < 24) {
        return null;
    }

    // 防溢出与下溢校验：size_bottom + 8 <= cd_offset
    if (size_bottom > u64(cd_offset) - 8) {
        return null;
    }

    const total_size = usize(size_bottom + 8);
    const block_offset = cd_offset - total_size;

    // 校验顶部 8 字节 block size 与底部严格相等
    const size_top = readU64LE(data, block_offset);
    if (size_top != size_bottom) {
        return null;
    }

    const pairs_start = block_offset + 8;
    const pairs_end = cd_offset - 24;
    return ApkSigningBlock {
        block_offset: block_offset,
        block_size: size_bottom,
        pairs_slice: data[pairs_start..pairs_end],
        cd_offset: cd_offset,
    };
}

// ---------------------------------------------------------------------------
// 3. ID-Value Pair 零分配迭代器、查询与汇总
// ---------------------------------------------------------------------------

/// APK Signing Block ID-Value Pair 零分配迭代器
pub struct ApkSigPairIter {
    pairs: []const u8,
    pos: usize,
    malformed: bool,

    pub static fn init(block: ApkSigningBlock): ApkSigPairIter {
        return ApkSigPairIter {
            pairs: block.pairs_slice,
            pos: 0,
            malformed: false,
        };
    }

    pub fn next(*_): ApkSigPair? {
        if (_.pos >= _.pairs.len) {
            return null;
        }
        const rem = _.pairs.len - _.pos;
        if (rem < 8) {
            _.malformed = true;
            _.pos = _.pairs.len;
            return null;
        }
        const pair_len = readU64LE(_.pairs, _.pos);
        // 每个 pair 至少 4 字节 ID，且不能超过剩余字节数 (rem - 8)
        if (pair_len < 4 || pair_len > u64(rem - 8)) {
            _.malformed = true;
            _.pos = _.pairs.len;
            return null;
        }
        const id = readU32LE(_.pairs, _.pos + 8);
        const val_start = _.pos + 12;
        const val_len = usize(pair_len - 4);
        const val_end = val_start + val_len;
        _.pos = val_end;
        return ApkSigPair {
            id: id,
            value: _.pairs[val_start..val_end],
        };
    }
}

/// 按签名块 ID 零分配查找对应的 value 切片
pub fn findApkSigPair(block: ApkSigningBlock, id: u32): ([]const u8)? {
    let it = ApkSigPairIter.init(block);
    while (pair := it.next()) {
        if (pair.id == id) {
            return pair.value;
        }
    }
    return null;
}

/// 扫描 APK Signing Block 并汇总 v2/v3/v3.1/Verity Padding 存在性与总 pair 数；
/// 若不存在合法签名块或内部 pair 长度越界畸形则返回 `null`
pub fn summarizeApkSigning(data: []const u8): ApkSigSummary? {
    const block = findApkSigningBlock(data) ?? return null;
    let it = ApkSigPairIter.init(block);

    let has_v2 = false;
    let has_v3 = false;
    let has_v3_1 = false;
    let has_verity_padding = false;
    let pair_count: u32 = 0;

    while (pair := it.next()) {
        if (pair.id == APK_SIG_V2_BLOCK_ID) {
            has_v2 = true;
        } else if (pair.id == APK_SIG_V3_BLOCK_ID) {
            has_v3 = true;
        } else if (pair.id == APK_SIG_V3_1_BLOCK_ID) {
            has_v3_1 = true;
        } else if (pair.id == APK_VERITY_PADDING_BLOCK_ID) {
            has_verity_padding = true;
        }
        pair_count += 1;
    }

    if (it.malformed) {
        return null;
    }

    return ApkSigSummary {
        has_v2: has_v2,
        has_v3: has_v3,
        has_v3_1: has_v3_1,
        has_verity_padding: has_verity_padding,
        pair_count: pair_count,
    };
}
