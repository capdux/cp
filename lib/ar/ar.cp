// Origin: Pure-CP Stdlib (Zero-Alloc Unix ar Static Library & .deb Archive Parser) | Created: 2026-11-22

/// Unix `ar` 归档全局魔数与头部常量
pub const AR_MAGIC: []const u8 = "!<arch>\n";
pub const AR_HEADER_SIZE: usize = 60;
pub const AR_END_MAGIC: []const u8 = "`\n";

/// `ar` 归档成员类型枚举
pub enum ArMemberKind: u8 {
    normal = 0,
    gnu_symtab = 1,
    gnu_strtab = 2,
    bsd_symtab = 3,
    unknown = 4,
}

/// 零拷贝 `ar` 归档成员视图
pub struct ArMember {
    /// 原始 16 字节头名称切片（已去尾部空格）
    raw_name: []const u8,
    /// 解析后的最终文件名（已解引用 GNU `"//"` 长名表、剥离 BSD `"#1/<len>"` 或尾部 `'/'`）
    name: []const u8,
    mtime: u64,
    uid: u32,
    gid: u32,
    mode: u32,
    size: u64,
    kind: ArMemberKind,
    /// 成员负载零拷贝切片（BSD 长名成员已剥离前置文件名）
    data: []const u8,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return strEq(s[0..prefix.len], prefix);
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && s[start] == ' ') {
        start += 1;
    }
    let end = s.len;
    while (end > start && s[end - 1] == ' ') {
        end -= 1;
    }
    return s[start..end];
}

fn trimTrailingNuls(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && s[end - 1] == 0) {
        end -= 1;
    }
    return s[0..end];
}

fn parseDecimalU64(raw: []const u8, allow_empty: bool): u64? {
    const s = trimSpaces(raw);
    if (s.len == 0) {
        if (allow_empty) return 0;
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = (c - '0') as u64;
        if (acc > 1844674407370955161) return null;
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

fn parseOctalU32(raw: []const u8, allow_empty: bool): u32? {
    const s = trimSpaces(raw);
    if (s.len == 0) {
        if (allow_empty) return 0;
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '7') return null;
        acc = (acc << 3) | ((c - '0') as u64);
        if (acc > 0xFFFFFFFF) return null;
        i += 1;
    }
    return acc as u32;
}

fn lookupGnuLongName(strtab: []const u8, offset: usize): ([]const u8)? {
    if (offset >= strtab.len) return null;
    let end = offset;
    while (end < strtab.len) {
        const c = strtab[end];
        if (c == '/' || c == '\n' || c == 0) break;
        end += 1;
    }
    return strtab[offset..end];
}

fn isBsdSymtabName(name: []const u8): bool {
    return strEq(name, "__.SYMDEF") || strEq(name, "__.SYMDEF SORTED");
}

/// 校验数据是否以 8 字节 Unix `ar` 魔数 `"!<arch>\n"` 开头
pub fn isAr(data: []const u8): bool {
    if (data.len < 8) return false;
    return strEq(data[0..8], AR_MAGIC);
}

/// 零堆分配 Unix `ar` / `.a` / `.deb` 成员流式迭代器（支持 GNU `"//"` 与 BSD `"#1/<len>"` 长文件名）
pub struct ArIter {
    data: []const u8,
    pos: usize = 8,
    gnu_strtab: ([]const u8)? = null,
    had_error: bool = false,

    pub static fn init(data: []const u8): ArIter {
        if (!isAr(data)) {
            return ArIter {
                data: data,
                pos: 0,
                gnu_strtab: null,
                had_error: true,
            };
        }
        return ArIter {
            data: data,
            pos: 8,
            gnu_strtab: null,
            had_error: false,
        };
    }

    /// 读取下一归档成员；到达归档末尾返回 `null`，遇到截断/缺少 `` `\n ``/越界 `size` 时置 `had_error = true` 并返回 `null`
    pub fn next(*_): ArMember? {
        if (_.had_error || _.pos >= _.data.len) return null;
        if (_.data.len - _.pos < AR_HEADER_SIZE) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const h = _.pos;
        if (_.data[h + 58] != '`' || _.data[h + 59] != '\n') {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const raw_name = trimSpaces(_.data[h .. h + 16]);
        const mtime = parseDecimalU64(_.data[h + 16 .. h + 28], true) ?? 0;
        const uid_u64 = parseDecimalU64(_.data[h + 28 .. h + 34], true) ?? 0;
        const gid_u64 = parseDecimalU64(_.data[h + 34 .. h + 40], true) ?? 0;
        const mode = parseOctalU32(_.data[h + 40 .. h + 48], true) ?? 0;

        let raw_size: u64 = 0;
        if (sz := parseDecimalU64(_.data[h + 48 .. h + 58], false)) {
            raw_size = sz;
        } else {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const data_start = h + AR_HEADER_SIZE;
        if (raw_size > ((_.data.len - data_start) as u64)) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const size_usize: usize = raw_size as usize;
        let payload = _.data[data_start .. data_start + size_usize];
        _.pos = data_start + size_usize;

        // 2-byte even alignment padding after odd-sized members
        if ((size_usize & 1) == 1 && _.pos < _.data.len) {
            _.pos += 1;
        }

        // Resolve member kind and name (GNU symtab/strtab/longname, BSD #1/<len>, or normal)
        let kind = ArMemberKind.normal;
        let resolved_name = raw_name;
        let resolved_size = raw_size;

        if (strEq(raw_name, "/")) {
            kind = ArMemberKind.gnu_symtab;
        } else if (strEq(raw_name, "//")) {
            kind = ArMemberKind.gnu_strtab;
            _.gnu_strtab = payload;
        } else if (startsWith(raw_name, "#1/")) {
            // BSD long filename: `#1/<name_len>` followed by name bytes inside payload
            let bsd_len: u64 = 0;
            if (bl := parseDecimalU64(raw_name[3..raw_name.len], false)) {
                bsd_len = bl;
            } else {
                _.had_error = true;
                _.pos = _.data.len;
                return null;
            }
            if (bsd_len > (payload.len as u64)) {
                _.had_error = true;
                _.pos = _.data.len;
                return null;
            }
            const bl_usize: usize = bsd_len as usize;
            resolved_name = trimTrailingNuls(payload[0..bl_usize]);
            payload = payload[bl_usize..payload.len];
            resolved_size = payload.len as u64;
            if (isBsdSymtabName(resolved_name)) {
                kind = ArMemberKind.bsd_symtab;
            }
        } else if (raw_name.len >= 2 && raw_name[0] == '/' && raw_name[1] >= '0' && raw_name[1] <= '9') {
            // GNU long filename table reference: `/<offset>`
            if (off_u64 := parseDecimalU64(raw_name[1..raw_name.len], false)) {
                if (strtab := _.gnu_strtab) {
                    if (long_name := lookupGnuLongName(strtab, off_u64 as usize)) {
                        resolved_name = long_name;
                    }
                }
            }
            if (isBsdSymtabName(resolved_name)) {
                kind = ArMemberKind.bsd_symtab;
            }
        } else {
            // Standard short name: strip trailing GNU '/' delimiter if present
            if (resolved_name.len > 1 && resolved_name[resolved_name.len - 1] == '/') {
                resolved_name = resolved_name[0 .. resolved_name.len - 1];
            }
            if (isBsdSymtabName(resolved_name)) {
                kind = ArMemberKind.bsd_symtab;
            }
        }

        return ArMember {
            raw_name: raw_name,
            name: resolved_name,
            mtime: mtime,
            uid: uid_u64 as u32,
            gid: gid_u64 as u32,
            mode: mode,
            size: resolved_size,
            kind: kind,
            data: payload,
        };
    }
}

/// 按解析后的成员名称在归档中查找首个匹配成员
pub fn findArMember(data: []const u8, name: []const u8): ArMember? {
    let it = ArIter.init(data);
    while (member := it.next()) {
        if (strEq(member.name, name)) return member;
    }
    return null;
}

/// 统计合法 `ar` 归档中的成员总数（非法归档返回 0）
pub fn countArMembers(data: []const u8): usize {
    let it = ArIter.init(data);
    let count: usize = 0;
    while (it.next() != null) {
        count += 1;
    }
    return count;
}
