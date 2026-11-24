// Source: Absorbed from /data/dev/bun/src/install/windows-shim/BinLinkingShim.cp | Synced: 2026-11-23T20:23:46Z
// Pure-CP zero-allocation `.bunx` bin-linking shim binary codec, shebang parser
// (`#!/usr/bin/env node|bun`), script extension launcher classifier, and UTF-16LE
// trailer/flags encoder & decoder (`v1=5474..v5=5478` corruption-detection magic).

/// `.bunx` 垫片版本魔数（防偏移错位校验，严格对齐 Bun `VersionFlag`）
pub const BIN_SHIM_VERSION_V1: u16 = 5474;
pub const BIN_SHIM_VERSION_V2: u16 = 5475;
pub const BIN_SHIM_VERSION_V3: u16 = 5476;
pub const BIN_SHIM_VERSION_V4: u16 = 5477;
pub const BIN_SHIM_VERSION_V5: u16 = 5478;
pub const BIN_SHIM_VERSION_CURRENT: u16 = BIN_SHIM_VERSION_V5;

/// Shebang 最大扫描长度（`32766 + "#!".len`，对齐 Windows `CreateProcessW` 上限）
pub const BIN_SHIM_MAX_SHEBANG_INPUT_LEN: usize = 32768;

/// `.bunx` 垫片编解码错误集
pub err BinShimError {
    EmptyBinPath,
    LeadingSlashNotAllowed,
    BufferTooSmall,
    CorruptShim,
    InvalidVersionTag,
    InvalidUtf16Length,
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) {
        return false;
    }
    return bytesEql(s[0..prefix.len], prefix);
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn pathBasename(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 0 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    let i = end;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/' || path[i] == '\\' || (path[i] == ':' && i == 1)) {
            return path[i + 1..end];
        }
    }
    return path[0..end];
}

fn pathExtension(path: []const u8): []const u8 {
    const base = pathBasename(path);
    let i = base.len;
    while (i > 0) {
        i -= 1;
        if (base[i] == '.') {
            if (i == 0) {
                return "";
            }
            return base[i..];
        }
    }
    return "";
}

fn utf8ToUtf16CodeUnitCount(s: []const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b < 0x80) {
            count += 1;
            i += 1;
        } else if ((b & 0xE0) == 0xC0 && i + 1 < s.len) {
            count += 1;
            i += 2;
        } else if ((b & 0xF0) == 0xE0 && i + 2 < s.len) {
            count += 1;
            i += 3;
        } else if ((b & 0xF8) == 0xF0 && i + 3 < s.len) {
            count += 2; // surrogate pair
            i += 4;
        } else {
            count += 1;
            i += 1;
        }
    }
    return count;
}

fn writeLeU16(out: []u8, pos: usize, v: u16): usize! {
    if (2 > out.len - pos) {
        return err.BufferTooSmall;
    }
    out[pos] = @intCast(v & 0xFF);
    out[pos + 1] = @intCast((v >> 8) & 0xFF);
    return pos + 2;
}

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

fn readLeU16(src: []const u8, pos: usize): u16! {
    if (2 > src.len - pos) {
        return err.CorruptShim;
    }
    return @as(u16, src[pos]) | (@as(u16, src[pos + 1]) << 8);
}

fn readLeU32(src: []const u8, pos: usize): u32! {
    if (4 > src.len - pos) {
        return err.CorruptShim;
    }
    return @as(u32, src[pos]) |
        (@as(u32, src[pos + 1]) << 8) |
        (@as(u32, src[pos + 2]) << 16) |
        (@as(u32, src[pos + 3]) << 24);
}

fn writeUtf8AsUtf16Le(out: []u8, start_pos: usize, s: []const u8): usize! {
    let pos = start_pos;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b < 0x80) {
            pos = writeLeU16(out, pos, @as(u16, b))?;
            i += 1;
        } else if ((b & 0xE0) == 0xC0 && i + 1 < s.len) {
            const cp: u32 = ((@as(u32, b & 0x1F)) << 6) | @as(u32, s[i + 1] & 0x3F);
            pos = writeLeU16(out, pos, @intCast(cp))?;
            i += 2;
        } else if ((b & 0xF0) == 0xE0 && i + 2 < s.len) {
            const cp: u32 = ((@as(u32, b & 0x0F)) << 12) |
                ((@as(u32, s[i + 1] & 0x3F)) << 6) |
                @as(u32, s[i + 2] & 0x3F);
            pos = writeLeU16(out, pos, @intCast(cp))?;
            i += 3;
        } else if ((b & 0xF8) == 0xF0 && i + 3 < s.len) {
            const cp: u32 = ((@as(u32, b & 0x07)) << 18) |
                ((@as(u32, s[i + 1] & 0x3F)) << 12) |
                ((@as(u32, s[i + 2] & 0x3F)) << 6) |
                @as(u32, s[i + 3] & 0x3F);
            const adj = cp - 0x10000;
            const hi: u16 = @intCast(0xD800 + (adj >> 10));
            const lo: u16 = @intCast(0xDC00 + (adj & 0x3FF));
            pos = writeLeU16(out, pos, hi)?;
            pos = writeLeU16(out, pos, lo)?;
            i += 4;
        } else {
            pos = writeLeU16(out, pos, 0xFFFD)?;
            i += 1;
        }
    }
    return pos;
}

/// 脚本扩展名对应的默认启动器类别（对齐 Bun `ExtensionType`）
pub enum BinShimExtensionKind: u8 {
    RunWithBun = 0,
    RunWithCmd = 1,
    RunWithPowerShell = 2,
}

/// 按脚本扩展名分类默认启动器（`.js/.ts/.sh` → Bun，`.cmd/.bat` → cmd，`.ps1` → PowerShell）
pub fn classifyBinExtension(bin_path: []const u8): BinShimExtensionKind? {
    const ext = pathExtension(bin_path);
    if (ext.len == 0) return null;
    if (eqlIgnoreCase(ext, ".js") ||
        eqlIgnoreCase(ext, ".mjs") ||
        eqlIgnoreCase(ext, ".cjs") ||
        eqlIgnoreCase(ext, ".jsx") ||
        eqlIgnoreCase(ext, ".ts") ||
        eqlIgnoreCase(ext, ".cts") ||
        eqlIgnoreCase(ext, ".mts") ||
        eqlIgnoreCase(ext, ".tsx") ||
        eqlIgnoreCase(ext, ".sh"))
    {
        return .RunWithBun;
    }
    if (eqlIgnoreCase(ext, ".cmd") || eqlIgnoreCase(ext, ".bat")) {
        return .RunWithCmd;
    }
    if (eqlIgnoreCase(ext, ".ps1")) {
        return .RunWithPowerShell;
    }
    return null;
}

/// `.bunx` 尾部 16 位打包标志（bit0=`is_node_or_bun`, bit1=`is_node`, bit2=`has_shebang`, bits3..15=`version_tag`）
pub struct BinShimFlags {
    is_node_or_bun: bool = false,
    is_node: bool = false,
    has_shebang: bool = false,
    version_tag: u16 = BIN_SHIM_VERSION_CURRENT,

    fn toU16(): u16 {
        let v: u16 = (_.version_tag & 0x1FFF) << 3;
        if (_.is_node_or_bun) v |= 1;
        if (_.is_node) v |= 2;
        if (_.has_shebang) v |= 4;
        return v;
    }

    static fn fromU16(raw: u16): BinShimFlags {
        return BinShimFlags{
            is_node_or_bun: (raw & 1) != 0,
            is_node: (raw & 2) != 0,
            has_shebang: (raw & 4) != 0,
            version_tag: raw >> 3,
        };
    }

    fn isValid(): bool {
        return _.version_tag >= BIN_SHIM_VERSION_V1 && _.version_tag <= BIN_SHIM_VERSION_V5;
    }
}

/// 解析后的 Shebang 启动器信息（零分配视图，切片指向原始文件内容或静态常量）
pub struct BinShimShebang {
    launcher: []const u8 = "",
    utf16_len: usize = 0,
    is_node_or_bun: bool = false,
    is_node: bool = false,

    static fn init(launcher: []const u8, is_node_or_bun: bool): BinShimShebang {
        const exact_node = startsWith(launcher, "node") &&
            (launcher.len == 4 || launcher[4] == ' ');
        return BinShimShebang{
            launcher: launcher,
            utf16_len: utf8ToUtf16CodeUnitCount(launcher),
            is_node_or_bun: is_node_or_bun,
            is_node: exact_node && is_node_or_bun,
        };
    }

    /// 仅根据扩展名推导默认启动器（无 `#!` 时回退）
    static fn parseFromBinPath(bin_path: []const u8): BinShimShebang? {
        const ext_kind = classifyBinExtension(bin_path) ?? return null;
        return switch (ext_kind) {
            .RunWithBun => BinShimShebang.init("bun run", true),
            .RunWithCmd => BinShimShebang.init("cmd /c", false),
            .RunWithPowerShell => BinShimShebang.init("powershell -ExecutionPolicy Bypass -File", false),
        };
    }

    /// 解析可执行文件头部的 `#!` Shebang 行（支持 `/usr/bin/env`、`/bin/env` 及 `\r\n`）
    static fn parse(contents_raw: []const u8, bin_path: []const u8): BinShimShebang? {
        const limit = if (contents_raw.len < BIN_SHIM_MAX_SHEBANG_INPUT_LEN)
            contents_raw.len
        else
            BIN_SHIM_MAX_SHEBANG_INPUT_LEN;
        const contents = contents_raw[0..limit];

        if (contents.len < 3 || contents[0] != '#' || contents[1] != '!') {
            return BinShimShebang.parseFromBinPath(bin_path);
        }

        let nl: usize = 0;
        let found_nl = false;
        while (nl < contents.len) {
            if (contents[nl] == '\n') {
                found_nl = true;
                break;
            }
            nl += 1;
        }
        if (!found_nl) {
            return BinShimShebang.parseFromBinPath(bin_path);
        }
        let line_end = nl;
        if (line_end > 2 && contents[line_end - 1] == '\r') {
            line_end -= 1;
        }
        const line = contents[2..line_end];

        // Skip leading spaces after #!
        let pos: usize = 0;
        while (pos < line.len && line[pos] == ' ') {
            pos += 1;
        }
        if (pos >= line.len) {
            return BinShimShebang.parseFromBinPath(bin_path);
        }

        const first_start = pos;
        while (pos < line.len && line[pos] != ' ') {
            pos += 1;
        }
        const first = line[first_start..pos];

        if (bytesEql(first, "/usr/bin/env") || bytesEql(first, "/bin/env")) {
            while (pos < line.len && line[pos] == ' ') {
                pos += 1;
            }
            if (pos >= line.len) {
                return BinShimShebang.parseFromBinPath(bin_path);
            }
            const rest = line[pos..];
            let p_end = pos;
            while (p_end < line.len && line[p_end] != ' ') {
                p_end += 1;
            }
            const program = line[pos..p_end];
            const is_nb = bytesEql(program, "bun") || bytesEql(program, "node");
            return BinShimShebang.init(rest, is_nb);
        }

        return BinShimShebang.init(line, false);
    }

    fn encodedLength(): usize {
        // [UTF16LE launcher][u16 ' '][u32 bin_path_byte_len][u32 arg_byte_len]
        return (1 + _.utf16_len) * 2 + 8;
    }
}

/// 解码后的 `.bunx` 垫片结构（零拷贝切片视图）
pub struct DecodedBinShim {
    flags: BinShimFlags = BinShimFlags{},
    bin_path_utf16_bytes: []const u8 = "",
    launcher_utf16_bytes: []const u8 = "",
    bin_path_byte_len: usize = 0,
    arg_byte_len: usize = 0,
}

/// 将 UTF-16LE 字节切片解码为 UTF-8（用于校验或展示）
pub fn decodeShimUtf16LeToUtf8(utf16_bytes: []const u8, out: []u8): usize! {
    if (utf16_bytes.len % 2 != 0) {
        return err.InvalidUtf16Length;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < utf16_bytes.len) {
        const cu = readLeU16(utf16_bytes, i)?;
        i += 2;
        if (cu < 0x80) {
            if (pos >= out.len) return err.BufferTooSmall;
            out[pos] = @intCast(cu);
            pos += 1;
        } else if (cu < 0x800) {
            if (pos + 2 > out.len) return err.BufferTooSmall;
            out[pos] = @intCast(0xC0 | (cu >> 6));
            out[pos + 1] = @intCast(0x80 | (cu & 0x3F));
            pos += 2;
        } else {
            if (pos + 3 > out.len) return err.BufferTooSmall;
            out[pos] = @intCast(0xE0 | (cu >> 12));
            out[pos + 1] = @intCast(0x80 | ((cu >> 6) & 0x3F));
            out[pos + 2] = @intCast(0x80 | (cu & 0x3F));
            pos += 3;
        }
    }
    return pos;
}

/// `.bunx` 二进制垫片编码器与解码器（吸收自 Bun `BinLinkingShim.cp`）
pub struct BinLinkingShim {
    /// 相对于 `node_modules` 的可执行文件路径（不能以 `/` 开头）
    bin_path: []const u8 = "",
    shebang: BinShimShebang? = null,

    fn encodedLength(): usize {
        const bin_u16 = utf8ToUtf16CodeUnitCount(_.bin_path);
        let shebang_len: usize = 0;
        if (s := _.shebang) {
            shebang_len = s.encodedLength();
        }
        // [UTF16LE bin_path][u16 '"'][u16 0][optional shebang][u16 flags]
        return (bin_u16 + 2) * 2 + 2 + shebang_len;
    }

    fn encodeInto(out: []u8): usize! {
        if (_.bin_path.len == 0) {
            return err.EmptyBinPath;
        }
        if (_.bin_path[0] == '/') {
            return err.LeadingSlashNotAllowed;
        }
        const needed = _.encodedLength();
        if (out.len < needed) {
            return err.BufferTooSmall;
        }

        let pos = writeUtf8AsUtf16Le(out, 0, _.bin_path)?;
        const bin_path_byte_len: u32 = @intCast(pos);
        pos = writeLeU16(out, pos, '"')?;
        pos = writeLeU16(out, pos, 0)?;

        let flags = BinShimFlags{
            is_node_or_bun: false,
            is_node: false,
            has_shebang: false,
            version_tag: BIN_SHIM_VERSION_CURRENT,
        };

        if (s := _.shebang) {
            flags.has_shebang = true;
            flags.is_node_or_bun = s.is_node_or_bun;
            flags.is_node = s.is_node;

            const launcher_start = pos;
            pos = writeUtf8AsUtf16Le(out, pos, s.launcher)?;
            pos = writeLeU16(out, pos, ' ')?;
            const arg_byte_len: u32 = @intCast(pos - launcher_start);

            pos = writeLeU32(out, pos, bin_path_byte_len)?;
            pos = writeLeU32(out, pos, arg_byte_len)?;
        }

        pos = writeLeU16(out, pos, flags.toU16())?;
        return pos;
    }

    /// 宽松解码并严格校验版本魔数、偶数字节长度与边界（对齐 Bun `BinLinkingShim.looseDecode`）
    static fn decode(input: []const u8): DecodedBinShim! {
        if (input.len < 6) {
            return err.CorruptShim;
        }
        const flags_raw = readLeU16(input, input.len - 2)?;
        const flags = BinShimFlags.fromU16(flags_raw);
        if (!flags.isValid()) {
            return err.InvalidVersionTag;
        }

        if (flags.has_shebang) {
            if (input.len < 16) {
                return err.CorruptShim;
            }
            const bin_byte_len: usize = @intCast(readLeU32(input, input.len - 10)?);
            const arg_byte_len: usize = @intCast(readLeU32(input, input.len - 6)?);
            if (bin_byte_len % 2 != 0 || arg_byte_len % 2 != 0) {
                return err.InvalidUtf16Length;
            }
            // [bin_path][u16 '"'][u16 0][launcher + ' '][u32 bin_len][u32 arg_len][u16 flags]
            if (bin_byte_len + 4 + arg_byte_len + 10 > input.len) {
                return err.CorruptShim;
            }
            const launcher_start = bin_byte_len + 4;
            const launcher_end = launcher_start + arg_byte_len;
            return DecodedBinShim{
                flags: flags,
                bin_path_utf16_bytes: input[0..bin_byte_len],
                launcher_utf16_bytes: input[launcher_start..launcher_end],
                bin_path_byte_len: bin_byte_len,
                arg_byte_len: arg_byte_len,
            };
        }

        const payload_len = input.len - 2;
        if (payload_len < 4 || payload_len % 2 != 0) {
            return err.CorruptShim;
        }
        const bin_byte_len = payload_len - 4;
        return DecodedBinShim{
            flags: flags,
            bin_path_utf16_bytes: input[0..bin_byte_len],
            launcher_utf16_bytes: "",
            bin_path_byte_len: bin_byte_len,
            arg_byte_len: 0,
        };
    }
}
