// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Khronos COLLADA 1.4/1.5 3D Asset XML Parser)

/// COLLADA `<asset>` 元数据与根版本零拷贝视图
pub struct DaeAsset {
    version: []const u8 = "",
    author: []const u8 = "",
    authoring_tool: []const u8 = "",
    created: []const u8 = "",
    modified: []const u8 = "",
    unit_name: []const u8 = "",
    unit_meter: f64 = 1.0,
    up_axis: []const u8 = "Y_UP",
}

/// `<library_geometries>` 中的单个 `<geometry>` 零拷贝视图
pub struct DaeGeometry {
    id: []const u8,
    name: []const u8,
    raw_xml: []const u8,
}

/// 几何体内的 `<float_array id="..." count="...">...</float_array>` 零拷贝视图
pub struct DaeFloatArray {
    id: []const u8,
    declared_count: u32,
    raw_values: []const u8,
}

/// `<library_visual_scenes>` 中的单个 `<node>` 零拷贝视图
pub struct DaeSceneNode {
    id: []const u8,
    name: []const u8,
    node_type: []const u8,
    instance_geometry_url: []const u8,
}

/// COLLADA 3D 资产统计汇总
pub struct DaeStats {
    geometry_count: usize = 0,
    float_array_count: usize = 0,
    total_floats: usize = 0,
    node_count: usize = 0,
    material_count: usize = 0,
}

fn isXmlSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isTagDelim(c: u8): bool {
    return isXmlSpace(c) || c == '>' || c == '/';
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (haystack[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn localName(qname: []const u8): []const u8 {
    let i: usize = 0;
    while (i < qname.len) {
        if (qname[i] == ':') {
            return qname[i + 1 .. qname.len];
        }
        i += 1;
    }
    return qname;
}

fn skipComment(xml: []const u8, pos: usize): usize {
    let i = pos + 4;
    while (i + 2 < xml.len) {
        if (xml[i] == '-' && xml[i + 1] == '-' && xml[i + 2] == '>') {
            return i + 3;
        }
        i += 1;
    }
    return xml.len;
}

fn skipCdata(xml: []const u8, pos: usize): usize {
    let i = pos + 9;
    while (i + 2 < xml.len) {
        if (xml[i] == ']' && xml[i + 1] == ']' && xml[i + 2] == '>') {
            return i + 3;
        }
        i += 1;
    }
    return xml.len;
}

fn skipPi(xml: []const u8, pos: usize): usize {
    let i = pos + 2;
    while (i + 1 < xml.len) {
        if (xml[i] == '?' && xml[i + 1] == '>') {
            return i + 2;
        }
        i += 1;
    }
    return xml.len;
}

fn skipDecl(xml: []const u8, pos: usize): usize {
    let i = pos + 2;
    let in_quote: u8 = 0;
    while (i < xml.len) {
        const c = xml[i];
        if (in_quote != 0) {
            if (c == in_quote) in_quote = 0;
        } else if (c == '"' || c == '\'') {
            in_quote = c;
        } else if (c == '>') {
            return i + 1;
        }
        i += 1;
    }
    return xml.len;
}

fn findTagEnd(xml: []const u8, lt_pos: usize): usize? {
    let i = lt_pos + 1;
    let in_quote: u8 = 0;
    while (i < xml.len) {
        const c = xml[i];
        if (in_quote != 0) {
            if (c == in_quote) in_quote = 0;
        } else if (c == '"' || c == '\'') {
            in_quote = c;
        } else if (c == '>') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn parseTagName(xml: []const u8, name_start: usize, tag_end: usize): []const u8 {
    let i = name_start;
    while (i < tag_end && !isTagDelim(xml[i])) {
        i += 1;
    }
    return xml[name_start..i];
}

fn isSelfClosingTag(xml: []const u8, lt_pos: usize, tag_end: usize): bool {
    if (tag_end <= lt_pos + 1) return false;
    let i = tag_end - 1;
    while (i > lt_pos && isXmlSpace(xml[i])) {
        i -= 1;
    }
    return xml[i] == '/';
}

fn skipProlog(xml: []const u8): usize {
    let pos: usize = 0;
    if (xml.len >= 3 && xml[0] == 0xEF && xml[1] == 0xBB && xml[2] == 0xBF) {
        pos = 3;
    }
    while (pos < xml.len) {
        while (pos < xml.len && isXmlSpace(xml[pos])) {
            pos += 1;
        }
        if (pos >= xml.len || xml[pos] != '<') {
            return pos;
        }
        if (startsWith(xml[pos..xml.len], "<!--")) {
            pos = skipComment(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<?")) {
            pos = skipPi(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<!")) {
            pos = skipDecl(xml, pos);
            continue;
        }
        return pos;
    }
    return pos;
}

fn extractAttrExact(tag: []const u8, target_name: []const u8): ([]const u8)? {
    if (tag.len < 3 || tag[0] != '<') return null;
    let i: usize = 1;
    while (i < tag.len && !isTagDelim(tag[i])) {
        i += 1;
    }
    while (i < tag.len) {
        while (i < tag.len && (isXmlSpace(tag[i]) || tag[i] == '/')) {
            i += 1;
        }
        if (i >= tag.len || tag[i] == '>') break;
        const name_start = i;
        while (i < tag.len && !isXmlSpace(tag[i]) && tag[i] != '=' && tag[i] != '>' && tag[i] != '/') {
            i += 1;
        }
        const attr_name = tag[name_start..i];
        while (i < tag.len && isXmlSpace(tag[i])) {
            i += 1;
        }
        if (i >= tag.len || tag[i] != '=') {
            continue;
        }
        i += 1;
        while (i < tag.len && isXmlSpace(tag[i])) {
            i += 1;
        }
        if (i >= tag.len) break;
        const quote = tag[i];
        if (quote != '"' && quote != '\'') {
            while (i < tag.len && !isXmlSpace(tag[i]) && tag[i] != '>' && tag[i] != '/') {
                i += 1;
            }
            continue;
        }
        i += 1;
        const val_start = i;
        while (i < tag.len && tag[i] != quote) {
            i += 1;
        }
        if (i >= tag.len) break;
        const val = tag[val_start..i];
        i += 1;
        if (strEql(attr_name, target_name)) {
            return val;
        }
    }
    return null;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isXmlSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isXmlSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn parseU32Dec(s: []const u8): u32? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + u64(c - '0');
        if (val > 4294967295) return null;
        i += 1;
    }
    return val as u32;
}

fn pow10Pos(exp: u32): f64 {
    let result: f64 = 1.0;
    let base: f64 = 10.0;
    let e = exp;
    while (e > 0) {
        if ((e & 1) != 0) {
            result = result * base;
        }
        base = base * base;
        e = e >> 1;
    }
    return result;
}

fn parseF64Token(tok: []const u8): f64? {
    if (tok.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (tok[i] == '-') {
        neg = true;
        i += 1;
    } else if (tok[i] == '+') {
        i += 1;
    }

    let val: f64 = 0.0;
    let digits: usize = 0;
    while (i < tok.len && tok[i] >= '0' && tok[i] <= '9') {
        val = val * 10.0 + ((tok[i] - '0') as f64);
        digits += 1;
        i += 1;
    }

    if (i < tok.len && tok[i] == '.') {
        i += 1;
        let frac: f64 = 0.0;
        let scale: f64 = 0.1;
        while (i < tok.len && tok[i] >= '0' && tok[i] <= '9') {
            frac = frac + ((tok[i] - '0') as f64) * scale;
            scale = scale * 0.1;
            digits += 1;
            i += 1;
        }
        val = val + frac;
    }

    if (digits == 0) return null;

    if (i < tok.len && (tok[i] == 'e' || tok[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < tok.len && tok[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < tok.len && tok[i] == '+') {
            i += 1;
        }
        let exp_digits: usize = 0;
        let exp_val: u32 = 0;
        while (i < tok.len && tok[i] >= '0' && tok[i] <= '9') {
            if (exp_val < 308) {
                exp_val = exp_val * 10 + u32(tok[i] - '0');
            }
            exp_digits += 1;
            i += 1;
        }
        if (exp_digits == 0) return null;
        const factor = pow10Pos(exp_val);
        if (exp_neg) {
            val = val / factor;
        } else {
            val = val * factor;
        }
    }

    if (i != tok.len) return null;
    if (neg) return -val;
    return val;
}

/// 跳过可选 BOM / XML 声明与注释，检测 `<COLLADA` 根标签
pub fn isCollada(data: []const u8): bool {
    const pos = skipProlog(data);
    if (pos >= data.len || data[pos] != '<') return false;
    if (pos + 1 >= data.len || data[pos + 1] == '/') return false;
    const tag_end = findTagEnd(data, pos) ?? return false;
    const name = parseTagName(data, pos + 1, tag_end);
    return strEql(localName(name), "COLLADA");
}

/// 解析 `<COLLADA version="...">` 与 `<asset>` 元数据（非 COLLADA 文档返回 `null`）
pub fn parseDaeAsset(xml: []const u8): DaeAsset? {
    const root_pos = skipProlog(xml);
    if (root_pos >= xml.len || xml[root_pos] != '<') return null;
    if (root_pos + 1 >= xml.len || xml[root_pos + 1] == '/') return null;

    const root_end = findTagEnd(xml, root_pos) ?? return null;
    const root_name = parseTagName(xml, root_pos + 1, root_end);
    if (!strEql(localName(root_name), "COLLADA")) return null;

    let asset = DaeAsset {
        version: extractAttrExact(xml[root_pos .. root_end + 1], "version") ?? "",
    };

    if (isSelfClosingTag(xml, root_pos, root_end)) {
        return asset;
    }

    // 定位 `<asset>...</asset>`
    let pos = root_end + 1;
    let in_asset = false;
    while (pos < xml.len) {
        if (xml[pos] != '<') {
            pos += 1;
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<!--")) {
            pos = skipComment(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<![CDATA[")) {
            pos = skipCdata(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<?")) {
            pos = skipPi(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<!")) {
            pos = skipDecl(xml, pos);
            continue;
        }

        const tag_end = findTagEnd(xml, pos) ?? break;
        if (pos + 1 < tag_end && xml[pos + 1] == '/') {
            const cname = localName(parseTagName(xml, pos + 2, tag_end));
            pos = tag_end + 1;
            if (in_asset && strEql(cname, "asset")) {
                break;
            }
            continue;
        }

        const lname = localName(parseTagName(xml, pos + 1, tag_end));
        if (!in_asset) {
            if (strEql(lname, "asset")) {
                if (isSelfClosingTag(xml, pos, tag_end)) {
                    break;
                }
                in_asset = true;
            }
            pos = tag_end + 1;
            continue;
        }

        // 在 `<asset>` 内部处理子元素
        if (strEql(lname, "unit")) {
            const tag = xml[pos .. tag_end + 1];
            if (un := extractAttrExact(tag, "name")) {
                asset.unit_name = un;
            }
            if (ms := extractAttrExact(tag, "meter")) {
                if (mv := parseF64Token(trimSpaces(ms))) {
                    asset.unit_meter = mv;
                }
            }
            pos = tag_end + 1;
            continue;
        }

        if (isSelfClosingTag(xml, pos, tag_end)) {
            pos = tag_end + 1;
            continue;
        }

        const is_leaf = strEql(lname, "author") ||
            strEql(lname, "authoring_tool") ||
            strEql(lname, "created") ||
            strEql(lname, "modified") ||
            strEql(lname, "up_axis");

        if (!is_leaf) {
            pos = tag_end + 1;
            continue;
        }

        const content_start = tag_end + 1;
        let scan = content_start;
        let close_start = content_start;
        let found_close = false;
        while (scan < xml.len) {
            if (xml[scan] != '<') {
                scan += 1;
                continue;
            }
            if (startsWith(xml[scan..xml.len], "<!--")) {
                scan = skipComment(xml, scan);
                continue;
            }
            const c_end = findTagEnd(xml, scan) ?? break;
            if (scan + 1 < c_end && xml[scan + 1] == '/') {
                const cname = localName(parseTagName(xml, scan + 2, c_end));
                if (strEql(cname, lname)) {
                    close_start = scan;
                    pos = c_end + 1;
                    found_close = true;
                    break;
                }
            }
            scan = c_end + 1;
        }
        if (!found_close) break;

        const val = trimSpaces(xml[content_start..close_start]);
        if (strEql(lname, "author")) {
            asset.author = val;
        } else if (strEql(lname, "authoring_tool")) {
            asset.authoring_tool = val;
        } else if (strEql(lname, "created")) {
            asset.created = val;
        } else if (strEql(lname, "modified")) {
            asset.modified = val;
        } else if (strEql(lname, "up_axis")) {
            asset.up_axis = val;
        }
    }

    return asset;
}

/// 零堆分配 `<library_geometries>` 几何体迭代器
pub struct DaeGeometryIter {
    xml: []const u8,
    pos: usize = 0,
    in_lib: bool = false,
    done: bool = false,

    pub static fn init(xml: []const u8): DaeGeometryIter {
        return DaeGeometryIter {
            xml: xml,
            pos: 0,
            in_lib: false,
            done: false,
        };
    }

    pub fn next(*_): DaeGeometry? {
        if (_.done) return null;

        while (_.pos < _.xml.len) {
            if (_.xml[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<!--")) {
                _.pos = skipComment(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<![CDATA[")) {
                _.pos = skipCdata(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<?")) {
                _.pos = skipPi(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<!")) {
                _.pos = skipDecl(_.xml, _.pos);
                continue;
            }

            const tag_end_opt = findTagEnd(_.xml, _.pos);
            if (tag_end_opt == null) {
                _.done = true;
                _.pos = _.xml.len;
                return null;
            }
            const tag_end = tag_end_opt ?? return null;

            if (_.pos + 1 < tag_end && _.xml[_.pos + 1] == '/') {
                const cname = localName(parseTagName(_.xml, _.pos + 2, tag_end));
                _.pos = tag_end + 1;
                if (_.in_lib && strEql(cname, "library_geometries")) {
                    _.in_lib = false;
                }
                continue;
            }

            const lname = localName(parseTagName(_.xml, _.pos + 1, tag_end));
            if (!_.in_lib) {
                if (strEql(lname, "library_geometries") && !isSelfClosingTag(_.xml, _.pos, tag_end)) {
                    _.in_lib = true;
                }
                _.pos = tag_end + 1;
                continue;
            }

            if (!strEql(lname, "geometry")) {
                _.pos = tag_end + 1;
                continue;
            }

            const geom_start = _.pos;
            const open_tag = _.xml[geom_start .. tag_end + 1];
            const gid = extractAttrExact(open_tag, "id") ?? "";
            const gname = extractAttrExact(open_tag, "name") ?? "";

            if (isSelfClosingTag(_.xml, geom_start, tag_end)) {
                _.pos = tag_end + 1;
                return DaeGeometry {
                    id: gid,
                    name: gname,
                    raw_xml: open_tag,
                };
            }

            let depth: usize = 1;
            let scan = tag_end + 1;
            while (scan < _.xml.len && depth > 0) {
                if (_.xml[scan] != '<') {
                    scan += 1;
                    continue;
                }
                if (startsWith(_.xml[scan.._.xml.len], "<!--")) {
                    scan = skipComment(_.xml, scan);
                    continue;
                }
                if (startsWith(_.xml[scan.._.xml.len], "<![CDATA[")) {
                    scan = skipCdata(_.xml, scan);
                    continue;
                }
                const sub_end_opt = findTagEnd(_.xml, scan);
                if (sub_end_opt == null) {
                    _.done = true;
                    _.pos = _.xml.len;
                    return null;
                }
                const sub_end = sub_end_opt ?? return null;
                if (scan + 1 < sub_end && _.xml[scan + 1] == '/') {
                    const cname = localName(parseTagName(_.xml, scan + 2, sub_end));
                    if (strEql(cname, "geometry")) {
                        depth -= 1;
                        if (depth == 0) {
                            const geom_end = sub_end + 1;
                            _.pos = geom_end;
                            return DaeGeometry {
                                id: gid,
                                name: gname,
                                raw_xml: _.xml[geom_start..geom_end],
                            };
                        }
                    }
                } else {
                    const oname = localName(parseTagName(_.xml, scan + 1, sub_end));
                    if (strEql(oname, "geometry") && !isSelfClosingTag(_.xml, scan, sub_end)) {
                        depth += 1;
                    }
                }
                scan = sub_end + 1;
            }

            _.done = true;
            _.pos = _.xml.len;
            return null;
        }

        _.done = true;
        return null;
    }
}

/// 零堆分配 `<float_array id="..." count="...">...</float_array>` 迭代器
pub struct DaeFloatArrayIter {
    xml: []const u8,
    pos: usize = 0,

    pub static fn init(xml: []const u8): DaeFloatArrayIter {
        return DaeFloatArrayIter {
            xml: xml,
            pos: 0,
        };
    }

    pub fn next(*_): DaeFloatArray? {
        while (_.pos < _.xml.len) {
            if (_.xml[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<!--")) {
                _.pos = skipComment(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<![CDATA[")) {
                _.pos = skipCdata(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<?")) {
                _.pos = skipPi(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<!")) {
                _.pos = skipDecl(_.xml, _.pos);
                continue;
            }

            const tag_end_opt = findTagEnd(_.xml, _.pos);
            if (tag_end_opt == null) {
                _.pos = _.xml.len;
                return null;
            }
            const tag_end = tag_end_opt ?? return null;

            if (_.pos + 1 < tag_end && _.xml[_.pos + 1] == '/') {
                _.pos = tag_end + 1;
                continue;
            }

            const lname = localName(parseTagName(_.xml, _.pos + 1, tag_end));
            if (!strEql(lname, "float_array")) {
                _.pos = tag_end + 1;
                continue;
            }

            const tag = _.xml[_.pos .. tag_end + 1];
            const fid = extractAttrExact(tag, "id") ?? "";
            let dcount: u32 = 0;
            if (cs := extractAttrExact(tag, "count")) {
                dcount = parseU32Dec(trimSpaces(cs)) ?? 0;
            }

            if (isSelfClosingTag(_.xml, _.pos, tag_end)) {
                _.pos = tag_end + 1;
                return DaeFloatArray {
                    id: fid,
                    declared_count: dcount,
                    raw_values: "",
                };
            }

            const content_start = tag_end + 1;
            let scan = content_start;
            while (scan < _.xml.len) {
                if (_.xml[scan] != '<') {
                    scan += 1;
                    continue;
                }
                if (startsWith(_.xml[scan.._.xml.len], "<!--")) {
                    scan = skipComment(_.xml, scan);
                    continue;
                }
                const c_end_opt = findTagEnd(_.xml, scan);
                if (c_end_opt == null) {
                    _.pos = _.xml.len;
                    return null;
                }
                const c_end = c_end_opt ?? return null;
                if (scan + 1 < c_end && _.xml[scan + 1] == '/') {
                    const cname = localName(parseTagName(_.xml, scan + 2, c_end));
                    if (strEql(cname, "float_array")) {
                        _.pos = c_end + 1;
                        return DaeFloatArray {
                            id: fid,
                            declared_count: dcount,
                            raw_values: _.xml[content_start..scan],
                        };
                    }
                }
                scan = c_end + 1;
            }

            _.pos = _.xml.len;
            return null;
        }
        return null;
    }
}

/// 零堆分配空白分隔 `f64` 浮点数流式解码器（支持科学计数法 `e`/`E`）
pub struct DaeFloatIter {
    raw: []const u8,
    pos: usize = 0,
    had_error: bool = false,

    pub static fn init(raw_values: []const u8): DaeFloatIter {
        return DaeFloatIter {
            raw: raw_values,
            pos: 0,
            had_error: false,
        };
    }

    pub fn next(*_): f64? {
        if (_.had_error) return null;
        while (_.pos < _.raw.len && isXmlSpace(_.raw[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.raw.len) return null;

        const start = _.pos;
        while (_.pos < _.raw.len && !isXmlSpace(_.raw[_.pos])) {
            _.pos += 1;
        }
        const tok = _.raw[start.._.pos];
        const v_opt = parseF64Token(tok);
        if (v_opt == null) {
            _.had_error = true;
            return null;
        }
        return v_opt;
    }
}

/// 在单个 `<node>` 的直接作用域内（跳过子 `<node>`）提取首个 `<instance_geometry url="..."/>`
fn findDirectInstanceGeometryUrl(xml: []const u8, start_after_open: usize): []const u8 {
    let scan = start_after_open;
    let node_depth: usize = 0;
    while (scan < xml.len) {
        if (xml[scan] != '<') {
            scan += 1;
            continue;
        }
        if (startsWith(xml[scan..xml.len], "<!--")) {
            scan = skipComment(xml, scan);
            continue;
        }
        if (startsWith(xml[scan..xml.len], "<![CDATA[")) {
            scan = skipCdata(xml, scan);
            continue;
        }
        const tag_end = findTagEnd(xml, scan) ?? break;
        if (scan + 1 < tag_end && xml[scan + 1] == '/') {
            const cname = localName(parseTagName(xml, scan + 2, tag_end));
            if (strEql(cname, "node")) {
                if (node_depth == 0) {
                    return "";
                }
                node_depth -= 1;
            }
            scan = tag_end + 1;
            continue;
        }

        const lname = localName(parseTagName(xml, scan + 1, tag_end));
        if (strEql(lname, "node")) {
            if (!isSelfClosingTag(xml, scan, tag_end)) {
                node_depth += 1;
            }
        } else if (node_depth == 0 && strEql(lname, "instance_geometry")) {
            if (u := extractAttrExact(xml[scan .. tag_end + 1], "url")) {
                return u;
            }
        }
        scan = tag_end + 1;
    }
    return "";
}

/// 零堆分配 `<library_visual_scenes>` 场景节点迭代器（前序遍历支持嵌套 `<node>`）
pub struct DaeSceneNodeIter {
    xml: []const u8,
    pos: usize = 0,
    in_lib: bool = false,

    pub static fn init(xml: []const u8): DaeSceneNodeIter {
        return DaeSceneNodeIter {
            xml: xml,
            pos: 0,
            in_lib: false,
        };
    }

    pub fn next(*_): DaeSceneNode? {
        while (_.pos < _.xml.len) {
            if (_.xml[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<!--")) {
                _.pos = skipComment(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<![CDATA[")) {
                _.pos = skipCdata(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<?")) {
                _.pos = skipPi(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<!")) {
                _.pos = skipDecl(_.xml, _.pos);
                continue;
            }

            const tag_end_opt = findTagEnd(_.xml, _.pos);
            if (tag_end_opt == null) {
                _.pos = _.xml.len;
                return null;
            }
            const tag_end = tag_end_opt ?? return null;

            if (_.pos + 1 < tag_end && _.xml[_.pos + 1] == '/') {
                const cname = localName(parseTagName(_.xml, _.pos + 2, tag_end));
                _.pos = tag_end + 1;
                if (_.in_lib && strEql(cname, "library_visual_scenes")) {
                    _.in_lib = false;
                }
                continue;
            }

            const lname = localName(parseTagName(_.xml, _.pos + 1, tag_end));
            if (!_.in_lib) {
                if (strEql(lname, "library_visual_scenes") && !isSelfClosingTag(_.xml, _.pos, tag_end)) {
                    _.in_lib = true;
                }
                _.pos = tag_end + 1;
                continue;
            }

            if (!strEql(lname, "node")) {
                _.pos = tag_end + 1;
                continue;
            }

            const open_tag = _.xml[_.pos .. tag_end + 1];
            const nid = extractAttrExact(open_tag, "id") ?? "";
            const nname = extractAttrExact(open_tag, "name") ?? "";
            const ntype = extractAttrExact(open_tag, "type") ?? "";
            const self_closed = isSelfClosingTag(_.xml, _.pos, tag_end);
            _.pos = tag_end + 1;

            const ig_url = if (self_closed) "" else findDirectInstanceGeometryUrl(_.xml, _.pos);

            return DaeSceneNode {
                id: nid,
                name: nname,
                node_type: ntype,
                instance_geometry_url: ig_url,
            };
        }
        return null;
    }
}

/// 统计 COLLADA 资产的几何体数、浮点数组数、总浮点元素数、场景节点数与材质数
pub fn summarizeDae(xml: []const u8): DaeStats {
    let stats = DaeStats {};
    if (!isCollada(xml)) return stats;

    let git = DaeGeometryIter.init(xml);
    while (g := git.next()) {
        * = g;
        stats.geometry_count += 1;
    }

    let fait = DaeFloatArrayIter.init(xml);
    while (fa := fait.next()) {
        stats.float_array_count += 1;
        stats.total_floats += usize(fa.declared_count);
    }

    let nit = DaeSceneNodeIter.init(xml);
    while (n := nit.next()) {
        * = n;
        stats.node_count += 1;
    }

    // 统计 `<library_materials>` 中的 `<material ...>`
    let pos: usize = 0;
    let in_mats = false;
    while (pos < xml.len) {
        if (xml[pos] != '<') {
            pos += 1;
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<!--")) {
            pos = skipComment(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<![CDATA[")) {
            pos = skipCdata(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<?")) {
            pos = skipPi(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<!")) {
            pos = skipDecl(xml, pos);
            continue;
        }
        const tag_end = findTagEnd(xml, pos) ?? break;
        if (pos + 1 < tag_end && xml[pos + 1] == '/') {
            const cname = localName(parseTagName(xml, pos + 2, tag_end));
            if (in_mats && strEql(cname, "library_materials")) {
                in_mats = false;
            }
            pos = tag_end + 1;
            continue;
        }
        const lname = localName(parseTagName(xml, pos + 1, tag_end));
        if (!in_mats) {
            if (strEql(lname, "library_materials") && !isSelfClosingTag(xml, pos, tag_end)) {
                in_mats = true;
            }
        } else if (strEql(lname, "material")) {
            stats.material_count += 1;
        }
        pos = tag_end + 1;
    }

    return stats;
}
