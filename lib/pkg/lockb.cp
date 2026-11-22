// Source: Absorbed from /data/dev/bun/src/install/lockfile/bun.lockb.cp | Synced: 2026-11-22T14:42:30Z
// Pure-CP zero-allocation Bun binary lockfile (`bun.lockb`) wire header codec,
// format v2->v3 migration validator, 0xDEADBEEF array sentinel reader/writer,
// and 8-byte trailing extension section inspector (aligned with Bun `bun.lockb.cp` & `docs/pm/lockfile.mdx`).

/// `bun.lockb` 固定 42 字节 Shebang + 版本魔数头部
pub const LOCKB_SHEBANG_HEADER: []const u8 = "#!/usr/bin/env bun\nbun-lockfile-format-v0\n";
/// `meta_hash` 固定长度（32 字节 SHA/Wyhash 摘要）
pub const LOCKB_META_HASH_LEN: usize = 32;
/// 固定文件头长度：42B magic + 4B format + 32B meta_hash + 8B total_buffer_size = 86B
pub const LOCKB_FIXED_HEADER_LEN: usize = 86;

/// 支持自动迁移的历史二进制格式版本（v2）
pub const LOCKB_FORMAT_V2: u32 = 2;
/// 当前标准二进制格式版本（v3）
pub const LOCKB_FORMAT_CURRENT: u32 = 3;

/// `Lockfile.Buffers` 未写入数组的 64 位哨兵偏移（对齐 `Buffers.cp`）
pub const LOCKB_DEAD_SENTINEL: u64 = 0xDEADBEEF;

/// 尾部扩展段最大扫描条目数
pub const LOCKB_MAX_SECTIONS: usize = 8;

/// 7 大 8 字节尾部扩展段 ASCII 魔数标签（little-endian `u64`）
pub const LOCKB_TAG_WORKSPACES: u64 = 0x436150734B724F77;       // "wOrKsPaC"
pub const LOCKB_TAG_TRUSTED_DEPS: u64 = 0x6444457453755274;     // "tRuStEDd"
pub const LOCKB_TAG_EMPTY_TRUSTED: u64 = 0x5473557254704D65;    // "eMpTrUsT"
pub const LOCKB_TAG_OVERRIDES: u64 = 0x734469725265566F;        // "oVeRriDs"
pub const LOCKB_TAG_PATCHED_DEPS: u64 = 0x4464456843744170;     // "pAtChEdD"
pub const LOCKB_TAG_CATALOGS: u64 = 0x53674F6C41744163;         // "cAtAlOgS"
pub const LOCKB_TAG_CONFIG_VERSION: u64 = 0x4E73527647664E63;   // "cNfGvRsN"

/// 扩展段存在性位掩码
pub const LOCKB_EXT_WORKSPACES: u16 = 1 << 0;
pub const LOCKB_EXT_TRUSTED_DEPS: u16 = 1 << 1;
pub const LOCKB_EXT_EMPTY_TRUSTED: u16 = 1 << 2;
pub const LOCKB_EXT_OVERRIDES: u16 = 1 << 3;
pub const LOCKB_EXT_PATCHED_DEPS: u16 = 1 << 4;
pub const LOCKB_EXT_CATALOGS: u16 = 1 << 5;
pub const LOCKB_EXT_CONFIG_VERSION: u16 = 1 << 6;

/// `bun.lockb` 编解码与校验错误集
pub err LockbError {
    InvalidMagic,
    TruncatedHeader,
    UnexpectedVersion,
    OutdatedVersion,
    CorruptTerminator,
    TruncatedSection,
    InvalidArrayRange,
    BufferTooSmall,
    TooManySections,
}

/// 尾部扩展段标签类别
pub enum LockbTagKind: u8 {
    Workspaces = 0,
    TrustedDeps = 1,
    EmptyTrustedDeps = 2,
    Overrides = 3,
    PatchedDeps = 4,
    Catalogs = 5,
    ConfigVersion = 6,
    Unknown = 7,

    static fn fromU64(raw: u64): LockbTagKind {
        if (raw == LOCKB_TAG_WORKSPACES) return .Workspaces;
        if (raw == LOCKB_TAG_TRUSTED_DEPS) return .TrustedDeps;
        if (raw == LOCKB_TAG_EMPTY_TRUSTED) return .EmptyTrustedDeps;
        if (raw == LOCKB_TAG_OVERRIDES) return .Overrides;
        if (raw == LOCKB_TAG_PATCHED_DEPS) return .PatchedDeps;
        if (raw == LOCKB_TAG_CATALOGS) return .Catalogs;
        if (raw == LOCKB_TAG_CONFIG_VERSION) return .ConfigVersion;
        return .Unknown;
    }

    fn toU64(): u64 {
        return switch (_) {
            .Workspaces => LOCKB_TAG_WORKSPACES,
            .TrustedDeps => LOCKB_TAG_TRUSTED_DEPS,
            .EmptyTrustedDeps => LOCKB_TAG_EMPTY_TRUSTED,
            .Overrides => LOCKB_TAG_OVERRIDES,
            .PatchedDeps => LOCKB_TAG_PATCHED_DEPS,
            .Catalogs => LOCKB_TAG_CATALOGS,
            .ConfigVersion => LOCKB_TAG_CONFIG_VERSION,
            .Unknown => 0,
        };
    }

    fn extBit(): u16 {
        return switch (_) {
            .Workspaces => LOCKB_EXT_WORKSPACES,
            .TrustedDeps => LOCKB_EXT_TRUSTED_DEPS,
            .EmptyTrustedDeps => LOCKB_EXT_EMPTY_TRUSTED,
            .Overrides => LOCKB_EXT_OVERRIDES,
            .PatchedDeps => LOCKB_EXT_PATCHED_DEPS,
            .Catalogs => LOCKB_EXT_CATALOGS,
            .ConfigVersion => LOCKB_EXT_CONFIG_VERSION,
            .Unknown => 0,
        };
    }

    fn label(): []const u8 {
        return switch (_) {
            .Workspaces => "workspaces",
            .TrustedDeps => "trustedDependencies",
            .EmptyTrustedDeps => "emptyTrustedDependencies",
            .Overrides => "overrides",
            .PatchedDeps => "patchedDependencies",
            .Catalogs => "catalogs",
            .ConfigVersion => "configVersion",
            .Unknown => "unknown",
        };
    }
}

/// 将 8 字节 ASCII 标签按 little-endian 打包为 `u64`
pub fn lockbTagFromAscii8(tag: [8]u8): u64 {
    return readU64Le(tag[0..], 0);
}

/// 快速检测缓冲区是否以 `bun.lockb` Shebang + 魔数开头
pub fn isLockbMagic(buf: []const u8): bool {
    if (buf.len < LOCKB_SHEBANG_HEADER.len) {
        return false;
    }
    let i: usize = 0;
    while (i < LOCKB_SHEBANG_HEADER.len) {
        if (buf[i] != LOCKB_SHEBANG_HEADER[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 计算向上对齐到 `align_bytes`（2 的幂）的下一个二进制偏移
pub fn alignLockbOffset(pos: usize, align_bytes: usize): usize {
    if (align_bytes <= 1) return pos;
    const mask = align_bytes - 1;
    return (pos + mask) & ~mask;
}

/// `bun.lockb` 86 字节固定文件头
pub struct LockbHeader {
    format_version: u32 = LOCKB_FORMAT_CURRENT,
    meta_hash: [LOCKB_META_HASH_LEN]u8 = [0; LOCKB_META_HASH_LEN],
    total_buffer_size: u64 = LOCKB_FIXED_HEADER_LEN,
    /// 若源文件为 v2 并已自动标记迁移至 v3，则为 `true`
    migrated_from_v2: bool = false,

    /// 解码并校验 86 字节 `bun.lockb` 头部（严格复刻 Bun `Serializer.load` 版本迁移规则）
    static fn decode(buf: []const u8): LockbHeader! {
        if (buf.len < LOCKB_FIXED_HEADER_LEN) {
            return err.TruncatedHeader;
        }
        if (!isLockbMagic(buf)) {
            return err.InvalidMagic;
        }

        const raw_format = readU32Le(buf, 42);
        if (raw_format > LOCKB_FORMAT_CURRENT) {
            return err.UnexpectedVersion;
        }
        let migrated = false;
        if (raw_format < LOCKB_FORMAT_CURRENT) {
            // Bun 仅允许从 v2 迁移至 v3，更旧版本拒绝加载
            if (raw_format != LOCKB_FORMAT_V2) {
                return err.OutdatedVersion;
            }
            migrated = true;
        }

        let mh: [LOCKB_META_HASH_LEN]u8 = [0; LOCKB_META_HASH_LEN];
        let i: usize = 0;
        while (i < LOCKB_META_HASH_LEN) {
            mh[i] = buf[46 + i];
            i += 1;
        }
        const total_sz = readU64Le(buf, 78);

        return LockbHeader{
            format_version: LOCKB_FORMAT_CURRENT,
            meta_hash: mh,
            total_buffer_size: total_sz,
            migrated_from_v2: migrated,
        };
    }

    /// 编码为 86 字节线协议头部
    fn encode(out: []u8): usize! {
        if (out.len < LOCKB_FIXED_HEADER_LEN) {
            return err.BufferTooSmall;
        }
        copyBytes(out[0..42], LOCKB_SHEBANG_HEADER);
        writeU32Le(out, 42, _.format_version);
        copyBytes(out[46..78], _.meta_hash[0..]);
        writeU64Le(out, 78, _.total_buffer_size);
        return LOCKB_FIXED_HEADER_LEN;
    }

    /// 将 32 字节 `meta_hash` 格式化为 64 字节小写十六进制字符串
    fn formatMetaHashHex(out: []u8): usize! {
        const hex_len = LOCKB_META_HASH_LEN * 2;
        if (out.len < hex_len) {
            return err.BufferTooSmall;
        }
        const digits: []const u8 = "0123456789abcdef";
        let i: usize = 0;
        while (i < LOCKB_META_HASH_LEN) {
            const b = _.meta_hash[i];
            out[i * 2] = digits[@as(usize, b >> 4)];
            out[i * 2 + 1] = digits[@as(usize, b & 0x0f)];
            i += 1;
        }
        return hex_len;
    }
}

/// 二进制锁文件数组描述符（对齐 `Lockfile.Buffers.readArray` 的 `start_pos` + `byte_len` 与 `0xDEADBEEF` 空哨兵）
pub struct LockbArrayRef {
    start_pos: u64 = 0,
    byte_len: u64 = 0,
    is_empty_sentinel: bool = false,

    /// 从 `buf[offset..]` 解码数组描述符并校验边界
    static fn readFrom(buf: []const u8, offset: usize): LockbArrayRef! {
        if (offset + 8 > buf.len) {
            return err.TruncatedSection;
        }
        const start_pos = readU64Le(buf, offset);
        if (start_pos == LOCKB_DEAD_SENTINEL) {
            return LockbArrayRef{
                start_pos: LOCKB_DEAD_SENTINEL,
                byte_len: 0,
                is_empty_sentinel: true,
            };
        }
        if (offset + 16 > buf.len) {
            return err.TruncatedSection;
        }
        const byte_len = readU64Le(buf, offset + 8);
        if (start_pos > @as(u64, buf.len) || byte_len > @as(u64, buf.len) - start_pos) {
            return err.InvalidArrayRange;
        }
        return LockbArrayRef{
            start_pos: start_pos,
            byte_len: byte_len,
            is_empty_sentinel: false,
        };
    }

    /// 写入非空数组描述符（16 字节：`start_pos` + `byte_len`）
    static fn writeTo(out: []u8, offset: usize, start_pos: u64, byte_len: u64): usize! {
        if (offset + 16 > out.len) {
            return err.BufferTooSmall;
        }
        writeU64Le(out, offset, start_pos);
        writeU64Le(out, offset + 8, byte_len);
        return offset + 16;
    }

    /// 写入空数组 `0xDEADBEEF` 哨兵（8 字节）
    static fn writeEmptySentinel(out: []u8, offset: usize): usize! {
        if (offset + 8 > out.len) {
            return err.BufferTooSmall;
        }
        writeU64Le(out, offset, LOCKB_DEAD_SENTINEL);
        return offset + 8;
    }

    fn encodedSize(): usize {
        if (_.is_empty_sentinel) return 8;
        return 16;
    }
}

/// 单个尾部扩展段描述符
pub struct LockbSectionEntry {
    kind: LockbTagKind = .Unknown,
    tag_offset: usize = 0,
    payload_offset: usize = 0,
    payload_len: usize = 0,
}

/// 尾部扩展段扫描汇总结果
pub struct LockbTrailingSummary {
    sections: [LOCKB_MAX_SECTIONS]LockbSectionEntry = [LockbSectionEntry{}; LOCKB_MAX_SECTIONS],
    section_count: usize = 0,
    extension_mask: u16 = 0,
    config_version: u64? = null,
    has_empty_trusted_deps: bool = false,

    fn hasExtension(bit: u16): bool {
        return (_.extension_mask & bit) != 0;
    }
}

/// `LockbFrameCodec.inspectFrame()` 完整审查结果
pub struct LockbInspectResult {
    header: LockbHeader = LockbHeader{},
    effective_size: usize = 0,
    trailing: LockbTrailingSummary = LockbTrailingSummary{},
}

/// 零堆分配 `bun.lockb` 帧编解码器与尾部扩展段审查器
pub struct LockbFrameCodec {
    /// 组装完整的 `bun.lockb` 帧：86B Header + `core_payload` + 8B 零终止符 + 按 Bun 规范顺序排列的尾部扩展段
    static fn encodeFrame(
        header: LockbHeader,
        core_payload: []const u8,
        ext_mask: u16,
        config_version: u64?,
        out: []u8,
    ): usize! {
        let pos: usize = header.encode(out)?;
        if (pos + core_payload.len + 8 > out.len) {
            return err.BufferTooSmall;
        }
        copyBytes(out[pos .. pos + core_payload.len], core_payload);
        pos += core_payload.len;

        // 对齐 bun.lockb.cp L114：核心 Buffers 后紧跟 8 字节 0 终止符
        writeU64Le(out, pos, 0);
        pos += 8;

        // 按 Bun `Serializer.save` 规范顺序写入尾部扩展段
        if ((ext_mask & LOCKB_EXT_WORKSPACES) != 0) {
            pos = writeTaggedEmptySection(out, pos, LOCKB_TAG_WORKSPACES)?;
        }
        if ((ext_mask & LOCKB_EXT_TRUSTED_DEPS) != 0) {
            pos = writeTaggedEmptySection(out, pos, LOCKB_TAG_TRUSTED_DEPS)?;
        } else if ((ext_mask & LOCKB_EXT_EMPTY_TRUSTED) != 0) {
            // `eMpTrUsT` 是无载荷的纯 8 字节标签（对齐 bun.lockb.cp L458）
            if (pos + 8 > out.len) return err.BufferTooSmall;
            writeU64Le(out, pos, LOCKB_TAG_EMPTY_TRUSTED);
            pos += 8;
        }
        if ((ext_mask & LOCKB_EXT_OVERRIDES) != 0) {
            pos = writeTaggedEmptySection(out, pos, LOCKB_TAG_OVERRIDES)?;
        }
        if ((ext_mask & LOCKB_EXT_PATCHED_DEPS) != 0) {
            pos = writeTaggedEmptySection(out, pos, LOCKB_TAG_PATCHED_DEPS)?;
        }
        if ((ext_mask & LOCKB_EXT_CATALOGS) != 0) {
            pos = writeTaggedEmptySection(out, pos, LOCKB_TAG_CATALOGS)?;
        }
        if (cv := config_version) {
            if (pos + 16 > out.len) return err.BufferTooSmall;
            writeU64Le(out, pos, LOCKB_TAG_CONFIG_VERSION);
            writeU64Le(out, pos + 8, cv);
            pos += 16;
        } else if ((ext_mask & LOCKB_EXT_CONFIG_VERSION) != 0) {
            if (pos + 16 > out.len) return err.BufferTooSmall;
            writeU64Le(out, pos, LOCKB_TAG_CONFIG_VERSION);
            writeU64Le(out, pos + 8, 1);
            pos += 16;
        }

        // 回填头部第 78..86 字节的 `total_buffer_size`
        writeU64Le(out, 78, @as(u64, pos));
        return pos;
    }

    /// 零拷贝解析并审查 `bun.lockb` 帧（校验头部、截断边界、0 终止符及全部尾部扩展段）
    static fn inspectFrame(buf: []const u8, core_payload_len: usize): LockbInspectResult! {
        const hdr = LockbHeader.decode(buf)?;

        let effective_len: usize = buf.len;
        if (hdr.total_buffer_size > 0 && hdr.total_buffer_size < @as(u64, buf.len)) {
            effective_len = @as(usize, @intCast(hdr.total_buffer_size));
        }

        const term_pos = LOCKB_FIXED_HEADER_LEN + core_payload_len;
        if (term_pos + 8 > effective_len) {
            return err.TruncatedSection;
        }
        const terminator = readU64Le(buf, term_pos);
        if (terminator != 0) {
            return err.CorruptTerminator;
        }

        let summary = LockbTrailingSummary{};
        let pos: usize = term_pos + 8;

        while (pos + 8 <= effective_len) {
            const raw_tag = readU64Le(buf, pos);
            const kind = LockbTagKind.fromU64(raw_tag);
            if (kind == .Unknown) {
                break;
            }
            if (summary.section_count >= LOCKB_MAX_SECTIONS) {
                return err.TooManySections;
            }

            if (kind == .EmptyTrustedDeps) {
                summary.sections[summary.section_count] = LockbSectionEntry{
                    kind: kind,
                    tag_offset: pos,
                    payload_offset: pos + 8,
                    payload_len: 0,
                };
                summary.section_count += 1;
                summary.extension_mask |= kind.extBit();
                summary.has_empty_trusted_deps = true;
                pos += 8;
            } else if (kind == .ConfigVersion) {
                if (pos + 16 > effective_len) {
                    return err.TruncatedSection;
                }
                const cv = readU64Le(buf, pos + 8);
                summary.sections[summary.section_count] = LockbSectionEntry{
                    kind: kind,
                    tag_offset: pos,
                    payload_offset: pos + 8,
                    payload_len: 8,
                };
                summary.section_count += 1;
                summary.extension_mask |= kind.extBit();
                summary.config_version = cv;
                pos += 16;
            } else {
                // 其他扩展段在规范帧中携带 8 字节载荷长度前缀 + 载荷字节
                if (pos + 16 > effective_len) {
                    return err.TruncatedSection;
                }
                const plen_u64 = readU64Le(buf, pos + 8);
                if (plen_u64 > @as(u64, effective_len - (pos + 16))) {
                    return err.TruncatedSection;
                }
                const plen = @as(usize, @intCast(plen_u64));
                summary.sections[summary.section_count] = LockbSectionEntry{
                    kind: kind,
                    tag_offset: pos,
                    payload_offset: pos + 16,
                    payload_len: plen,
                };
                summary.section_count += 1;
                summary.extension_mask |= kind.extBit();
                pos += 16 + plen;
            }
        }

        return LockbInspectResult{
            header: hdr,
            effective_size: effective_len,
            trailing: summary,
        };
    }
}

// ----------------------------------------------------------------------------
// 内部零分配小端整数与字节辅助函数
// ----------------------------------------------------------------------------

fn writeTaggedEmptySection(out: []u8, pos: usize, tag: u64): usize! {
    if (pos + 16 > out.len) {
        return err.BufferTooSmall;
    }
    writeU64Le(out, pos, tag);
    writeU64Le(out, pos + 8, 0);
    return pos + 16;
}

fn copyBytes(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    return @as(u32, buf[off]) |
        (@as(u32, buf[off + 1]) << 8) |
        (@as(u32, buf[off + 2]) << 16) |
        (@as(u32, buf[off + 3]) << 24);
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = @as(u8, @truncate(v));
    buf[off + 1] = @as(u8, @truncate(v >> 8));
    buf[off + 2] = @as(u8, @truncate(v >> 16));
    buf[off + 3] = @as(u8, @truncate(v >> 24));
}

fn readU64Le(buf: []const u8, off: usize): u64 {
    return @as(u64, buf[off]) |
        (@as(u64, buf[off + 1]) << 8) |
        (@as(u64, buf[off + 2]) << 16) |
        (@as(u64, buf[off + 3]) << 24) |
        (@as(u64, buf[off + 4]) << 32) |
        (@as(u64, buf[off + 5]) << 40) |
        (@as(u64, buf[off + 6]) << 48) |
        (@as(u64, buf[off + 7]) << 56);
}

fn writeU64Le(buf: []u8, off: usize, v: u64) {
    buf[off] = @as(u8, @truncate(v));
    buf[off + 1] = @as(u8, @truncate(v >> 8));
    buf[off + 2] = @as(u8, @truncate(v >> 16));
    buf[off + 3] = @as(u8, @truncate(v >> 24));
    buf[off + 4] = @as(u8, @truncate(v >> 32));
    buf[off + 5] = @as(u8, @truncate(v >> 40));
    buf[off + 6] = @as(u8, @truncate(v >> 48));
    buf[off + 7] = @as(u8, @truncate(v >> 56));
}
