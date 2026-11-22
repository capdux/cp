// Origin: Pure-CP Stdlib (Zero-Alloc Stanford PLY 3D Mesh & Point Cloud Parser) | Created: 2026-11-22

/// Stanford PLY 数据编码格式
pub enum PlyFormat {
    ascii,
    binary_little_endian,
    binary_big_endian,
    unknown,
}

/// Stanford PLY 文件头摘要（切片零拷贝指向原始输入）
pub struct PlyHeader {
    format: PlyFormat,
    version: []const u8,
    vertex_count: u64,
    face_count: u64,
    element_count: u32,
    data_offset: usize,
}

/// Stanford PLY 元素定义及其下属属性计数
pub struct PlyElement {
    name: []const u8,
    count: u64,
    property_count: u32,
}

fn isHoriSpace(c: u8): bool {
    return c == ' ' || c == '\t';
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

fn trimHori(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isHoriSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isHoriSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

struct LineSpan {
    line: []const u8,
    next_pos: usize,
}

fn nextLine(data: []const u8, pos: usize): LineSpan? {
    if (pos >= data.len) return null;
    let i = pos;
    while (i < data.len && data[i] != '\n') {
        i += 1;
    }
    let line_end = i;
    if (line_end > pos && data[line_end - 1] == '\r') {
        line_end -= 1;
    }
    const next_pos: usize = if (i < data.len) i + 1 else data.len;
    return LineSpan {
        line: data[pos..line_end],
        next_pos: next_pos,
    };
}

struct WordSpan {
    word: []const u8,
    next_pos: usize,
}

fn nextWord(line: []const u8, pos: usize): WordSpan? {
    let i = pos;
    while (i < line.len && isHoriSpace(line[i])) {
        i += 1;
    }
    if (i >= line.len) return null;
    const start = i;
    while (i < line.len && !isHoriSpace(line[i])) {
        i += 1;
    }
    return WordSpan {
        word: line[start..i],
        next_pos: i,
    };
}

fn parseU64Dec(s: []const u8): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    const max_div10: u64 = 1844674407370955161;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) return null;
        const d = u64(s[i] - '0');
        if (acc > max_div10 || (acc == max_div10 && d > 5)) {
            return null;
        }
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

/// 快速校验首行是否为 Stanford PLY 魔数 `"ply\n"` 或 `"ply\r\n"`
pub fn isPly(data: []const u8): bool {
    if (data.len >= 4 && data[0] == 'p' && data[1] == 'l' && data[2] == 'y') {
        if (data[3] == '\n') return true;
        if (data.len >= 5 && data[3] == '\r' && data[4] == '\n') return true;
    }
    return false;
}

/// 解析 PLY 文件头（`format` / `element` / `property` / `comment` 直至 `end_header`）；缺失 `format` 或未闭合 `end_header` 返回 `null`
pub fn parsePlyHeader(data: []const u8): PlyHeader? {
    if (!isPly(data)) {
        return null;
    }

    let pos: usize = 0;
    if (first := nextLine(data, 0)) {
        pos = first.next_pos;
    } else {
        return null;
    }

    let has_format = false;
    let format = PlyFormat.unknown;
    let version: []const u8 = "";
    let vertex_count: u64 = 0;
    let face_count: u64 = 0;
    let element_count: u32 = 0;

    while (span := nextLine(data, pos)) {
        pos = span.next_pos;
        const clean = trimHori(span.line);
        if (clean.len == 0) {
            continue;
        }

        let kw = WordSpan { word: "", next_pos: 0 };
        if (w := nextWord(clean, 0)) {
            kw = w;
        } else {
            continue;
        }

        if (eqlBytes(kw.word, "end_header")) {
            if (!has_format) {
                return null;
            }
            return PlyHeader {
                format: format,
                version: version,
                vertex_count: vertex_count,
                face_count: face_count,
                element_count: element_count,
                data_offset: pos,
            };
        }

        if (eqlBytes(kw.word, "comment") || eqlBytes(kw.word, "obj_info")) {
            continue;
        }

        if (eqlBytes(kw.word, "format")) {
            if (has_format) {
                return null;
            }
            let fmt_w = WordSpan { word: "", next_pos: 0 };
            if (w1 := nextWord(clean, kw.next_pos)) {
                fmt_w = w1;
            } else {
                return null;
            }
            let ver_w = WordSpan { word: "", next_pos: 0 };
            if (w2 := nextWord(clean, fmt_w.next_pos)) {
                ver_w = w2;
            } else {
                return null;
            }

            if (eqlBytes(fmt_w.word, "ascii")) {
                format = PlyFormat.ascii;
            } else if (eqlBytes(fmt_w.word, "binary_little_endian")) {
                format = PlyFormat.binary_little_endian;
            } else if (eqlBytes(fmt_w.word, "binary_big_endian")) {
                format = PlyFormat.binary_big_endian;
            } else {
                return null;
            }
            version = ver_w.word;
            has_format = true;
            continue;
        }

        if (eqlBytes(kw.word, "element")) {
            let name_w = WordSpan { word: "", next_pos: 0 };
            if (w1 := nextWord(clean, kw.next_pos)) {
                name_w = w1;
            } else {
                return null;
            }
            let cnt_w = WordSpan { word: "", next_pos: 0 };
            if (w2 := nextWord(clean, name_w.next_pos)) {
                cnt_w = w2;
            } else {
                return null;
            }
            let cnt: u64 = 0;
            if (c := parseU64Dec(cnt_w.word)) {
                cnt = c;
            } else {
                return null;
            }

            element_count += 1;
            if (eqlBytes(name_w.word, "vertex")) {
                vertex_count = cnt;
            } else if (eqlBytes(name_w.word, "face")) {
                face_count = cnt;
            }
            continue;
        }

        if (eqlBytes(kw.word, "property")) {
            if (element_count == 0) {
                return null;
            }
            continue;
        }
    }

    return null;
}

/// 零堆分配 PLY 元素流式迭代器（逐个产出 `element <name> <count>` 并统计下属 `property` 数量）
pub struct PlyElementIter {
    data: []const u8,
    pos: usize = 0,
    header_end: usize = 0,

    static fn init(data: []const u8): PlyElementIter {
        if (hdr := parsePlyHeader(data)) {
            if (first := nextLine(data, 0)) {
                return PlyElementIter {
                    data: data,
                    pos: first.next_pos,
                    header_end: hdr.data_offset,
                };
            }
        }
        return PlyElementIter {
            data: data,
            pos: 0,
            header_end: 0,
        };
    }

    /// 读取下一条 `PlyElement`；遍历结束返回 `null`
    fn next(*_): PlyElement? {
        // 1. 查找下一条 `element <name> <count>`
        let elem_name: []const u8 = "";
        let elem_count: u64 = 0;
        let found = false;

        while (_.pos < _.header_end) {
            if (span := nextLine(_.data, _.pos)) {
                _.pos = span.next_pos;
                const clean = trimHori(span.line);
                if (clean.len == 0) continue;
                if (kw := nextWord(clean, 0)) {
                    if (eqlBytes(kw.word, "end_header")) {
                        _.pos = _.header_end;
                        return null;
                    }
                    if (eqlBytes(kw.word, "element")) {
                        if (name_w := nextWord(clean, kw.next_pos)) {
                            if (cnt_w := nextWord(clean, name_w.next_pos)) {
                                if (cnt := parseU64Dec(cnt_w.word)) {
                                    elem_name = name_w.word;
                                    elem_count = cnt;
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            } else {
                break;
            }
        }

        if (!found) {
            return null;
        }

        // 2. 统计紧随其后的 `property` 数量，遇到下一个 `element` 或 `end_header` 停住
        let prop_count: u32 = 0;
        while (_.pos < _.header_end) {
            if (span := nextLine(_.data, _.pos)) {
                const clean = trimHori(span.line);
                if (clean.len == 0) {
                    _.pos = span.next_pos;
                    continue;
                }
                if (kw := nextWord(clean, 0)) {
                    if (eqlBytes(kw.word, "element") || eqlBytes(kw.word, "end_header")) {
                        break;
                    }
                    if (eqlBytes(kw.word, "property")) {
                        prop_count += 1;
                    }
                }
                _.pos = span.next_pos;
            } else {
                break;
            }
        }

        return PlyElement {
            name: elem_name,
            count: elem_count,
            property_count: prop_count,
        };
    }
}

/// 返回 PLY 标准标量类型字节宽度（1 / 2 / 4 / 8）；未知类型返回 `null`
pub fn plyScalarSize(type_name: []const u8): u8? {
    if (eqlBytes(type_name, "char")
        || eqlBytes(type_name, "uchar")
        || eqlBytes(type_name, "int8")
        || eqlBytes(type_name, "uint8")) {
        return 1;
    }
    if (eqlBytes(type_name, "short")
        || eqlBytes(type_name, "ushort")
        || eqlBytes(type_name, "int16")
        || eqlBytes(type_name, "uint16")) {
        return 2;
    }
    if (eqlBytes(type_name, "int")
        || eqlBytes(type_name, "uint")
        || eqlBytes(type_name, "float")
        || eqlBytes(type_name, "int32")
        || eqlBytes(type_name, "uint32")
        || eqlBytes(type_name, "float32")) {
        return 4;
    }
    if (eqlBytes(type_name, "double") || eqlBytes(type_name, "float64")) {
        return 8;
    }
    return null;
}
