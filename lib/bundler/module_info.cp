// Source: Absorbed from /data/dev/bun/src/bundler/analyze_transpiled_module.cp | Synced: 2026-11-22T09:59:21Z
// Pure-CP zero-allocation transpiled ESM/TypeScript module analysis metadata builder,
// `finalize()` local-to-indirect re-export promoter (with TS type-only import elision safety),
// cross-chunk `replaceStringId` specifier remapper, and aligned binary cache serializer/deserializer.

/// 单个模块允许的最大字符串驻留条目数
pub const MODULE_INFO_MAX_STRINGS: usize = 128;

/// 单个模块字符串驻留池总字节容量
pub const MODULE_INFO_STRING_BUF_CAP: usize = 4096;

/// 单个模块允许的最大分析记录条数
pub const MODULE_INFO_MAX_RECORDS: usize = 128;

/// 单个模块记录缓冲最大 StringID 槽位数
pub const MODULE_INFO_MAX_BUFFER_IDS: usize = 384;

/// 单个模块允许的最大 `requested_modules` 数量
pub const MODULE_INFO_MAX_REQUESTED: usize = 64;

/// 特殊哨兵 StringID：`default` 导出标记
pub const MODULE_STRING_STAR_DEFAULT: u32 = 0xFFFF_FFFF;

/// 特殊哨兵 StringID：`*` 命名空间标记
pub const MODULE_STRING_STAR_NAMESPACE: u32 = 0xFFFF_FFFE;

/// `FetchParameters` 预置常量（对齐 Bun `ModuleInfo.FetchParameters`）
pub const MODULE_FETCH_NONE: u32 = 0xFFFF_FFFF;
pub const MODULE_FETCH_JAVASCRIPT: u32 = 0xFFFF_FFFE;
pub const MODULE_FETCH_WEBASSEMBLY: u32 = 0xFFFF_FFFD;
pub const MODULE_FETCH_JSON: u32 = 0xFFFF_FFFC;

/// 转译模块分析记录与序列化错误集
pub err ModuleInfoError {
    AlreadyFinalized,
    NotFinalized,
    StringTableFull,
    StringBufferFull,
    RecordTableFull,
    BufferFull,
    RequestedTableFull,
    InvalidRecordKind,
    BufferTooSmall,
    CorruptBinary,
}

/// 模块分析记录类型（严格对齐 Bun `analyze_transpiled_module.RecordKind`）
pub enum ModuleRecordKind: u8 {
    DeclaredVariable = 0,
    LexicalVariable = 1,
    ImportInfoSingle = 2,
    ImportInfoSingleTypeScript = 3,
    ImportInfoNamespace = 4,
    ExportInfoIndirect = 5,
    ExportInfoLocal = 6,
    ExportInfoNamespace = 7,
    ExportInfoStar = 8,

    /// 返回该记录类型在 `buffer` 中占用的 `StringID` 槽位数
    fn slotCount(): usize {
        return switch (_) {
            .DeclaredVariable, .LexicalVariable => 1,
            .ImportInfoSingle => 3,
            .ImportInfoSingleTypeScript => 3,
            .ImportInfoNamespace => 3,
            .ExportInfoIndirect => 3,
            .ExportInfoLocal => 3,
            .ExportInfoNamespace => 2,
            .ExportInfoStar => 1,
        };
    }

    static fn fromU8(v: u8): ModuleRecordKind? {
        return switch (v) {
            0 => .DeclaredVariable,
            1 => .LexicalVariable,
            2 => .ImportInfoSingle,
            3 => .ImportInfoSingleTypeScript,
            4 => .ImportInfoNamespace,
            5 => .ExportInfoIndirect,
            6 => .ExportInfoLocal,
            7 => .ExportInfoNamespace,
            8 => .ExportInfoStar,
            else => null,
        };
    }
}

/// 模块布尔特性标志（按 Bun 位布局打包为单字节）
pub struct ModuleInfoFlags {
    contains_import_meta: bool = false,
    is_typescript: bool = false,
    has_tla: bool = false,

    fn toByte(): u8 {
        let b: u8 = 0;
        if (_.contains_import_meta) b |= 1;
        if (_.is_typescript) b |= 2;
        if (_.has_tla) b |= 4;
        return b;
    }

    static fn fromByte(b: u8): ModuleInfoFlags {
        return ModuleInfoFlags{
            contains_import_meta: (b & 1) != 0,
            is_typescript: (b & 2) != 0,
            has_tla: (b & 4) != 0,
        };
    }
}

/// 模块分析二进制序列化/反序列化辅助读写
fn writeLeU32(out: []u8, pos: usize, v: u32): usize! {
    if (4 > out.len - pos) {
        return err.BufferTooSmall;
    }
    out[pos] = @intCast(v & 0xFF);
    out[pos + 1] = @intCast((v >> 8) & 0xFF);
    out[pos + 2] = @intCast((v >> 16) & 0xFF);
    out[pos + 3] = @intCast((v >> 24) & 0xFF);
    return pos + 4;
}

fn readLeU32(src: []const u8, pos: usize): u32! {
    if (4 > src.len - pos) {
        return err.CorruptBinary;
    }
    return @as(u32, src[pos]) |
        (@as(u32, src[pos + 1]) << 8) |
        (@as(u32, src[pos + 2]) << 16) |
        (@as(u32, src[pos + 3]) << 24);
}

fn bytesEql(a: []const u8, b: []const u8): bool {
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

/// 零分配转译模块分析元数据构建器与二进制缓存编解码器
pub struct TranspiledModuleInfo {
    strings_buf: [MODULE_INFO_STRING_BUF_CAP]u8,
    strings_buf_len: usize,
    string_offsets: [MODULE_INFO_MAX_STRINGS]u32,
    string_lens: [MODULE_INFO_MAX_STRINGS]u32,
    string_count: usize,

    requested_keys: [MODULE_INFO_MAX_REQUESTED]u32,
    requested_values: [MODULE_INFO_MAX_REQUESTED]u32,
    requested_count: usize,

    buffer: [MODULE_INFO_MAX_BUFFER_IDS]u32,
    buffer_len: usize,

    record_kinds: [MODULE_INFO_MAX_RECORDS]ModuleRecordKind,
    record_count: usize,

    exported_names: [MODULE_INFO_MAX_RECORDS]u32,
    exported_count: usize,

    flags: ModuleInfoFlags,
    finalized: bool,

    static fn init(is_typescript: bool): TranspiledModuleInfo {
        return TranspiledModuleInfo{
            strings_buf: [0; MODULE_INFO_STRING_BUF_CAP],
            strings_buf_len: 0,
            string_offsets: [0; MODULE_INFO_MAX_STRINGS],
            string_lens: [0; MODULE_INFO_MAX_STRINGS],
            string_count: 0,
            requested_keys: [0; MODULE_INFO_MAX_REQUESTED],
            requested_values: [0; MODULE_INFO_MAX_REQUESTED],
            requested_count: 0,
            buffer: [0; MODULE_INFO_MAX_BUFFER_IDS],
            buffer_len: 0,
            record_kinds: [.DeclaredVariable; MODULE_INFO_MAX_RECORDS],
            record_count: 0,
            exported_names: [0; MODULE_INFO_MAX_RECORDS],
            exported_count: 0,
            flags: ModuleInfoFlags{
                contains_import_meta: false,
                is_typescript: is_typescript,
                has_tla: false,
            },
            finalized: false,
        };
    }

    /// 驻留字符串并返回其去重后的 `StringID`
    fn internString(*_, value: []const u8): u32! {
        let i: usize = 0;
        while (i < _.string_count) {
            const off: usize = @intCast(_.string_offsets[i]);
            const len: usize = @intCast(_.string_lens[i]);
            if (bytesEql(_.strings_buf[off..off + len], value)) {
                return @intCast(i);
            }
            i += 1;
        }
        if (_.string_count >= MODULE_INFO_MAX_STRINGS) {
            return err.StringTableFull;
        }
        if (value.len > MODULE_INFO_STRING_BUF_CAP - _.strings_buf_len) {
            return err.StringBufferFull;
        }
        const id = _.string_count;
        const off = _.strings_buf_len;
        let k: usize = 0;
        while (k < value.len) {
            _.strings_buf[off + k] = value[k];
            k += 1;
        }
        _.string_offsets[id] = @intCast(off);
        _.string_lens[id] = @intCast(value.len);
        _.strings_buf_len += value.len;
        _.string_count += 1;
        return @intCast(id);
    }

    /// 按 `StringID` 读取驻留字符串切片
    fn getString(id: u32): ([]const u8)? {
        const idx: usize = @intCast(id);
        if (idx >= _.string_count) {
            return null;
        }
        const off: usize = @intCast(_.string_offsets[idx]);
        const len: usize = @intCast(_.string_lens[idx]);
        return _.strings_buf[off..off + len];
    }

    fn hasOrAddExportedName(*_, export_name: u32): bool {
        let i: usize = 0;
        while (i < _.exported_count) {
            if (_.exported_names[i] == export_name) {
                return true;
            }
            i += 1;
        }
        if (_.exported_count < MODULE_INFO_MAX_RECORDS) {
            _.exported_names[_.exported_count] = export_name;
            _.exported_count += 1;
        }
        return false;
    }

    fn pushRecord1(*_, kind: ModuleRecordKind, a: u32)! {
        if (_.finalized) return err.AlreadyFinalized;
        if (_.record_count >= MODULE_INFO_MAX_RECORDS) return err.RecordTableFull;
        if (_.buffer_len + 1 > MODULE_INFO_MAX_BUFFER_IDS) return err.BufferFull;
        _.record_kinds[_.record_count] = kind;
        _.record_count += 1;
        _.buffer[_.buffer_len] = a;
        _.buffer_len += 1;
    }

    fn pushRecord2(*_, kind: ModuleRecordKind, a: u32, b: u32)! {
        if (_.finalized) return err.AlreadyFinalized;
        if (_.record_count >= MODULE_INFO_MAX_RECORDS) return err.RecordTableFull;
        if (_.buffer_len + 2 > MODULE_INFO_MAX_BUFFER_IDS) return err.BufferFull;
        _.record_kinds[_.record_count] = kind;
        _.record_count += 1;
        _.buffer[_.buffer_len] = a;
        _.buffer[_.buffer_len + 1] = b;
        _.buffer_len += 2;
    }

    fn pushRecord3(*_, kind: ModuleRecordKind, a: u32, b: u32, c: u32)! {
        if (_.finalized) return err.AlreadyFinalized;
        if (_.record_count >= MODULE_INFO_MAX_RECORDS) return err.RecordTableFull;
        if (_.buffer_len + 3 > MODULE_INFO_MAX_BUFFER_IDS) return err.BufferFull;
        _.record_kinds[_.record_count] = kind;
        _.record_count += 1;
        _.buffer[_.buffer_len] = a;
        _.buffer[_.buffer_len + 1] = b;
        _.buffer[_.buffer_len + 2] = c;
        _.buffer_len += 3;
    }

    fn addDeclaredVariable(*_, name_id: u32)! {
        _.pushRecord1(.DeclaredVariable, name_id)?;
    }

    fn addLexicalVariable(*_, name_id: u32)! {
        _.pushRecord1(.LexicalVariable, name_id)?;
    }

    fn addImportSingle(*_, module_id: u32, import_id: u32, local_id: u32, only_used_as_type: bool)! {
        const kind = if (only_used_as_type) ModuleRecordKind.ImportInfoSingleTypeScript else ModuleRecordKind.ImportInfoSingle;
        _.pushRecord3(kind, module_id, import_id, local_id)?;
    }

    fn addImportNamespace(*_, module_id: u32, local_id: u32)! {
        _.pushRecord3(.ImportInfoNamespace, module_id, MODULE_STRING_STAR_NAMESPACE, local_id)?;
    }

    fn addExportIndirect(*_, export_id: u32, import_id: u32, module_id: u32)! {
        if (_.hasOrAddExportedName(export_id)) return;
        _.pushRecord3(.ExportInfoIndirect, export_id, import_id, module_id)?;
    }

    fn addExportLocal(*_, export_id: u32, local_id: u32)! {
        if (_.hasOrAddExportedName(export_id)) return;
        _.pushRecord3(.ExportInfoLocal, export_id, local_id, MODULE_STRING_STAR_DEFAULT)?;
    }

    fn addExportNamespace(*_, export_id: u32, module_id: u32)! {
        if (_.hasOrAddExportedName(export_id)) return;
        _.pushRecord2(.ExportInfoNamespace, export_id, module_id)?;
    }

    fn addExportStar(*_, module_id: u32)! {
        _.pushRecord1(.ExportInfoStar, module_id)?;
    }

    /// 记录模块导入依赖及其 `FetchParameters`（按 JSC 规范仅保留首个导入声明的属性）
    fn requestModule(*_, module_id: u32, fetch_params: u32)! {
        if (_.finalized) return err.AlreadyFinalized;
        let i: usize = 0;
        while (i < _.requested_count) {
            if (_.requested_keys[i] == module_id) {
                return;
            }
            i += 1;
        }
        if (_.requested_count >= MODULE_INFO_MAX_REQUESTED) {
            return err.RequestedTableFull;
        }
        _.requested_keys[_.requested_count] = module_id;
        _.requested_values[_.requested_count] = fetch_params;
        _.requested_count += 1;
    }

    /// 跨 chunk 路径重映射：在所有记录和 `requested_modules` 中将 `old_id` 替换为 `new_id`
    fn replaceStringId(*_, old_id: u32, new_id: u32)! {
        if (_.finalized) return err.AlreadyFinalized;
        let i: usize = 0;
        while (i < _.buffer_len) {
            if (_.buffer[i] == old_id) {
                _.buffer[i] = new_id;
            }
            i += 1;
        }
        i = 0;
        while (i < _.requested_count) {
            if (_.requested_keys[i] == old_id) {
                _.requested_keys[i] = new_id;
            }
            i += 1;
        }
    }

    /// 终结分析（精确复刻 Bun `ModuleInfo.finalize`）：
    /// 将引用已导入本地绑定的 `ExportInfoLocal` 原位提升为 `ExportInfoIndirect`；
    /// 在 TypeScript 模式下同步将非命名空间导入升级为 `ImportInfoSingleTypeScript` 以兼容类型消除。
    fn finalize(*_)! {
        if (_.finalized) {
            return err.AlreadyFinalized;
        }

        // Pass 1: collect local_name -> (module_id, import_id, record_idx, is_namespace)
        let local_ids: [MODULE_INFO_MAX_RECORDS]u32 = [0; MODULE_INFO_MAX_RECORDS];
        let mod_ids: [MODULE_INFO_MAX_RECORDS]u32 = [0; MODULE_INFO_MAX_RECORDS];
        let imp_ids: [MODULE_INFO_MAX_RECORDS]u32 = [0; MODULE_INFO_MAX_RECORDS];
        let rec_indices: [MODULE_INFO_MAX_RECORDS]usize = [0; MODULE_INFO_MAX_RECORDS];
        let is_ns: [MODULE_INFO_MAX_RECORDS]bool = [false; MODULE_INFO_MAX_RECORDS];
        let map_len: usize = 0;

        let buf_i: usize = 0;
        let idx: usize = 0;
        while (idx < _.record_count) {
            const k = _.record_kinds[idx];
            if (k == .ImportInfoSingle || k == .ImportInfoSingleTypeScript) {
                local_ids[map_len] = _.buffer[buf_i + 2];
                mod_ids[map_len] = _.buffer[buf_i];
                imp_ids[map_len] = _.buffer[buf_i + 1];
                rec_indices[map_len] = idx;
                is_ns[map_len] = false;
                map_len += 1;
            } else if (k == .ImportInfoNamespace) {
                local_ids[map_len] = _.buffer[buf_i + 2];
                mod_ids[map_len] = _.buffer[buf_i];
                imp_ids[map_len] = MODULE_STRING_STAR_NAMESPACE;
                rec_indices[map_len] = idx;
                is_ns[map_len] = true;
                map_len += 1;
            }
            buf_i += k.slotCount();
            idx += 1;
        }

        // Pass 2: promote matching ExportInfoLocal -> ExportInfoIndirect in place
        buf_i = 0;
        idx = 0;
        while (idx < _.record_count) {
            const k = _.record_kinds[idx];
            if (k == .ExportInfoLocal) {
                const target_local = _.buffer[buf_i + 1];
                let m: usize = 0;
                let found_idx: usize? = null;
                while (m < map_len) {
                    if (local_ids[m] == target_local) {
                        found_idx = m;
                    }
                    m += 1;
                }
                if (fi := found_idx) {
                    _.record_kinds[idx] = .ExportInfoIndirect;
                    _.buffer[buf_i + 1] = imp_ids[fi];
                    _.buffer[buf_i + 2] = mod_ids[fi];
                    if (!is_ns[fi] && _.flags.is_typescript) {
                        _.record_kinds[rec_indices[fi]] = .ImportInfoSingleTypeScript;
                    }
                }
            }
            buf_i += k.slotCount();
            idx += 1;
        }

        _.finalized = true;
    }

    /// 按 Bun 紧凑 4 字节对齐二进制格式序列化到 `out`
    fn serialize(out: []u8): usize! {
        if (!_.finalized) {
            return err.NotFinalized;
        }
        let pos: usize = 0;

        // 1. record_kinds + 4-byte alignment padding
        pos = writeLeU32(out, pos, @intCast(_.record_count))?;
        if (_.record_count > out.len - pos) return err.BufferTooSmall;
        let i: usize = 0;
        while (i < _.record_count) {
            out[pos + i] = @intFromEnum(_.record_kinds[i]);
            i += 1;
        }
        pos += _.record_count;
        const pad = (4 - (_.record_count % 4)) % 4;
        if (pad > out.len - pos) return err.BufferTooSmall;
        i = 0;
        while (i < pad) {
            out[pos + i] = 0;
            i += 1;
        }
        pos += pad;

        // 2. buffer (u32 LE[])
        pos = writeLeU32(out, pos, @intCast(_.buffer_len))?;
        i = 0;
        while (i < _.buffer_len) {
            pos = writeLeU32(out, pos, _.buffer[i])?;
            i += 1;
        }

        // 3. requested_modules (keys then values)
        pos = writeLeU32(out, pos, @intCast(_.requested_count))?;
        i = 0;
        while (i < _.requested_count) {
            pos = writeLeU32(out, pos, _.requested_keys[i])?;
            i += 1;
        }
        i = 0;
        while (i < _.requested_count) {
            pos = writeLeU32(out, pos, _.requested_values[i])?;
            i += 1;
        }

        // 4. flags (1 byte) + 3 bytes alignment padding
        if (4 > out.len - pos) return err.BufferTooSmall;
        out[pos] = _.flags.toByte();
        out[pos + 1] = 0;
        out[pos + 2] = 0;
        out[pos + 3] = 0;
        pos += 4;

        // 5. strings_lens (u32 LE[]) + strings_buf
        pos = writeLeU32(out, pos, @intCast(_.string_count))?;
        i = 0;
        while (i < _.string_count) {
            pos = writeLeU32(out, pos, _.string_lens[i])?;
            i += 1;
        }
        if (_.strings_buf_len > out.len - pos) return err.BufferTooSmall;
        i = 0;
        while (i < _.strings_buf_len) {
            out[pos + i] = _.strings_buf[i];
            i += 1;
        }
        pos += _.strings_buf_len;
        return pos;
    }

    /// 从 Bun 紧凑二进制缓存记录反序列化并校验完整性（抗截断与非法枚举值）
    static fn deserialize(src: []const u8): TranspiledModuleInfo! {
        let info = TranspiledModuleInfo.init(false);
        let pos: usize = 0;

        // 1. record_kinds + alignment padding
        const rec_count: usize = @intCast(readLeU32(src, pos)?);
        pos += 4;
        if (rec_count > MODULE_INFO_MAX_RECORDS || rec_count > src.len - pos) {
            return err.CorruptBinary;
        }
        let i: usize = 0;
        let expected_buf_ids: usize = 0;
        while (i < rec_count) {
            const maybe_kind = ModuleRecordKind.fromU8(src[pos + i]);
            if (kind := maybe_kind) {
                info.record_kinds[i] = kind;
                expected_buf_ids += kind.slotCount();
            } else {
                return err.InvalidRecordKind;
            }
            i += 1;
        }
        info.record_count = rec_count;
        pos += rec_count;

        const pad = (4 - (rec_count % 4)) % 4;
        if (pad > src.len - pos) return err.CorruptBinary;
        pos += pad;

        // 2. buffer
        const buf_len: usize = @intCast(readLeU32(src, pos)?);
        pos += 4;
        if (buf_len > MODULE_INFO_MAX_BUFFER_IDS || buf_len != expected_buf_ids) {
            return err.CorruptBinary;
        }
        i = 0;
        while (i < buf_len) {
            info.buffer[i] = readLeU32(src, pos)?;
            pos += 4;
            i += 1;
        }
        info.buffer_len = buf_len;

        // 3. requested_modules
        const req_count: usize = @intCast(readLeU32(src, pos)?);
        pos += 4;
        if (req_count > MODULE_INFO_MAX_REQUESTED) {
            return err.CorruptBinary;
        }
        i = 0;
        while (i < req_count) {
            info.requested_keys[i] = readLeU32(src, pos)?;
            pos += 4;
            i += 1;
        }
        i = 0;
        while (i < req_count) {
            info.requested_values[i] = readLeU32(src, pos)?;
            pos += 4;
            i += 1;
        }
        info.requested_count = req_count;

        // 4. flags + 3-byte padding
        if (4 > src.len - pos) return err.CorruptBinary;
        info.flags = ModuleInfoFlags.fromByte(src[pos]);
        pos += 4;

        // 5. strings_lens + strings_buf
        const str_count: usize = @intCast(readLeU32(src, pos)?);
        pos += 4;
        if (str_count > MODULE_INFO_MAX_STRINGS) {
            return err.CorruptBinary;
        }
        let total_str_bytes: usize = 0;
        i = 0;
        while (i < str_count) {
            const slen = readLeU32(src, pos)?;
            pos += 4;
            info.string_offsets[i] = @intCast(total_str_bytes);
            info.string_lens[i] = slen;
            const slen_usize: usize = @intCast(slen);
            total_str_bytes += slen_usize;
            i += 1;
        }
        if (total_str_bytes > MODULE_INFO_STRING_BUF_CAP || total_str_bytes > src.len - pos) {
            return err.CorruptBinary;
        }
        i = 0;
        while (i < total_str_bytes) {
            info.strings_buf[i] = src[pos + i];
            i += 1;
        }
        info.string_count = str_count;
        info.strings_buf_len = total_str_bytes;
        info.finalized = true;
        return info;
    }
}
