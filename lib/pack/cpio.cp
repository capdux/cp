// Origin: Pure-CP Stdlib (Zero-Alloc Unix CPIO newc/crc/odc Archive Parser) | Created: 2026-11-22

/// CPIO 魔数与固定头大小常量
pub const CPIO_MAGIC_NEWC: []const u8 = "070701";
pub const CPIO_MAGIC_CRC: []const u8 = "070702";
pub const CPIO_MAGIC_ODC: []const u8 = "070707";
pub const CPIO_NEWC_HEADER_SIZE: usize = 110;
pub const CPIO_ODC_HEADER_SIZE: usize = 76;
pub const CPIO_TRAILER: []const u8 = "TRAILER!!!";

/// CPIO 归档格式类型
pub enum CpioFormat {
    newc,
    crc,
    odc,
    unknown,
}

/// CPIO 单个归档条目（`name` 已剥离末尾 `\0`，`data` 零拷贝指向原始输入）
pub struct CpioEntry {
    format: CpioFormat,
    ino: u32,
    mode: u32,
    uid: u32,
    gid: u32,
    nlink: u32,
    mtime: u64,
    filesize: u64,
    checksum: u32,
    name: []const u8,
    data: []const u8,

    /// 是否为目录（`S_IFDIR = 0o040000`）
    fn isDir(): bool {
        return (_.mode & 0xF000) == 0x4000;
    }

    /// 是否为普通文件（`S_IFREG = 0o100000`）
    fn isRegular(): bool {
        return (_.mode & 0xF000) == 0x8000;
    }

    /// 是否为符号链接（`S_IFLNK = 0o120000`）
    fn isSymlink(): bool {
        return (_.mode & 0xF000) == 0xA000;
    }
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

fn parseHexU32(s: []const u8): u32? {
    if (s.len != 8) return null;
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < 8) {
        const c = s[i];
        let v: u32 = 0;
        if (c >= '0' && c <= '9') {
            v = u32(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            v = u32(c - 'a') + 10;
        } else if (c >= 'A' && c <= 'F') {
            v = u32(c - 'A') + 10;
        } else {
            return null;
        }
        acc = (acc << 4) | v;
        i += 1;
    }
    return acc;
}

fn parseOctalU64(s: []const u8): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '7') {
            return null;
        }
        const d = u64(c - '0');
        if (acc > (0xFFFF_FFFF_FFFF_FFFF - d) / 8) {
            return null;
        }
        acc = (acc << 3) | d;
        i += 1;
    }
    return acc;
}

fn align4Usize(n: usize): usize? {
    const rem = n % 4;
    if (rem == 0) return n;
    const pad = 4 - rem;
    if (n > 0x7FFF_FFFF_FFFF_FFFF - pad) {
        return null;
    }
    return n + pad;
}

/// 检测 CPIO 归档格式（`newc` / `crc` / `odc` / `unknown`）
pub fn detectCpioFormat(data: []const u8): CpioFormat {
    if (data.len < 6) return CpioFormat.unknown;
    const magic = data[0..6];
    if (eqlBytes(magic, CPIO_MAGIC_NEWC)) return CpioFormat.newc;
    if (eqlBytes(magic, CPIO_MAGIC_CRC)) return CpioFormat.crc;
    if (eqlBytes(magic, CPIO_MAGIC_ODC)) return CpioFormat.odc;
    return CpioFormat.unknown;
}

/// 快速校验输入是否以合法 CPIO 魔数开头
pub fn isCpio(data: []const u8): bool {
    return detectCpioFormat(data) != CpioFormat.unknown;
}

/// 零堆分配 CPIO 归档流式迭代器（支持 `newc`/`crc` 4B 对齐与 `odc` 八进制格式，遇 `TRAILER!!!` 停止）
pub struct CpioIter {
    data: []const u8,
    pos: usize = 0,
    done: bool = false,
    failed: bool = false,

    static fn init(data: []const u8): CpioIter {
        return CpioIter {
            data: data,
            pos: 0,
            done: false,
            failed: false,
        };
    }

    /// 读取下一条 `CpioEntry`；遇到 `TRAILER!!!` 或到达末尾返回 `null`，遇到畸形条目置 `failed = true`
    fn next(*_): CpioEntry? {
        if (_.done || _.failed || _.pos >= _.data.len) {
            return null;
        }

        const rem_slice = _.data[_.pos..];
        const fmt = detectCpioFormat(rem_slice);
        if (fmt == CpioFormat.unknown) {
            _.failed = true;
            _.pos = _.data.len;
            return null;
        }

        if (fmt == CpioFormat.newc || fmt == CpioFormat.crc) {
            if (rem_slice.len < CPIO_NEWC_HEADER_SIZE) {
                _.failed = true;
                _.pos = _.data.len;
                return null;
            }
            let ino: u32 = 0;
            let mode: u32 = 0;
            let uid: u32 = 0;
            let gid: u32 = 0;
            let nlink: u32 = 0;
            let mtime32: u32 = 0;
            let filesize32: u32 = 0;
            let namesize32: u32 = 0;
            let checksum: u32 = 0;

            if (v := parseHexU32(rem_slice[6..14])) { ino = v; } else { _.failed = true; return null; }
            if (v := parseHexU32(rem_slice[14..22])) { mode = v; } else { _.failed = true; return null; }
            if (v := parseHexU32(rem_slice[22..30])) { uid = v; } else { _.failed = true; return null; }
            if (v := parseHexU32(rem_slice[30..38])) { gid = v; } else { _.failed = true; return null; }
            if (v := parseHexU32(rem_slice[38..46])) { nlink = v; } else { _.failed = true; return null; }
            if (v := parseHexU32(rem_slice[46..54])) { mtime32 = v; } else { _.failed = true; return null; }
            if (v := parseHexU32(rem_slice[54..62])) { filesize32 = v; } else { _.failed = true; return null; }
            // Validate dev/rdev fields are valid hex
            if (parseHexU32(rem_slice[62..70]) == null) { _.failed = true; return null; }
            if (parseHexU32(rem_slice[70..78]) == null) { _.failed = true; return null; }
            if (parseHexU32(rem_slice[78..86]) == null) { _.failed = true; return null; }
            if (parseHexU32(rem_slice[86..94]) == null) { _.failed = true; return null; }
            if (v := parseHexU32(rem_slice[94..102])) { namesize32 = v; } else { _.failed = true; return null; }
            if (v := parseHexU32(rem_slice[102..110])) { checksum = v; } else { _.failed = true; return null; }

            const namesize = usize(namesize32);
            const filesize = usize(filesize32);
            if (namesize == 0 || namesize > rem_slice.len - CPIO_NEWC_HEADER_SIZE) {
                _.failed = true;
                _.pos = _.data.len;
                return null;
            }

            let data_rel_start: usize = 0;
            if (al := align4Usize(CPIO_NEWC_HEADER_SIZE + namesize)) {
                data_rel_start = al;
            } else {
                _.failed = true;
                _.pos = _.data.len;
                return null;
            }
            if (data_rel_start > rem_slice.len || filesize > rem_slice.len - data_rel_start) {
                _.failed = true;
                _.pos = _.data.len;
                return null;
            }

            let data_padded_len: usize = 0;
            if (al := align4Usize(filesize)) {
                data_padded_len = al;
            } else {
                _.failed = true;
                _.pos = _.data.len;
                return null;
            }

            const raw_name = rem_slice[CPIO_NEWC_HEADER_SIZE .. CPIO_NEWC_HEADER_SIZE + namesize];
            const clean_name = if (raw_name.len > 0 && raw_name[raw_name.len - 1] == 0)
                raw_name[0 .. raw_name.len - 1]
            else
                raw_name;

            const entry_data = rem_slice[data_rel_start .. data_rel_start + filesize];

            // 推进游标（若在文件末尾允许省略最后的 padding 边界，但若有后续条目必对齐）
            const next_rel = if (data_padded_len <= rem_slice.len - data_rel_start)
                data_rel_start + data_padded_len
            else
                data_rel_start + filesize;
            _.pos += next_rel;

            if (eqlBytes(clean_name, CPIO_TRAILER)) {
                _.done = true;
                return null;
            }

            return CpioEntry {
                format: fmt,
                ino: ino,
                mode: mode,
                uid: uid,
                gid: gid,
                nlink: nlink,
                mtime: u64(mtime32),
                filesize: u64(filesize32),
                checksum: checksum,
                name: clean_name,
                data: entry_data,
            };
        }

        // ODC POSIX 76-byte octal format
        if (rem_slice.len < CPIO_ODC_HEADER_SIZE) {
            _.failed = true;
            _.pos = _.data.len;
            return null;
        }
        let ino64: u64 = 0;
        let mode64: u64 = 0;
        let uid64: u64 = 0;
        let gid64: u64 = 0;
        let nlink64: u64 = 0;
        let mtime64: u64 = 0;
        let namesize64: u64 = 0;
        let filesize64: u64 = 0;

        if (parseOctalU64(rem_slice[6..12]) == null) { _.failed = true; return null; } // dev
        if (v := parseOctalU64(rem_slice[12..18])) { ino64 = v; } else { _.failed = true; return null; }
        if (v := parseOctalU64(rem_slice[18..24])) { mode64 = v; } else { _.failed = true; return null; }
        if (v := parseOctalU64(rem_slice[24..30])) { uid64 = v; } else { _.failed = true; return null; }
        if (v := parseOctalU64(rem_slice[30..36])) { gid64 = v; } else { _.failed = true; return null; }
        if (v := parseOctalU64(rem_slice[36..42])) { nlink64 = v; } else { _.failed = true; return null; }
        if (parseOctalU64(rem_slice[42..48]) == null) { _.failed = true; return null; } // rdev
        if (v := parseOctalU64(rem_slice[48..59])) { mtime64 = v; } else { _.failed = true; return null; }
        if (v := parseOctalU64(rem_slice[59..65])) { namesize64 = v; } else { _.failed = true; return null; }
        if (v := parseOctalU64(rem_slice[65..76])) { filesize64 = v; } else { _.failed = true; return null; }

        const namesize = usize(namesize64);
        const filesize = usize(filesize64);
        if (namesize == 0 || namesize > rem_slice.len - CPIO_ODC_HEADER_SIZE) {
            _.failed = true;
            _.pos = _.data.len;
            return null;
        }
        const data_rel_start = CPIO_ODC_HEADER_SIZE + namesize;
        if (filesize > rem_slice.len - data_rel_start) {
            _.failed = true;
            _.pos = _.data.len;
            return null;
        }

        const raw_name = rem_slice[CPIO_ODC_HEADER_SIZE..data_rel_start];
        const clean_name = if (raw_name.len > 0 && raw_name[raw_name.len - 1] == 0)
            raw_name[0 .. raw_name.len - 1]
        else
            raw_name;
        const entry_data = rem_slice[data_rel_start .. data_rel_start + filesize];
        _.pos += data_rel_start + filesize;

        if (eqlBytes(clean_name, CPIO_TRAILER)) {
            _.done = true;
            return null;
        }

        return CpioEntry {
            format: CpioFormat.odc,
            ino: u32(ino64),
            mode: u32(mode64),
            uid: u32(uid64),
            gid: u32(gid64),
            nlink: u32(nlink64),
            mtime: mtime64,
            filesize: filesize64,
            checksum: 0,
            name: clean_name,
            data: entry_data,
        };
    }
}

/// 在 CPIO 归档中按路径名查找首个匹配条目（零堆分配）
pub fn findCpioEntry(data: []const u8, name: []const u8): CpioEntry? {
    let it = CpioIter.init(data);
    while (entry := it.next()) {
        if (eqlBytes(entry.name, name)) {
            return entry;
        }
    }
    return null;
}

/// 统计 CPIO 归档中有效条目数量（不含 `TRAILER!!!`）
pub fn countCpioEntries(data: []const u8): usize {
    let it = CpioIter.init(data);
    let count: usize = 0;
    while (entry := it.next()) {
        * = entry;
        count += 1;
    }
    return count;
}
