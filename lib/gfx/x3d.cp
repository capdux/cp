// Origin: Pure-CP Stdlib (Zero-Alloc Web3D X3D 3.x/4.0 XML Scene Parser) | Created: 2026-11-22

/// X3D 根文档头信息（零拷贝切片）
pub struct X3dHeader {
    profile: []const u8,
    version: []const u8,
    scene_offset: usize,
}

/// X3D <head>/<meta> 元数据条目（零拷贝切片）
pub struct X3dMeta {
    name: []const u8,
    content: []const u8,
}

/// X3D <Scene> 内的单个 3D 节点（零拷贝切片）
pub struct X3dNode {
    tag: []const u8,
    def_name: []const u8,
    use_name: []const u8,
    attrs_slice: []const u8,
    depth: u16,

    /// 从节点的原始属性切片中零分配提取指定属性值
    pub fn getAttr(name: []const u8): ([]const u8)? {
        return getAttrFromSlice(_.attrs_slice, name);
    }
}

/// X3D 场景单遍统计汇总
pub struct X3dStats {
    node_count: u32,
    def_count: u32,
    use_count: u32,
    shape_count: u32,
    transform_count: u32,
    route_count: u32,
    max_depth: u32,
}

// ---------------------------------------------------------------------------
// 内部零分配基础与 XML 扫描辅助函数
// ---------------------------------------------------------------------------

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isNumSep(c: u8): bool {
    return isSpace(c) || c == ',';
}

fn isTagBoundary(c: u8): bool {
    return isSpace(c) || c == '>' || c == '/';
}

fn isNameStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
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

fn startsWithAt(haystack: []const u8, pos: usize, needle: []const u8): bool {
    if (pos + needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) {
        if (haystack[pos + i] != needle[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findSubstr(haystack: []const u8, start: usize, needle: []const u8): usize? {
    if (needle.len == 0) {
        return if (start <= haystack.len) start else null;
    }
    if (start + needle.len > haystack.len) {
        return null;
    }
    let i = start;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        if (startsWithAt(haystack, i, needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn localName(name: []const u8): []const u8 {
    let i: usize = 0;
    while (i < name.len) {
        if (name[i] == ':') {
            return name[i + 1 ..];
        }
        i += 1;
    }
    return name;
}

fn skipUtf8Bom(data: []const u8): usize {
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return 3;
    }
    return 0;
}

fn findTagClose(xml: []const u8, open_pos: usize): usize? {
    let i = open_pos;
    let quote: u8 = 0;
    while (i < xml.len) {
        const c = xml[i];
        if (quote != 0) {
            if (c == quote) {
                quote = 0;
            }
        } else {
            if (c == '"' || c == '\'') {
                quote = c;
            } else if (c == '>') {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn skipNonElementMarkup(xml: []const u8, pos: usize): usize {
    if (startsWithAt(xml, pos, "<!--")) {
        if (end_idx := findSubstr(xml, pos + 4, "-->")) {
            return end_idx + 3;
        }
        return xml.len;
    }
    if (startsWithAt(xml, pos, "<![CDATA[")) {
        if (end_idx := findSubstr(xml, pos + 9, "]]>")) {
            return end_idx + 3;
        }
        return xml.len;
    }
    if (startsWithAt(xml, pos, "<?")) {
        if (end_idx := findSubstr(xml, pos + 2, "?>")) {
            return end_idx + 2;
        }
        return xml.len;
    }
    if (startsWithAt(xml, pos, "<!")) {
        if (end_idx := findTagClose(xml, pos + 2)) {
            return end_idx + 1;
        }
        return xml.len;
    }
    return pos + 1;
}

struct OpenTagInfo {
    full_name: []const u8,
    attrs: []const u8,
    tag_end: usize,
    self_closing: bool,
}

struct CloseTagInfo {
    full_name: []const u8,
    tag_end: usize,
}

struct CloseMatch {
    inner_end: usize,
    after_close: usize,
}

struct ElementSpan {
    attrs: []const u8,
    inner_start: usize,
    inner_end: usize,
    next_pos: usize,
}

fn parseOpenTagAt(xml: []const u8, pos: usize): OpenTagInfo? {
    if (pos + 2 >= xml.len || xml[pos] != '<') {
        return null;
    }
    const name_start = pos + 1;
    if (!isNameStart(xml[name_start])) {
        return null;
    }
    let name_end = name_start;
    while (name_end < xml.len && !isTagBoundary(xml[name_end])) {
        name_end += 1;
    }
    if (name_end == name_start) {
        return null;
    }
    const close_gt = findTagClose(xml, name_end) ?? return null;
    let attr_end = close_gt;
    let self_closing = false;
    if (close_gt > name_end && xml[close_gt - 1] == '/') {
        self_closing = true;
        attr_end = close_gt - 1;
    }
    return OpenTagInfo {
        full_name: xml[name_start..name_end],
        attrs: xml[name_end..attr_end],
        tag_end: close_gt + 1,
        self_closing: self_closing,
    };
}

fn parseCloseTagAt(xml: []const u8, pos: usize): CloseTagInfo? {
    if (pos + 3 >= xml.len || xml[pos] != '<' || xml[pos + 1] != '/') {
        return null;
    }
    let name_start = pos + 2;
    while (name_start < xml.len && isSpace(xml[name_start])) {
        name_start += 1;
    }
    if (name_start >= xml.len || !isNameStart(xml[name_start])) {
        return null;
    }
    let name_end = name_start;
    while (name_end < xml.len && !isTagBoundary(xml[name_end])) {
        name_end += 1;
    }
    let i = name_end;
    while (i < xml.len && isSpace(xml[i])) {
        i += 1;
    }
    if (i >= xml.len || xml[i] != '>') {
        return null;
    }
    return CloseTagInfo {
        full_name: xml[name_start..name_end],
        tag_end: i + 1,
    };
}

fn getAttrFromSlice(attrs: []const u8, target_name: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < attrs.len) {
        while (i < attrs.len && isSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) {
            break;
        }
        const name_start = i;
        while (i < attrs.len && !isSpace(attrs[i]) && attrs[i] != '=' && attrs[i] != '/' && attrs[i] != '>') {
            i += 1;
        }
        const attr_name = attrs[name_start..i];
        while (i < attrs.len && isSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len || attrs[i] != '=') {
            continue;
        }
        i += 1;
        while (i < attrs.len && isSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const quote = attrs[i];
        if (quote != '"' && quote != '\'') {
            while (i < attrs.len && !isSpace(attrs[i])) {
                i += 1;
            }
            continue;
        }
        i += 1;
        const val_start = i;
        while (i < attrs.len && attrs[i] != quote) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const val = attrs[val_start..i];
        i += 1;
        if (eqlBytes(attr_name, target_name)) {
            return val;
        }
    }
    return null;
}

fn findMatchingCloseTag(xml: []const u8, start: usize, end_limit: usize, tag_local: []const u8): CloseMatch? {
    let i = start;
    let depth: usize = 0;
    while (i < end_limit) {
        if (xml[i] != '<') {
            i += 1;
            continue;
        }
        if (i + 1 < end_limit && (xml[i + 1] == '!' || xml[i + 1] == '?')) {
            i = skipNonElementMarkup(xml[0..end_limit], i);
            continue;
        }
        if (i + 1 < end_limit && xml[i + 1] == '/') {
            if (ct := parseCloseTagAt(xml[0..end_limit], i)) {
                if (eqlBytes(localName(ct.full_name), tag_local)) {
                    if (depth == 0) {
                        return CloseMatch {
                            inner_end: i,
                            after_close: ct.tag_end,
                        };
                    }
                    depth -= 1;
                }
                i = ct.tag_end;
                continue;
            }
            i += 1;
            continue;
        }
        if (ot := parseOpenTagAt(xml[0..end_limit], i)) {
            if (!ot.self_closing && eqlBytes(localName(ot.full_name), tag_local)) {
                depth += 1;
            }
            i = ot.tag_end;
            continue;
        }
        i += 1;
    }
    return null;
}

fn findElementSpanRange(xml: []const u8, start: usize, end_limit: usize, target_local: []const u8): ElementSpan? {
    let i = start;
    while (i < end_limit) {
        if (xml[i] != '<') {
            i += 1;
            continue;
        }
        if (i + 1 < end_limit && (xml[i + 1] == '!' || xml[i + 1] == '?')) {
            i = skipNonElementMarkup(xml[0..end_limit], i);
            continue;
        }
        if (i + 1 < end_limit && xml[i + 1] == '/') {
            if (ct := parseCloseTagAt(xml[0..end_limit], i)) {
                i = ct.tag_end;
            } else {
                i += 1;
            }
            continue;
        }
        if (ot := parseOpenTagAt(xml[0..end_limit], i)) {
            if (eqlBytes(localName(ot.full_name), target_local)) {
                if (ot.self_closing) {
                    return ElementSpan {
                        attrs: ot.attrs,
                        inner_start: ot.tag_end,
                        inner_end: ot.tag_end,
                        next_pos: ot.tag_end,
                    };
                }
                const cm = findMatchingCloseTag(xml, ot.tag_end, end_limit, target_local) ?? return null;
                return ElementSpan {
                    attrs: ot.attrs,
                    inner_start: ot.tag_end,
                    inner_end: cm.inner_end,
                    next_pos: cm.after_close,
                };
            }
            i = ot.tag_end;
            continue;
        }
        i += 1;
    }
    return null;
}

fn parseF64(s: []const u8): f64? {
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }
    let int_part: f64 = 0.0;
    let digits: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        int_part = int_part * 10.0 + f64(s[i] - '0');
        digits += 1;
        i += 1;
    }
    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 1.0;
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && isDigit(s[i])) {
            frac_scale *= 0.1;
            frac_part += f64(s[i] - '0') * frac_scale;
            digits += 1;
            i += 1;
        }
    }
    if (digits == 0) {
        return null;
    }
    let val = int_part + frac_part;
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        let exp_val: u32 = 0;
        let exp_digits: usize = 0;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + u32(s[i] - '0');
            }
            exp_digits += 1;
            i += 1;
        }
        if (exp_digits == 0 || exp_val > 308) {
            return null;
        }
        let e: u32 = 0;
        while (e < exp_val) {
            if (exp_neg) {
                val *= 0.1;
            } else {
                val *= 10.0;
            }
            e += 1;
        }
    }
    if (i != s.len) {
        return null;
    }
    return if (neg) -val else val;
}

fn parseI32(s: []const u8): i32? {
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }
    if (i >= s.len) {
        return null;
    }
    let acc: i64 = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return null;
        }
        acc = acc * 10 + i64(s[i] - '0');
        if (acc > 2147483648) {
            return null;
        }
        i += 1;
    }
    const signed = if (neg) -acc else acc;
    if (signed < -2147483648 || signed > 2147483647) {
        return null;
    }
    return signed as i32;
}

// ---------------------------------------------------------------------------
// 1. 格式识别与 <X3D> 根头解析
// ---------------------------------------------------------------------------

/// 跳过可选 UTF-8 BOM、XML 声明、DOCTYPE 与注释，检测首个根元素是否为 `<X3D`
pub fn isX3d(data: []const u8): bool {
    let i = skipUtf8Bom(data);
    while (i < data.len) {
        while (i < data.len && isSpace(data[i])) {
            i += 1;
        }
        if (i >= data.len || data[i] != '<') {
            return false;
        }
        if (i + 1 < data.len && (data[i + 1] == '!' || data[i + 1] == '?')) {
            i = skipNonElementMarkup(data, i);
            continue;
        }
        if (ot := parseOpenTagAt(data, i)) {
            return eqlBytes(localName(ot.full_name), "X3D");
        }
        return false;
    }
    return false;
}

/// 零分配解析 `<X3D profile="..." version="...">` 及 `<Scene>` 起始偏移
pub fn parseX3dHeader(xml: []const u8): X3dHeader? {
    if (!isX3d(xml)) {
        return null;
    }
    const start = skipUtf8Bom(xml);
    const root = findElementSpanRange(xml, start, xml.len, "X3D") ?? return null;
    const scene = findElementSpanRange(xml, root.inner_start, root.inner_end, "Scene") ?? return null;

    return X3dHeader {
        profile: getAttrFromSlice(root.attrs, "profile") ?? "",
        version: getAttrFromSlice(root.attrs, "version") ?? "",
        scene_offset: scene.inner_start,
    };
}

// ---------------------------------------------------------------------------
// 2. <head>/<meta> 元数据迭代器与查询
// ---------------------------------------------------------------------------

/// 零分配迭代 `<head>` 中全部 `<meta name="..." content="..."/>` 标签
pub struct X3dMetaIter {
    section: []const u8,
    pos: usize,

    pub fn init(xml: []const u8): X3dMetaIter {
        if (isX3d(xml)) {
            const start = skipUtf8Bom(xml);
            if (root := findElementSpanRange(xml, start, xml.len, "X3D")) {
                if (head := findElementSpanRange(xml, root.inner_start, root.inner_end, "head")) {
                    return X3dMetaIter {
                        section: xml[head.inner_start..head.inner_end],
                        pos: 0,
                    };
                }
            }
        }
        return X3dMetaIter {
            section: "",
            pos: 0,
        };
    }

    pub fn next(*_): X3dMeta? {
        const s = _.section;
        while (_.pos < s.len) {
            if (s[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (_.pos + 1 < s.len && (s[_.pos + 1] == '!' || s[_.pos + 1] == '?')) {
                _.pos = skipNonElementMarkup(s, _.pos);
                continue;
            }
            if (_.pos + 1 < s.len && s[_.pos + 1] == '/') {
                if (ct := parseCloseTagAt(s, _.pos)) {
                    _.pos = ct.tag_end;
                } else {
                    _.pos += 1;
                }
                continue;
            }
            if (ot := parseOpenTagAt(s, _.pos)) {
                _.pos = ot.tag_end;
                if (eqlBytes(localName(ot.full_name), "meta")) {
                    const name = getAttrFromSlice(ot.attrs, "name") ?? "";
                    if (name.len > 0) {
                        const content = getAttrFromSlice(ot.attrs, "content") ?? "";
                        return X3dMeta {
                            name: name,
                            content: content,
                        };
                    }
                }
                continue;
            }
            _.pos += 1;
        }
        return null;
    }
}

/// 按名称零分配查找 `<head>` 中的 `<meta>` 内容
pub fn findX3dMeta(xml: []const u8, name: []const u8): ([]const u8)? {
    if (name.len == 0) {
        return null;
    }
    let it = X3dMetaIter.init(xml);
    while (m := it.next()) {
        if (eqlBytes(m.name, name)) {
            return m.content;
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// 3. <Scene> 节点树前序迭代器
// ---------------------------------------------------------------------------

/// 零分配前序遍历 `<Scene>` 内全部 3D 节点标签
pub struct X3dNodeIter {
    section: []const u8,
    pos: usize,
    cur_depth: u16,

    pub fn init(xml: []const u8): X3dNodeIter {
        if (isX3d(xml)) {
            const start = skipUtf8Bom(xml);
            if (root := findElementSpanRange(xml, start, xml.len, "X3D")) {
                if (scene := findElementSpanRange(xml, root.inner_start, root.inner_end, "Scene")) {
                    return X3dNodeIter {
                        section: xml[scene.inner_start..scene.inner_end],
                        pos: 0,
                        cur_depth: 0,
                    };
                }
            }
        }
        return X3dNodeIter {
            section: "",
            pos: 0,
            cur_depth: 0,
        };
    }

    pub fn next(*_): X3dNode? {
        const s = _.section;
        while (_.pos < s.len) {
            if (s[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (_.pos + 1 < s.len && (s[_.pos + 1] == '!' || s[_.pos + 1] == '?')) {
                _.pos = skipNonElementMarkup(s, _.pos);
                continue;
            }
            if (_.pos + 1 < s.len && s[_.pos + 1] == '/') {
                if (ct := parseCloseTagAt(s, _.pos)) {
                    _.pos = ct.tag_end;
                    if (_.cur_depth > 0) {
                        _.cur_depth -= 1;
                    }
                } else {
                    _.pos += 1;
                }
                continue;
            }
            if (ot := parseOpenTagAt(s, _.pos)) {
                _.pos = ot.tag_end;
                const depth = _.cur_depth;
                if (!ot.self_closing) {
                    _.cur_depth += 1;
                }
                return X3dNode {
                    tag: localName(ot.full_name),
                    def_name: getAttrFromSlice(ot.attrs, "DEF") ?? "",
                    use_name: getAttrFromSlice(ot.attrs, "USE") ?? "",
                    attrs_slice: ot.attrs,
                    depth: depth,
                };
            }
            _.pos += 1;
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// 4. 数值数组属性零分配解码 (MFVec3f / MFFloat / MFInt32)
// ---------------------------------------------------------------------------

/// 零分配解码空白或逗号分隔的 `MFVec3f` / `MFFloat` 浮点属性（支持科学计数法）
pub struct X3dFloatIter {
    src: []const u8,
    pos: usize,
    malformed: bool,

    pub fn init(attr_val: []const u8): X3dFloatIter {
        return X3dFloatIter {
            src: attr_val,
            pos: 0,
            malformed: false,
        };
    }

    pub fn next(*_): f64? {
        const s = _.src;
        while (_.pos < s.len && isNumSep(s[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= s.len) {
            return null;
        }
        const start = _.pos;
        while (_.pos < s.len && !isNumSep(s[_.pos])) {
            _.pos += 1;
        }
        if (v := parseF64(s[start.._.pos])) {
            return v;
        }
        _.malformed = true;
        _.pos = s.len;
        return null;
    }
}

/// 零分配解码空白或逗号分隔的 `MFInt32` 整数属性（支持 `coordIndex` 中的 `-1` 面分隔符）
pub struct X3dIntIter {
    src: []const u8,
    pos: usize,
    malformed: bool,

    pub fn init(attr_val: []const u8): X3dIntIter {
        return X3dIntIter {
            src: attr_val,
            pos: 0,
            malformed: false,
        };
    }

    pub fn next(*_): i32? {
        const s = _.src;
        while (_.pos < s.len && isNumSep(s[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= s.len) {
            return null;
        }
        const start = _.pos;
        while (_.pos < s.len && !isNumSep(s[_.pos])) {
            _.pos += 1;
        }
        if (v := parseI32(s[start.._.pos])) {
            return v;
        }
        _.malformed = true;
        _.pos = s.len;
        return null;
    }
}

// ---------------------------------------------------------------------------
// 5. 场景统计汇总 (summarizeX3d)
// ---------------------------------------------------------------------------

/// 单遍统计 `<Scene>` 内的节点、DEF/USE、Shape/Transform/ROUTE 数量与最大嵌套深度
pub fn summarizeX3d(xml: []const u8): X3dStats? {
    * = parseX3dHeader(xml) ?? return null;

    let stats = X3dStats {
        node_count: 0,
        def_count: 0,
        use_count: 0,
        shape_count: 0,
        transform_count: 0,
        route_count: 0,
        max_depth: 0,
    };

    let it = X3dNodeIter.init(xml);
    while (n := it.next()) {
        stats.node_count += 1;
        if (n.def_name.len > 0) stats.def_count += 1;
        if (n.use_name.len > 0) stats.use_count += 1;
        if (eqlBytes(n.tag, "Shape")) stats.shape_count += 1;
        if (eqlBytes(n.tag, "Transform")) stats.transform_count += 1;
        if (eqlBytes(n.tag, "ROUTE")) stats.route_count += 1;
        const d = u32(n.depth) + 1;
        if (d > stats.max_depth) {
            stats.max_depth = d;
        }
    }

    return stats;
}
