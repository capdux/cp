// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Debian .deb & RFC 822 Control Parser)

/// 标准 Unix ar 归档全局魔数（8 字节）
pub const DEB_AR_MAGIC: []const u8 = "!<arch>\n";

/// Debian `.deb` 首个必选成员名
pub const DEB_BINARY_MEMBER: []const u8 = "debian-binary";

/// `.deb` 内嵌 `control.tar*` / `data.tar*` 压缩算法枚举
pub enum DebCompression: u8 {
    none = 0,
    gzip = 1,
    xz = 2,
    zstd = 3,
    bzip2 = 4,
    lzma = 5,
    unknown = 6,
}

/// 零拷贝 `.deb` 软件包三大核心成员视图
pub struct DebPackage {
    format_version: []const u8,
    control_name: []const u8,
    control_compression: DebCompression,
    control_data: []const u8,
    data_name: []const u8,
    data_compression: DebCompression,
    data_data: []const u8,
}

/// 零拷贝 RFC 822 / Debian `control` 单字段视图（支持多行续行）
pub struct DebControlField {
    name: []const u8,
    value: []const u8,
}

/// 零堆分配 RFC 822 / Debian `control` 字段流式迭代器
pub struct DebControlIter {
    data: []const u8,
    pos: usize = 0,

    static fn init(control_text: []const u8): DebControlIter {
        return DebControlIter {
            data: control_text,
            pos: 0,
        };
    }

    /// 读取下一条 `Key: Value` 字段（自动合并以空格/制表符开头的续行，忽略空行与 `#` 注释）
    fn next(*_): DebControlField? {
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            const line_end = findLineEnd(_.data, line_start);
            const next_pos = advancePastNewline(_.data, line_end);
            _.pos = next_pos;

            // 去掉行尾 '\r'
            const trimmed_end = trimTrailingCr(_.data, line_start, line_end);
            if (trimmed_end == line_start) {
                continue; // 空行
            }
            const first_ch = _.data[line_start];
            if (first_ch == '#') {
                continue; // 注释行
            }
            if (first_ch == ' ' || first_ch == '\t') {
                continue; // 孤立续行跳过
            }

            // 查找首个 ':'
            let colon: usize = trimmed_end;
            let i: usize = line_start;
            while (i < trimmed_end) {
                if (_.data[i] == ':') {
                    colon = i;
                    break;
                }
                i += 1;
            }
            if (colon >= trimmed_end) {
                continue; // 非 Key: Value 行跳过
            }

            const name = trimAsciiSpace(_.data[line_start..colon]);
            let val_start: usize = colon + 1;
            while (val_start < trimmed_end && (_.data[val_start] == ' ' || _.data[val_start] == '\t')) {
                val_start += 1;
            }
            let val_end: usize = trimmed_end;

            // 合并后续以空格或制表符开头的续行（零拷贝延伸切片终点）
            while (_.pos < _.data.len) {
                const c_start = _.pos;
                const c_end = findLineEnd(_.data, c_start);
                const c_trimmed = trimTrailingCr(_.data, c_start, c_end);
                if (c_trimmed == c_start) {
                    break; // 空行结束当前字段
                }
                const ch = _.data[c_start];
                if (ch != ' ' && ch != '\t') {
                    break;
                }
                val_end = c_trimmed;
                _.pos = advancePastNewline(_.data, c_end);
            }

            return DebControlField {
                name: name,
                value: _.data[val_start..val_end],
            };
        }
        return null;
    }
}

/// 一键提取的 Debian `DEBIAN/control` 核心元数据视图
pub struct DebControlMeta {
    package: []const u8,
    version: []const u8,
    architecture: []const u8,
    maintainer: []const u8,
    depends: []const u8,
    section: []const u8,
    priority: []const u8,
    installed_size: u64,
    description: []const u8,

    /// 检查必需的 `Package`、`Version` 与 `Architecture` 字段是否齐全
    fn isValid(): bool {
        return _.package.len > 0 && _.version.len > 0 && _.architecture.len > 0;
    }
}

struct ArMember {
    name: []const u8,
    payload: []const u8,
    next_pos: usize,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
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
    return bytesEql(s[0..prefix.len], prefix);
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn trimAsciiSpace(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn findLineEnd(data: []const u8, start: usize): usize {
    let i = start;
    while (i < data.len && data[i] != '\n') {
        i += 1;
    }
    return i;
}

fn advancePastNewline(data: []const u8, line_end: usize): usize {
    if (line_end < data.len && data[line_end] == '\n') {
        return line_end + 1;
    }
    return line_end;
}

fn trimTrailingCr(data: []const u8, start: usize, end: usize): usize {
    if (end > start && data[end - 1] == '\r') {
        return end - 1;
    }
    return end;
}

fn parseDecimalU64(s: []const u8): u64? {
    const t = trimAsciiSpace(s);
    if (t.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < t.len) {
        const ch = t[i];
        if (ch < '0' || ch > '9') return null;
        const digit: u64 = (ch - '0') as u64;
        if (acc > 1844674407370955161) return null;
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn parseArMember(data: []const u8, pos: usize): ArMember? {
    if (data.len - pos < 60) return null;
    // 校验 60B ar 头结尾魔数 "`\n" (0x60, 0x0A)
    if (data[pos + 58] != 0x60 || data[pos + 59] != 0x0A) return null;

    // 解析成员名（0..16，去掉尾空格与可选单个 GNU 尾斜杠 '/'）
    let name_end: usize = 16;
    while (name_end > 0 && data[pos + name_end - 1] == ' ') {
        name_end -= 1;
    }
    if (name_end > 0 && data[pos + name_end - 1] == '/') {
        name_end -= 1;
    }
    const name = data[pos .. pos + name_end];

    // 解析成员字节大小（48..58）
    const size_u64 = parseDecimalU64(data[pos + 48 .. pos + 58]) ?? return null;
    const size: usize = size_u64 as usize;
    const payload_start: usize = pos + 60;
    if (size > data.len - payload_start) return null;

    const payload_end: usize = payload_start + size;
    let next_pos: usize = payload_end;
    if ((size & 1) != 0 && next_pos < data.len) {
        next_pos += 1; // ar 2 字节偶对齐填充
    }

    return ArMember {
        name: name,
        payload: data[payload_start..payload_end],
        next_pos: next_pos,
    };
}

/// 根据 `control.tar*` / `data.tar*` 后缀识别压缩格式
pub fn detectDebCompression(member_name: []const u8, base_prefix: []const u8): DebCompression? {
    if (!startsWith(member_name, base_prefix)) return null;
    const ext = member_name[base_prefix.len .. member_name.len];
    if (ext.len == 0) return DebCompression.none;
    if (bytesEql(ext, ".gz")) return DebCompression.gzip;
    if (bytesEql(ext, ".xz")) return DebCompression.xz;
    if (bytesEql(ext, ".zst")) return DebCompression.zstd;
    if (bytesEql(ext, ".bz2")) return DebCompression.bzip2;
    if (bytesEql(ext, ".lzma")) return DebCompression.lzma;
    if (ext[0] == '.') return DebCompression.unknown;
    return null;
}

/// 探测缓冲区是否为合法 Debian `.deb` 软件包（校验 ar 魔数、首成员 `debian-binary` 与版本 `2.0`）
pub fn isDeb(data: []const u8): bool {
    if (data.len < 68) return false;
    if (!startsWith(data, DEB_AR_MAGIC)) return false;
    if (first := parseArMember(data, 8)) {
        if (!bytesEql(first.name, DEB_BINARY_MEMBER)) return false;
        const ver = trimAsciiSpace(first.payload);
        return startsWith(ver, "2.0");
    }
    return false;
}

/// 零拷贝解析 `.deb` 软件包，提取 `debian-binary`、`control.tar*` 与 `data.tar*`
pub fn parseDebPackage(data: []const u8): DebPackage? {
    if (data.len < 68) return null;
    if (!startsWith(data, DEB_AR_MAGIC)) return null;

    const first = parseArMember(data, 8) ?? return null;
    if (!bytesEql(first.name, DEB_BINARY_MEMBER)) return null;
    const ver = trimAsciiSpace(first.payload);
    if (!startsWith(ver, "2.0")) return null;

    let pos: usize = first.next_pos;
    let found_control: bool = false;
    let control_name: []const u8 = data[0..0];
    let control_comp: DebCompression = DebCompression.unknown;
    let control_data: []const u8 = data[0..0];

    let found_data: bool = false;
    let data_name: []const u8 = data[0..0];
    let data_comp: DebCompression = DebCompression.unknown;
    let data_data: []const u8 = data[0..0];

    while (pos < data.len) {
        const m = parseArMember(data, pos) ?? return null;
        pos = m.next_pos;

        if (!found_control) {
            if (comp := detectDebCompression(m.name, "control.tar")) {
                found_control = true;
                control_name = m.name;
                control_comp = comp;
                control_data = m.payload;
                continue;
            }
        }
        if (!found_data) {
            if (comp := detectDebCompression(m.name, "data.tar")) {
                found_data = true;
                data_name = m.name;
                data_comp = comp;
                data_data = m.payload;
                continue;
            }
        }
    }

    if (!found_control || !found_data) return null;

    return DebPackage {
        format_version: ver,
        control_name: control_name,
        control_compression: control_comp,
        control_data: control_data,
        data_name: data_name,
        data_compression: data_comp,
        data_data: data_data,
    };
}

/// 零拷贝解析 `DEBIAN/control` 文本，大小写不敏感提取核心元数据字段
pub fn parseDebControl(control_text: []const u8): DebControlMeta {
    const empty = control_text[0..0];
    let meta = DebControlMeta {
        package: empty,
        version: empty,
        architecture: empty,
        maintainer: empty,
        depends: empty,
        section: empty,
        priority: empty,
        installed_size: 0,
        description: empty,
    };

    let it = DebControlIter.init(control_text);
    while (f := it.next()) {
        if (eqlIgnoreCase(f.name, "Package")) {
            meta.package = f.value;
        } else if (eqlIgnoreCase(f.name, "Version")) {
            meta.version = f.value;
        } else if (eqlIgnoreCase(f.name, "Architecture")) {
            meta.architecture = f.value;
        } else if (eqlIgnoreCase(f.name, "Maintainer")) {
            meta.maintainer = f.value;
        } else if (eqlIgnoreCase(f.name, "Depends")) {
            meta.depends = f.value;
        } else if (eqlIgnoreCase(f.name, "Section")) {
            meta.section = f.value;
        } else if (eqlIgnoreCase(f.name, "Priority")) {
            meta.priority = f.value;
        } else if (eqlIgnoreCase(f.name, "Installed-Size")) {
            if (sz := parseDecimalU64(f.value)) {
                meta.installed_size = sz;
            }
        } else if (eqlIgnoreCase(f.name, "Description")) {
            meta.description = f.value;
        }
    }

    return meta;
}
