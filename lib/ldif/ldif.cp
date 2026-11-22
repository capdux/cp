// Origin: Pure-CP Stdlib (Zero-Alloc RFC 2849 LDAP Data Interchange Format .ldif Parser) | Created: 2026-11-22

/// LDIF 属性值编码类型（`: ` 普通文本、`:: ` Base64、`:< ` URL）
pub enum LdifValueKind: u8 {
    plain,
    base64,
    url,
}

/// LDIF 记录变更类型（普通目录条目或 RFC 2849 changetype）
pub enum LdifChangeType: u8 {
    content,
    add,
    delete,
    modify,
    modrdn,
    moddn,
}

/// 单条 LDIF 属性（零拷贝切片，保留原始续行折行序列）
pub struct LdifAttr {
    name: []const u8,
    raw_value: []const u8,
    kind: LdifValueKind,

    /// 按 RFC 2849 规则剥离续行折行序列（\r\n<SP> 或 \n<SP>）并写入 out，返回写入字节数；缓冲区不足返回 null
    fn unfoldValue(out: []u8): usize? {
        const src = _.raw_value;
        let i: usize = 0;
        let w: usize = 0;
        while (i < src.len) {
            if (src[i] == '\r' && i + 2 < src.len && src[i + 1] == '\n' && src[i + 2] == ' ') {
                i += 3;
                continue;
            }
            if (src[i] == '\n' && i + 1 < src.len && src[i + 1] == ' ') {
                i += 2;
                continue;
            }
            if (w >= out.len) {
                return null;
            }
            out[w] = src[i];
            w += 1;
            i += 1;
        }
        return w;
    }
}

/// 单条 LDIF 目录条目或变更记录（零拷贝切片）
pub struct LdifEntry {
    dn: []const u8,
    dn_is_base64: bool,
    change_type: LdifChangeType,
    raw_record: []const u8,
    attrs_offset: usize,

    /// 返回该条记录（dn 之后）全部属性的零分配迭代器
    fn attrs(): LdifAttrIter {
        const off = if (_.attrs_offset <= _.raw_record.len) _.attrs_offset else _.raw_record.len;
        return LdifAttrIter.init(_.raw_record[off.._.raw_record.len]);
    }

    /// 大小写不敏感查找首个匹配的 LDAP 属性
    fn findAttr(name: []const u8): LdifAttr? {
        let it = _.attrs();
        while (attr := it.next()) {
            if (eqlIgnoreCase(attr.name, name)) {
                return attr;
            }
        }
        return null;
    }
}

/// LDIF 单遍统计汇总
pub struct LdifStats {
    version: u32,
    entry_count: u32,
    add_count: u32,
    modify_count: u32,
    delete_count: u32,
    attr_count: u32,
    base64_attr_count: u32,
}

// ---------------------------------------------------------------------------
// 内部零分配辅助函数
// ---------------------------------------------------------------------------

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpaceOrTab(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpaceOrTab(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
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

fn parseU32Strict(raw: []const u8): u32? {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return null;
        }
        acc = acc * 10 + u64(s[i] - '0');
        if (acc > 4294967295) {
            return null;
        }
        i += 1;
    }
    return acc as u32;
}

/// 读取单条物理行（不含 \r\n），推进 pos
fn nextPhysicalLine(data: []const u8, pos: *usize): ([]const u8)? {
    const start = pos.*;
    if (start >= data.len) {
        return null;
    }
    let i = start;
    while (i < data.len && data[i] != '\n') {
        i += 1;
    }
    let end = i;
    if (end > start && data[end - 1] == '\r') {
        end -= 1;
    }
    pos.* = if (i < data.len) (i + 1) else data.len;
    return data[start..end];
}

/// 读取一条合并了单空格续行的逻辑行切片（不含末尾换行），推进 pos
fn nextLogicalLine(data: []const u8, pos: *usize): ([]const u8)? {
    if (pos.* >= data.len) {
        return null;
    }
    const start = pos.*;
    let line_end = start;

    // 读取首行
    let i = start;
    while (i < data.len && data[i] != '\n') {
        i += 1;
    }
    line_end = i;
    if (line_end > start && data[line_end - 1] == '\r') {
        line_end -= 1;
    }
    pos.* = if (i < data.len) (i + 1) else data.len;

    // 合并后续所有以单个 ' ' 开头的 RFC 2849 折行续行
    while (pos.* < data.len && data[pos.*] == ' ') {
        let j = pos.*;
        while (j < data.len && data[j] != '\n') {
            j += 1;
        }
        line_end = j;
        if (line_end > pos.* && data[line_end - 1] == '\r') {
            line_end -= 1;
        }
        pos.* = if (j < data.len) (j + 1) else data.len;
    }

    return data[start..line_end];
}

/// 将单条逻辑行解析为 LdifAttr
fn parseAttrLogicalLine(line: []const u8): LdifAttr? {
    // 仅在第一行内查找 ':'
    let colon: usize = 0;
    let found_colon = false;
    while (colon < line.len && line[colon] != '\r' && line[colon] != '\n') {
        if (line[colon] == ':') {
            found_colon = true;
            break;
        }
        colon += 1;
    }
    if (!found_colon || colon == 0) {
        return null;
    }

    const name = trimAscii(line[0..colon]);
    if (name.len == 0) {
        return null;
    }

    let kind = LdifValueKind.plain;
    let val_start = colon + 1;
    if (val_start < line.len && line[val_start] == ':') {
        kind = LdifValueKind.base64;
        val_start += 1;
    } else if (val_start < line.len && line[val_start] == '<') {
        kind = LdifValueKind.url;
        val_start += 1;
    }

    // RFC 2849: SEP 后的可选空格
    while (val_start < line.len && line[val_start] == ' ') {
        val_start += 1;
    }

    return LdifAttr{
        name: name,
        raw_value: line[val_start..line.len],
        kind: kind,
    };
}

fn parseChangeTypeValue(val: []const u8): LdifChangeType {
    const s = trimAscii(val);
    if (eqlIgnoreCase(s, "add")) return LdifChangeType.add;
    if (eqlIgnoreCase(s, "delete")) return LdifChangeType.delete;
    if (eqlIgnoreCase(s, "modify")) return LdifChangeType.modify;
    if (eqlIgnoreCase(s, "modrdn")) return LdifChangeType.modrdn;
    if (eqlIgnoreCase(s, "moddn")) return LdifChangeType.moddn;
    return LdifChangeType.content;
}

// ---------------------------------------------------------------------------
// 公开 API：属性迭代器、记录迭代器与单遍统计
// ---------------------------------------------------------------------------

/// 零分配迭代单条 LDIF 记录内的全部属性（合并续行、跳过 # 注释与 modify 中的 "-" 分隔行）
pub struct LdifAttrIter {
    data: []const u8,
    pos: usize,
    malformed: bool,

    fn init(slice: []const u8): LdifAttrIter {
        return LdifAttrIter{
            data: slice,
            pos: 0,
            malformed: false,
        };
    }

    fn next(*_): LdifAttr? {
        while (line := nextLogicalLine(_.data, *_.pos)) {
            if (line.len == 0) {
                continue;
            }
            if (line[0] == '#') {
                continue;
            }
            // modify 操作分隔行 "-"
            if (line.len == 1 && line[0] == '-') {
                continue;
            }
            if (trimAscii(line).len == 0) {
                continue;
            }
            if (attr := parseAttrLogicalLine(line)) {
                return attr;
            }
            _.malformed = true;
        }
        return null;
    }
}

/// 零分配按空行分隔迭代多条 LDIF 记录（自动解析顶部可选 version: 1，跳过 # 注释与多余空行）
pub struct LdifIter {
    data: []const u8,
    pos: usize,
    version: u32,
    seen_entry: bool,

    fn init(data: []const u8): LdifIter {
        return LdifIter{
            data: data,
            pos: 0,
            version: 0,
            seen_entry: false,
        };
    }

    fn next(*_): LdifEntry? {
        while (_.pos < _.data.len) {
            // 跳过前导空行
            while (_.pos < _.data.len) {
                const probe = _.pos;
                const line = nextPhysicalLine(_.data, *_.pos) ?? break;
                if (trimAscii(line).len > 0) {
                    _.pos = probe;
                    break;
                }
            }
            if (_.pos >= _.data.len) {
                return null;
            }

            // 切出连续非空行组成的记录块
            const block_start = _.pos;
            let block_end = block_start;
            while (_.pos < _.data.len) {
                const line_start = _.pos;
                const line = nextPhysicalLine(_.data, *_.pos) ?? break;
                if (trimAscii(line).len == 0) {
                    block_end = line_start;
                    break;
                }
                block_end = _.pos;
            }

            const block = _.data[block_start..block_end];
            if (block.len == 0) {
                continue;
            }

            // 在块内逐逻辑行定位首个非注释行（处理可选 version: N 与必需 dn:）
            let bpos: usize = 0;
            let rec_start_in_block: usize = 0;
            let found_dn = false;
            let dn_slice = block[0..0];
            let dn_b64 = false;
            let attrs_off: usize = 0;

            while (bpos < block.len) {
                const line_start = bpos;
                const line = nextLogicalLine(block, *bpos) ?? break;
                if (line.len == 0 || line[0] == '#') {
                    continue;
                }
                if (attr := parseAttrLogicalLine(line)) {
                    if (!_.seen_entry && _.version == 0 && eqlIgnoreCase(attr.name, "version")) {
                        if (v := parseU32Strict(attr.raw_value)) {
                            _.version = v;
                        }
                        continue;
                    }
                    if (eqlIgnoreCase(attr.name, "dn")) {
                        rec_start_in_block = line_start;
                        dn_slice = attr.raw_value;
                        dn_b64 = attr.kind == LdifValueKind.base64;
                        attrs_off = bpos - line_start;
                        found_dn = true;
                        break;
                    }
                }
                // 首个有效属性不是 version 也不是 dn -> 畸形块，跳过
                break;
            }

            if (!found_dn) {
                continue;
            }

            _.seen_entry = true;
            const raw_rec = block[rec_start_in_block..block.len];

            // 检测 changetype 属性
            let change_type = LdifChangeType.content;
            let ait = LdifAttrIter.init(raw_rec[attrs_off..raw_rec.len]);
            while (a := ait.next()) {
                if (eqlIgnoreCase(a.name, "changetype")) {
                    change_type = parseChangeTypeValue(a.raw_value);
                    break;
                }
            }

            return LdifEntry{
                dn: dn_slice,
                dn_is_base64: dn_b64,
                change_type: change_type,
                raw_record: raw_rec,
                attrs_offset: attrs_off,
            };
        }
        return null;
    }
}

/// 快速检测缓冲区是否包含合法的 LDIF 记录
pub fn isLdif(data: []const u8): bool {
    let it = LdifIter.init(data);
    return it.next() != null;
}

/// 单遍统计 LDIF 文档的版本号、条目数、各类变更数及属性/Base64 属性数量
pub fn summarizeLdif(data: []const u8): LdifStats {
    let stats = LdifStats{
        version: 0,
        entry_count: 0,
        add_count: 0,
        modify_count: 0,
        delete_count: 0,
        attr_count: 0,
        base64_attr_count: 0,
    };

    let it = LdifIter.init(data);
    while (entry := it.next()) {
        stats.entry_count += 1;
        if (entry.change_type == LdifChangeType.add) {
            stats.add_count += 1;
        } else if (entry.change_type == LdifChangeType.modify) {
            stats.modify_count += 1;
        } else if (entry.change_type == LdifChangeType.delete) {
            stats.delete_count += 1;
        }

        let ait = entry.attrs();
        while (attr := ait.next()) {
            stats.attr_count += 1;
            if (attr.kind == LdifValueKind.base64) {
                stats.base64_attr_count += 1;
            }
        }
    }
    stats.version = it.version;
    return stats;
}
