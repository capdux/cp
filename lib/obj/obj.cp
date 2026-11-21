// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Wavefront 3D OBJ Mesh Parser)

/// Wavefront 3D `.obj` 语句类型枚举
pub enum ObjRecordKind: u8 {
    vertex = 0,
    texcoord = 1,
    normal = 2,
    face = 3,
    object_name = 4,
    group_name = 5,
    usemtl = 6,
    mtllib = 7,
    other = 8,
}

/// 3D 双精度浮点向量（顶点坐标或法线）
pub struct ObjVec3 {
    x: f64,
    y: f64,
    z: f64,
}

/// OBJ 面片顶点引用三元组（支持 `v`、`v/vt`、`v//vn`、`v/vt/vn` 及正/负相对索引）
pub struct ObjFaceVertex {
    v: i32,
    vt: i32?,
    vn: i32?,
}

/// OBJ 网格文件统计摘要
pub struct ObjStats {
    vertices: usize,
    texcoords: usize,
    normals: usize,
    faces: usize,
    objects: usize,
    groups: usize,
}

/// 零拷贝 OBJ 单行记录（指令分类 + 已剥离注释与首尾空白的参数切片）
pub struct ObjRecord {
    kind: ObjRecordKind,
    rest: []const u8,
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
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

fn trimHoriz(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isHorizSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isHorizSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn classifyKeyword(kw: []const u8): ObjRecordKind {
    if (strEq(kw, "v")) return ObjRecordKind.vertex;
    if (strEq(kw, "vt")) return ObjRecordKind.texcoord;
    if (strEq(kw, "vn")) return ObjRecordKind.normal;
    if (strEq(kw, "f")) return ObjRecordKind.face;
    if (strEq(kw, "o")) return ObjRecordKind.object_name;
    if (strEq(kw, "g")) return ObjRecordKind.group_name;
    if (strEq(kw, "usemtl")) return ObjRecordKind.usemtl;
    if (strEq(kw, "mtllib")) return ObjRecordKind.mtllib;
    return ObjRecordKind.other;
}

fn parseNonZeroI32(s: []const u8): i32? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) return null;

    let acc: i64 = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isDigit(c)) return null;
        acc = acc * 10 + ((c - '0') as i64);
        if (neg) {
            if (acc > 2147483648) return null;
        } else {
            if (acc > 2147483647) return null;
        }
        i += 1;
    }

    if (acc == 0) return null;
    if (neg) {
        return (-acc) as i32;
    }
    return acc as i32;
}

fn parseF64(s: []const u8): f64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) return null;

    let val: f64 = 0.0;
    let saw_digit = false;

    while (i < s.len && isDigit(s[i])) {
        saw_digit = true;
        val = val * 10.0 + ((s[i] - '0') as f64);
        i += 1;
    }

    if (i < s.len && s[i] == '.') {
        i += 1;
        let frac: f64 = 0.1;
        while (i < s.len && isDigit(s[i])) {
            saw_digit = true;
            val += ((s[i] - '0') as f64) * frac;
            frac *= 0.1;
            i += 1;
        }
    }

    if (!saw_digit) return null;

    // Optional scientific notation exponent: [eE][+-]?[0-9]+
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        if (i >= s.len || !isDigit(s[i])) return null;

        let exp_val: i32 = 0;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 400) {
                exp_val = exp_val * 10 + ((s[i] - '0') as i32);
            }
            i += 1;
        }

        let e: i32 = 0;
        while (e < exp_val) {
            if (exp_neg) {
                val *= 0.1;
            } else {
                val *= 10.0;
            }
            e += 1;
        }
    }

    if (i != s.len) return null;
    if (neg) return -val;
    return val;
}

/// 零堆分配 Wavefront `.obj` 行记录流式迭代器（支持 `\n`/`\r\n`，自动跳过空行与 `#` 注释）
pub struct ObjLineIter {
    input: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): ObjLineIter {
        return ObjLineIter {
            input: input,
            pos: 0,
        };
    }

    /// 读取下一条非空 OBJ 指令记录；到达末尾时返回 `null`
    pub fn next(*_): ObjRecord? {
        while (_.pos < _.input.len) {
            const line_start = _.pos;
            while (_.pos < _.input.len && _.input[_.pos] != '\n') {
                _.pos += 1;
            }
            let line_end = _.pos;
            if (_.pos < _.input.len && _.input[_.pos] == '\n') {
                _.pos += 1;
            }

            // Strip trailing '#' comment
            let comment_idx = line_start;
            while (comment_idx < line_end) {
                if (_.input[comment_idx] == '#') {
                    line_end = comment_idx;
                    break;
                }
                comment_idx += 1;
            }

            const trimmed = trimHoriz(_.input[line_start..line_end]);
            if (trimmed.len == 0) continue;

            // Extract leading keyword
            let kw_end: usize = 0;
            while (kw_end < trimmed.len && !isHorizSpace(trimmed[kw_end])) {
                kw_end += 1;
            }
            const kw = trimmed[0..kw_end];
            const rest = trimHoriz(trimmed[kw_end..trimmed.len]);

            return ObjRecord {
                kind: classifyKeyword(kw),
                rest: rest,
            };
        }
        return null;
    }
}

/// 解析 3 个浮点数 `x y z`（可选第 4 齐次分量 `w` 存在则忽略）
pub fn parseObjVec3(args: []const u8): ObjVec3? {
    let i: usize = 0;
    let comps: [3]f64 = undefined;
    let count: usize = 0;

    while (count < 3) {
        while (i < args.len && isHorizSpace(args[i])) {
            i += 1;
        }
        if (i >= args.len) return null;
        const start = i;
        while (i < args.len && !isHorizSpace(args[i])) {
            i += 1;
        }
        const v = parseF64(args[start..i]) ?? return null;
        comps[count] = v;
        count += 1;
    }

    return ObjVec3 {
        x: comps[0],
        y: comps[1],
        z: comps[2],
    };
}

/// 解析 OBJ 面片顶点记号（`v`、`v/vt`、`v//vn`、`v/vt/vn`），拦截 `0` 索引与非法格式
pub fn parseObjFaceVertex(token: []const u8): ObjFaceVertex? {
    if (token.len == 0) return null;

    let slash1: usize? = null;
    let slash2: usize? = null;
    let i: usize = 0;
    while (i < token.len) {
        if (token[i] == '/') {
            if (slash1 == null) {
                slash1 = i;
            } else if (slash2 == null) {
                slash2 = i;
            } else {
                // More than 2 slashes is invalid
                return null;
            }
        }
        i += 1;
    }

    if (s1 := slash1) {
        const v_idx = parseNonZeroI32(token[0..s1]) ?? return null;
        if (s2 := slash2) {
            // `v//vn` or `v/vt/vn`
            const mid = token[s1 + 1 .. s2];
            const tail = token[s2 + 1 .. token.len];
            if (tail.len == 0) return null;
            const vn_idx = parseNonZeroI32(tail) ?? return null;
            let vt_idx: i32? = null;
            if (mid.len > 0) {
                vt_idx = parseNonZeroI32(mid) ?? return null;
            }
            return ObjFaceVertex {
                v: v_idx,
                vt: vt_idx,
                vn: vn_idx,
            };
        } else {
            // `v/vt`
            const tail = token[s1 + 1 .. token.len];
            if (tail.len == 0) return null;
            const vt_idx = parseNonZeroI32(tail) ?? return null;
            return ObjFaceVertex {
                v: v_idx,
                vt: vt_idx,
                vn: null,
            };
        }
    }

    // Plain `v`
    const v_idx = parseNonZeroI32(token) ?? return null;
    return ObjFaceVertex {
        v: v_idx,
        vt: null,
        vn: null,
    };
}

/// 一趟零分配统计 Wavefront `.obj` 网格的顶点、纹理坐标、法线、面片、对象与分组数量
pub fn summarizeObj(input: []const u8): ObjStats {
    let stats = ObjStats {
        vertices: 0,
        texcoords: 0,
        normals: 0,
        faces: 0,
        objects: 0,
        groups: 0,
    };
    let it = ObjLineIter.init(input);
    while (rec := it.next()) {
        if (rec.kind == ObjRecordKind.vertex) {
            stats.vertices += 1;
        } else if (rec.kind == ObjRecordKind.texcoord) {
            stats.texcoords += 1;
        } else if (rec.kind == ObjRecordKind.normal) {
            stats.normals += 1;
        } else if (rec.kind == ObjRecordKind.face) {
            stats.faces += 1;
        } else if (rec.kind == ObjRecordKind.object_name) {
            stats.objects += 1;
        } else if (rec.kind == ObjRecordKind.group_name) {
            stats.groups += 1;
        }
    }
    return stats;
}
