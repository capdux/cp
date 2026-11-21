// Source: cp/lib/pack/tar_pax.cp (POSIX.1-2001 PAX & USTAR Safe Archive)
// ============================================================================
// lib/pack/tar_pax.cp — POSIX.1-2001 PAX 扩展头与 USTAR 安全 Tar 读写引擎
// 支持 >100 字节超长路径与大文件 size (typeflag 'x')、符号链接、权限位与路径穿越防御
// ============================================================================

import {
    TAR_BLOCK_SIZE,
    TarTypeFlag,
    TarHeader,
} from "./tar.cp";

pub const USTAR_MAX_NAME_LEN: usize = 100;
pub const USTAR_MAX_OCTAL_SIZE: u64 = 0o77777777777;

/// 严格校验归档条目路径安全性：
/// 拒绝空路径、空字节 (\0)、绝对路径 (/ 或 \ 开头)、驱动器前缀 (C:) 与 .. 路径穿越段
pub fn validateArchivePath(path: []const u8): bool {
    if (path.len == 0) {
        return false;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return false;
    }
    if (path.len >= 2 && path[1] == ':') {
        return false;
    }

    let i: usize = 0;
    let seg_start: usize = 0;
    while (i < path.len) {
        const c = path[i];
        if (c == 0) {
            return false;
        }
        if (c == '/' || c == '\\') {
            const seg = path[seg_start..i];
            if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
                return false;
            }
            seg_start = i + 1;
        }
        i += 1;
    }

    const last_seg = path[seg_start..path.len];
    if (last_seg.len == 2 && last_seg[0] == '.' && last_seg[1] == '.') {
        return false;
    }
    return true;
}

/// POSIX.1-2001 PAX / USTAR 归档条目视图
pub struct PaxTarEntry {
    name: []const u8 = "",
    linkname: []const u8 = "",
    size: u64 = 0,
    mode: u32 = 0o644,
    mtime: u64 = 0,
    typeflag: u8 = '0',
    data: []const u8 = "",

    pub fn isDirectory(): bool {
        return _.typeflag == '5' || (_.name.len > 0 && _.name[_.name.len - 1] == '/');
    }

    pub fn isRegularFile(): bool {
        return _.typeflag == '0' || _.typeflag == 0;
    }

    pub fn isSymlink(): bool {
        return _.typeflag == '2';
    }
}

/// POSIX.1-2001 PAX & USTAR 安全 Tar 归档写入器
pub struct PaxTarWriter {
    buf: []u8,
    len: usize = 0,

    pub static fn init(): PaxTarWriter {
        const init_cap: usize = 4096;
        const raw: [*]u8 = @alloc(init_cap);
        return PaxTarWriter {
            buf: raw[0..init_cap],
            len: 0,
        };
    }

    fn ensureCap(*_, extra: usize) {
        const needed = _.len + extra;
        if (needed <= _.buf.len) {
            return;
        }
        let next_cap = _.buf.len * 2;
        while (next_cap < needed) {
            next_cap = next_cap * 2;
        }
        const raw: [*]u8 = @alloc(next_cap);
        let new_buf = unsafe { raw[0..next_cap] };
        if (_.len > 0) {
            @memcpy(new_buf[0.._.len], _.buf[0.._.len]);
        }
        _.buf = new_buf;
    }

    fn allocBlock(*_): []u8 {
        _.ensureCap(TAR_BLOCK_SIZE);
        const start = _.len;
        let i: usize = 0;
        while (i < TAR_BLOCK_SIZE) {
            _.buf[start + i] = 0;
            i += 1;
        }
        _.len += TAR_BLOCK_SIZE;
        return _.buf[start .. start + TAR_BLOCK_SIZE];
    }

    fn appendPaddedData(*_, data: []const u8) {
        if (data.len == 0) {
            return;
        }
        const padded = (data.len + TAR_BLOCK_SIZE - 1) & ~(TAR_BLOCK_SIZE - 1);
        _.ensureCap(padded);
        const start = _.len;
        @memcpy(_.buf[start .. start + data.len], data);
        let i: usize = data.len;
        while (i < padded) {
            _.buf[start + i] = 0;
            i += 1;
        }
        _.len += padded;
    }

    static fn countDecimalDigits(val: usize): usize {
        if (val < 10) { return 1; }
        let n = val;
        let d: usize = 0;
        while (n > 0) {
            n = n / 10;
            d += 1;
        }
        return d;
    }

    static fn writeDecimal(out: []u8, val: usize): usize {
        const digits = PaxTarWriter.countDecimalDigits(val);
        if (out.len < digits) {
            return 0;
        }
        let n = val;
        let idx = digits;
        while (idx > 0) {
            idx -= 1;
            out[idx] = (48 as u8) + ((n % 10) as u8);
            n = n / 10;
        }
        return digits;
    }

    static fn formatU64Decimal(val: u64): []u8 {
        let tmp_len: usize = 1;
        let t = val;
        while (t >= 10) {
            t = t / 10;
            tmp_len += 1;
        }
        const raw: [*]u8 = @alloc(tmp_len);
        let out = unsafe { raw[0..tmp_len] };
        let n = val;
        let idx = tmp_len;
        while (idx > 0) {
            idx -= 1;
            out[idx] = (48 as u8) + ((n % 10) as u8);
            n = n / 10;
        }
        return out;
    }

    static fn encodePaxRecord(key: []const u8, value: []const u8): []u8 {
        // 格式: "<total_len> <key>=<value>\n"
        const rest_len = 1 + key.len + 1 + value.len + 1;
        let len_digits: usize = 1;
        let total_len = len_digits + rest_len;
        while (PaxTarWriter.countDecimalDigits(total_len) != len_digits) {
            len_digits = PaxTarWriter.countDecimalDigits(total_len);
            total_len = len_digits + rest_len;
        }

        const raw: [*]u8 = @alloc(total_len);
        let out = unsafe { raw[0..total_len] };
        let pos = PaxTarWriter.writeDecimal(out[0..len_digits], total_len);
        out[pos] = ' ';
        pos += 1;
        @memcpy(out[pos .. pos + key.len], key);
        pos += key.len;
        out[pos] = '=';
        pos += 1;
        @memcpy(out[pos .. pos + value.len], value);
        pos += value.len;
        out[pos] = '\n';
        return out;
    }

    static fn writeUstarBlock(
        block: []u8,
        name: []const u8,
        size: u64,
        mode: u32,
        mtime: u64,
        typeflag: u8,
        linkname: []const u8,
    ) {
        let i: usize = 0;
        while (i < TAR_BLOCK_SIZE) {
            block[i] = 0;
            i += 1;
        }

        const n_copy = if (name.len > USTAR_MAX_NAME_LEN) USTAR_MAX_NAME_LEN else name.len;
        if (n_copy > 0) {
            @memcpy(block[0..n_copy], name[0..n_copy]);
        }

        TarHeader.writeOctal(block[100..108], (mode & 0o7777) as u64);
        TarHeader.writeOctal(block[108..116], 0);
        TarHeader.writeOctal(block[116..124], 0);
        TarHeader.writeOctal(block[124..136], size);
        TarHeader.writeOctal(block[136..148], mtime);

        block[156] = if (typeflag == 0) ('0' as u8) else typeflag;

        const l_copy = if (linkname.len > USTAR_MAX_NAME_LEN) USTAR_MAX_NAME_LEN else linkname.len;
        if (l_copy > 0) {
            @memcpy(block[157 .. 157 + l_copy], linkname[0..l_copy]);
        }

        // USTAR 魔数 "ustar\0" + "00" (257..265)
        block[257] = 'u';
        block[258] = 's';
        block[259] = 't';
        block[260] = 'a';
        block[261] = 'r';
        block[262] = 0;
        block[263] = '0';
        block[264] = '0';

        const chksum = TarHeader.computeChecksum(block[0..TAR_BLOCK_SIZE]);
        TarHeader.writeOctal(block[148..155], chksum as u64);
        block[155] = ' ';
    }

    fn writeEntryInternal(
        *_,
        name: []const u8,
        data: []const u8,
        mode: u32,
        mtime: u64,
        typeflag: u8,
        linkname: []const u8,
    ): bool {
        if (!validateArchivePath(name)) {
            return false;
        }
        if (typeflag == (TarTypeFlag.Symlink as u8) && linkname.len == 0) {
            return false;
        }

        const file_size: u64 = data.len as u64;
        const needs_pax_path = name.len > USTAR_MAX_NAME_LEN;
        const needs_pax_link = linkname.len > USTAR_MAX_NAME_LEN;
        const needs_pax_size = file_size > USTAR_MAX_OCTAL_SIZE;

        if (needs_pax_path || needs_pax_link || needs_pax_size) {
            let pax_len: usize = 0;
            let rec_path: []const u8 = "";
            let rec_link: []const u8 = "";
            let rec_size: []const u8 = "";

            if (needs_pax_path) {
                rec_path = PaxTarWriter.encodePaxRecord("path", name);
                pax_len += rec_path.len;
            }
            if (needs_pax_link) {
                rec_link = PaxTarWriter.encodePaxRecord("linkpath", linkname);
                pax_len += rec_link.len;
            }
            if (needs_pax_size) {
                const size_str = PaxTarWriter.formatU64Decimal(file_size);
                rec_size = PaxTarWriter.encodePaxRecord("size", size_str);
                pax_len += rec_size.len;
            }

            const pax_raw: [*]u8 = @alloc(pax_len);
            let pax_payload = unsafe { pax_raw[0..pax_len] };
            let ppos: usize = 0;
            if (rec_path.len > 0) {
                @memcpy(pax_payload[ppos .. ppos + rec_path.len], rec_path);
                ppos += rec_path.len;
            }
            if (rec_link.len > 0) {
                @memcpy(pax_payload[ppos .. ppos + rec_link.len], rec_link);
                ppos += rec_link.len;
            }
            if (rec_size.len > 0) {
                @memcpy(pax_payload[ppos .. ppos + rec_size.len], rec_size);
                ppos += rec_size.len;
            }

            // 写入 PAX 扩展头 ('x') 块及载荷
            let pax_hdr_block = _.allocBlock();
            PaxTarWriter.writeUstarBlock(
                pax_hdr_block,
                "PaxHeaders.0/entry",
                pax_len as u64,
                0o644,
                mtime,
                TarTypeFlag.PaxExtendedHeader as u8,
                "",
            );
            _.appendPaddedData(pax_payload);
        }

        const hdr_size_field: u64 = if (needs_pax_size) 0 else file_size;
        let entry_block = _.allocBlock();
        PaxTarWriter.writeUstarBlock(
            entry_block,
            name,
            hdr_size_field,
            mode,
            mtime,
            typeflag,
            linkname,
        );
        _.appendPaddedData(data);
        return true;
    }

    /// 添加常规文件条目（自动在路径 >100 字节时生成 PAX 'x' 扩展头）
    pub fn addFile(*_, name: []const u8, data: []const u8, mode: u32 = 0o644, mtime: u64 = 0): bool {
        return _.writeEntryInternal(name, data, mode, mtime, TarTypeFlag.Regular as u8, "");
    }

    /// 添加目录条目（自动补全末尾 '/' 并保留 POSIX 权限位）
    pub fn addDirectory(*_, name: []const u8, mode: u32 = 0o755, mtime: u64 = 0): bool {
        if (!validateArchivePath(name)) {
            return false;
        }
        let dir_name = name;
        if (name[name.len - 1] != '/') {
            const raw: [*]u8 = @alloc(name.len + 1);
            let tmp = unsafe { raw[0 .. name.len + 1] };
            @memcpy(tmp[0..name.len], name);
            tmp[name.len] = '/';
            dir_name = tmp;
        }
        return _.writeEntryInternal(dir_name, "", mode, mtime, TarTypeFlag.Directory as u8, "");
    }

    /// 添加符号链接条目（支持 >100 字节 linkpath PAX 扩展头）
    pub fn addSymlink(*_, name: []const u8, target: []const u8, mode: u32 = 0o777, mtime: u64 = 0): bool {
        return _.writeEntryInternal(name, "", mode, mtime, TarTypeFlag.Symlink as u8, target);
    }

    /// 写入两个 512 字节全零块结束归档并返回完整切片
    pub fn finish(*_): []u8 {
        * = _.allocBlock();
        * = _.allocBlock();
        return _.buf[0.._.len];
    }
}

/// POSIX.1-2001 PAX & USTAR 安全 Tar 归档读取器
pub struct PaxTarReader {
    buf: []const u8,
    pos: usize = 0,
    strict_paths: bool = true,

    pub static fn init(buf: []const u8, strict_paths: bool = true): PaxTarReader {
        return PaxTarReader {
            buf: buf,
            pos: 0,
            strict_paths: strict_paths,
        };
    }

    static fn parseDecimal(slice: []const u8): usize? {
        if (slice.len == 0) {
            return null;
        }
        let val: usize = 0;
        let i: usize = 0;
        while (i < slice.len) {
            const c = slice[i];
            if (c < '0' || c > '9') {
                return null;
            }
            val = val * 10 + ((c - '0') as usize);
            i += 1;
        }
        return val;
    }

    static fn parseU64Decimal(slice: []const u8): u64? {
        if (slice.len == 0) {
            return null;
        }
        let val: u64 = 0;
        let i: usize = 0;
        while (i < slice.len) {
            const c = slice[i];
            if (c < '0' || c > '9') {
                return null;
            }
            val = val * 10 + ((c - '0') as u64);
            i += 1;
        }
        return val;
    }

    static fn extractCString(field: []const u8): []const u8 {
        let end: usize = 0;
        while (end < field.len && field[end] != 0) {
            end += 1;
        }
        return field[0..end];
    }

    pub fn next(*_): PaxTarEntry? {
        let pax_path: []const u8 = "";
        let has_pax_path = false;
        let pax_linkpath: []const u8 = "";
        let has_pax_linkpath = false;
        let pax_size: u64 = 0;
        let has_pax_size = false;

        while (true) {
            if (_.pos + TAR_BLOCK_SIZE > _.buf.len) {
                return null;
            }
            const hdr = _.buf[_.pos .. _.pos + TAR_BLOCK_SIZE];
            if (TarHeader.isZeroBlock(hdr)) {
                return null;
            }

            const expected_sum = TarHeader.parseOctal(hdr[148..156]) as u32;
            const actual_sum = TarHeader.computeChecksum(hdr);
            if (expected_sum != actual_sum) {
                return null;
            }

            const typeflag = if (hdr[156] == 0) ('0' as u8) else hdr[156];
            const header_size = TarHeader.parseOctal(hdr[124..136]);

            _.pos += TAR_BLOCK_SIZE;

            if (typeflag == (TarTypeFlag.PaxExtendedHeader as u8)) {
                const pax_data_len = header_size as usize;
                if (_.pos + pax_data_len > _.buf.len) {
                    return null;
                }
                const pax_data = _.buf[_.pos .. _.pos + pax_data_len];
                const padded_pax = (pax_data_len + TAR_BLOCK_SIZE - 1) & ~(TAR_BLOCK_SIZE - 1);
                if (_.pos + padded_pax > _.buf.len) {
                    return null;
                }
                _.pos += padded_pax;

                // 解析 PAX 键值对: "<len> <key>=<value>\n"
                let rpos: usize = 0;
                while (rpos < pax_data.len) {
                    let sp: usize = rpos;
                    while (sp < pax_data.len && pax_data[sp] != ' ') {
                        sp += 1;
                    }
                    if (sp >= pax_data.len) {
                        break;
                    }
                    const rec_len_opt = PaxTarReader.parseDecimal(pax_data[rpos..sp]);
                    if (rec_len_opt == null) {
                        break;
                    }
                    const rec_len = rec_len_opt.?;
                    if (rec_len == 0 || rpos + rec_len > pax_data.len) {
                        break;
                    }
                    const rec_end = rpos + rec_len;
                    const kv_start = sp + 1;
                    const kv_end = if (pax_data[rec_end - 1] == '\n') (rec_end - 1) else rec_end;

                    let eq: usize = kv_start;
                    while (eq < kv_end && pax_data[eq] != '=') {
                        eq += 1;
                    }
                    if (eq < kv_end) {
                        const key = pax_data[kv_start..eq];
                        const val = pax_data[eq + 1 .. kv_end];
                        if (key == "path") {
                            pax_path = val;
                            has_pax_path = true;
                        } else if (key == "linkpath") {
                            pax_linkpath = val;
                            has_pax_linkpath = true;
                        } else if (key == "size") {
                            if (parsed_sz := PaxTarReader.parseU64Decimal(val)) {
                                pax_size = parsed_sz;
                                has_pax_size = true;
                            }
                        }
                    }
                    rpos = rec_end;
                }
                continue;
            }

            // 拼接 USTAR prefix (345..500) 与 name (0..100)
            let final_name: []const u8 = "";
            if (has_pax_path) {
                final_name = pax_path;
            } else {
                const raw_name = PaxTarReader.extractCString(hdr[0..100]);
                const raw_prefix = PaxTarReader.extractCString(hdr[345..500]);
                if (raw_prefix.len > 0) {
                    const combined_len = raw_prefix.len + 1 + raw_name.len;
                    const raw: [*]u8 = @alloc(combined_len);
                    let tmp = unsafe { raw[0..combined_len] };
                    @memcpy(tmp[0..raw_prefix.len], raw_prefix);
                    tmp[raw_prefix.len] = '/';
                    @memcpy(tmp[raw_prefix.len + 1 .. combined_len], raw_name);
                    final_name = tmp;
                } else {
                    final_name = raw_name;
                }
            }

            if (_.strict_paths && !validateArchivePath(final_name)) {
                return null;
            }

            const final_link = if (has_pax_linkpath) pax_linkpath else PaxTarReader.extractCString(hdr[157..257]);
            const final_size = if (has_pax_size) pax_size else header_size;
            const mode = TarHeader.parseOctal(hdr[100..108]) as u32;
            const mtime = TarHeader.parseOctal(hdr[136..148]);

            const data_size = final_size as usize;
            if (_.pos + data_size > _.buf.len) {
                return null;
            }
            const data = _.buf[_.pos .. _.pos + data_size];
            const padded_size = (data_size + TAR_BLOCK_SIZE - 1) & ~(TAR_BLOCK_SIZE - 1);
            if (_.pos + padded_size <= _.buf.len) {
                _.pos += padded_size;
            } else {
                _.pos += data_size;
            }

            return PaxTarEntry {
                name: final_name,
                linkname: final_link,
                size: final_size,
                mode: mode,
                mtime: mtime,
                typeflag: typeflag,
                data: data,
            };
        }
    }
}
