// Source: Absorbed from /data/dev/bun/src/cli/pm_pkg_command.cp | Synced: 2026-11-22T08:01:00Z
// Pure-CP zero-allocation `pm pkg` (`get` / `set` / `delete` / `fix`) subcommand parser,
// dot & bracket property path parser (`scripts.build`, `contributors[0]`, `workspaces.0`,
// `scripts[test:watch]`, `scripts["build.prod"]`), `key=value` assignment parser, and `--json` value classifier.

/// 单条 `pm pkg` 路径支持的最大段数
pub const PM_PKG_MAX_SEGMENTS: usize = 32;

/// `pm pkg` 子命令、路径与赋值解析错误集
pub err PmPkgError {
    InvalidSubcommand,
    EmptyKey,
    InvalidPath,
    UnclosedBracket,
    TooManySegments,
    InvalidAssignment,
    InvalidJsonValue,
    BufferTooSmall,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isSimpleIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-' || c == '$';
}

fn isSimpleDotKey(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isSimpleIdentChar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
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

fn parseStrictUsize(s: []const u8): usize? {
    if (s.len == 0) {
        return null;
    }
    if (s.len > 1 && s[0] == '0') {
        return null;
    }
    let acc: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isDigit(c)) {
            return null;
        }
        if (acc > 100_000_000) {
            return null;
        }
        acc = acc * 10 + ((c - '0') as usize);
        i += 1;
    }
    return acc;
}

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (pos + src.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn writeUsize(out: []u8, start: usize, val: usize): usize! {
    if (val == 0) {
        return writeByte(out, start, '0');
    }
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = ('0' as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    let pos = start;
    while (n > 0) {
        n -= 1;
        pos = writeByte(out, pos, tmp[n])?;
    }
    return pos;
}

/// `pm pkg` 子命令枚举（对应上游 `PmPkgSubCommand`）
pub enum PmPkgSubCommand: u8 {
    get = 0,
    set = 1,
    delete = 2,
    fix = 3,

    /// 从字符串解析子命令
    static fn fromString(str: []const u8): PmPkgSubCommand? {
        if (bytesEql(str, "get")) return .get;
        if (bytesEql(str, "set")) return .set;
        if (bytesEql(str, "delete")) return .delete;
        if (bytesEql(str, "fix")) return .fix;
        return null;
    }

    /// 返回规范关键字
    fn asKeyword(): []const u8 {
        if (_ == .get) return "get";
        if (_ == .set) return "set";
        if (_ == .delete) return "delete";
        return "fix";
    }
}

/// 路径段类别：对象属性名或数组下标
pub enum PmPkgSegmentKind: u8 {
    property = 0,
    array_index = 1,
}

/// `package.json` 键路径中的单个段
pub struct PmPkgPathSegment {
    kind: PmPkgSegmentKind,
    key: []const u8,
    index: usize,
    from_bracket: bool,

    /// 空段
    static fn empty(): PmPkgPathSegment {
        return PmPkgPathSegment{
            kind: .property,
            key: "",
            index: 0,
            from_bracket: false,
        };
    }

    /// 语义等价比较
    fn eql(other: PmPkgPathSegment): bool {
        if (_.kind != other.kind) {
            return false;
        }
        if (_.kind == .array_index) {
            return _.index == other.index;
        }
        return bytesEql(_.key, other.key);
    }
}

/// 解析后的 `pm pkg` 点号与方括号属性路径（对应上游 `parseKeyPath`）
pub struct PmPkgKeyPath {
    segments: [PM_PKG_MAX_SEGMENTS]PmPkgPathSegment,
    len: usize,

    /// 空路径
    static fn empty(): PmPkgKeyPath {
        return PmPkgKeyPath{
            segments: [PmPkgPathSegment.empty(); PM_PKG_MAX_SEGMENTS],
            len: 0,
        };
    }

    /// 解析点号与方括号路径（支持 `scripts.build`、`contributors[0]`、`workspaces.0`、`scripts[test:watch]`、`scripts["build.prod"]`）
    static fn parse(raw_key: []const u8): PmPkgKeyPath! {
        const key = trimWs(raw_key);
        if (key.len == 0) {
            return err.EmptyKey;
        }

        let path = PmPkgKeyPath.empty();
        let i: usize = 0;

        while (i < key.len) {
            if (key[i] == '.') {
                // 禁止前导点号或连续空段 `..`
                return err.InvalidPath;
            }
            if (key[i] == ']') {
                return err.InvalidPath;
            }

            if (key[i] == '[') {
                // 解析方括号段 `[...]` 或 `["..."]` / `['...']`
                i += 1;
                if (i >= key.len) {
                    return err.UnclosedBracket;
                }
                let seg_text: []const u8 = "";
                let quoted = false;
                if (key[i] == '"' || key[i] == '\'') {
                    const q = key[i];
                    quoted = true;
                    i += 1;
                    const q_start = i;
                    while (i < key.len && key[i] != q) {
                        i += 1;
                    }
                    if (i >= key.len) {
                        return err.UnclosedBracket;
                    }
                    seg_text = key[q_start..i];
                    i += 1; // skip closing quote
                    if (i >= key.len || key[i] != ']') {
                        return err.UnclosedBracket;
                    }
                    i += 1; // skip ']'
                } else {
                    const b_start = i;
                    while (i < key.len && key[i] != ']') {
                        if (key[i] == '[') {
                            return err.InvalidPath;
                        }
                        i += 1;
                    }
                    if (i >= key.len) {
                        return err.UnclosedBracket;
                    }
                    seg_text = key[b_start..i];
                    i += 1; // skip ']'
                }

                if (seg_text.len == 0) {
                    return err.InvalidPath;
                }
                if (path.len >= PM_PKG_MAX_SEGMENTS) {
                    return err.TooManySegments;
                }
                if (!quoted) {
                    if (idx_val := parseStrictUsize(seg_text)) {
                        path.segments[path.len] = PmPkgPathSegment{
                            kind: .array_index,
                            key: seg_text,
                            index: idx_val,
                            from_bracket: true,
                        };
                    } else {
                        path.segments[path.len] = PmPkgPathSegment{
                            kind: .property,
                            key: seg_text,
                            index: 0,
                            from_bracket: true,
                        };
                    }
                } else {
                    path.segments[path.len] = PmPkgPathSegment{
                        kind: .property,
                        key: seg_text,
                        index: 0,
                        from_bracket: true,
                    };
                }
                path.len += 1;

                // 方括号之后必须是结尾、`.` 或下一个 `[`
                if (i < key.len) {
                    if (key[i] == '.') {
                        i += 1;
                        if (i >= key.len) {
                            return err.InvalidPath;
                        }
                    } else if (key[i] != '[') {
                        return err.InvalidPath;
                    }
                }
                continue;
            }

            // 解析普通点分属性段，直到 `.` 或 `[` 或结尾
            const start = i;
            while (i < key.len && key[i] != '.' && key[i] != '[') {
                if (key[i] == ']') {
                    return err.InvalidPath;
                }
                i += 1;
            }
            const seg = key[start..i];
            if (seg.len == 0) {
                return err.InvalidPath;
            }
            if (path.len >= PM_PKG_MAX_SEGMENTS) {
                return err.TooManySegments;
            }
            if (idx_val := parseStrictUsize(seg)) {
                path.segments[path.len] = PmPkgPathSegment{
                    kind: .array_index,
                    key: seg,
                    index: idx_val,
                    from_bracket: false,
                };
            } else {
                path.segments[path.len] = PmPkgPathSegment{
                    kind: .property,
                    key: seg,
                    index: 0,
                    from_bracket: false,
                };
            }
            path.len += 1;

            if (i < key.len && key[i] == '.') {
                i += 1;
                if (i >= key.len) {
                    return err.InvalidPath;
                }
            }
        }

        if (path.len == 0) {
            return err.EmptyKey;
        }
        return path;
    }

    /// 返回路径段数量
    fn count(): usize {
        return _.len;
    }

    /// 按索引读取路径段
    fn get(idx: usize): PmPkgPathSegment? {
        if (idx >= _.len) {
            return null;
        }
        return _.segments[idx];
    }

    /// 语义等价比较
    fn eql(other: PmPkgKeyPath): bool {
        if (_.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!_.segments[i].eql(other.segments[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 格式化为规范路径字符串（支持 Round-Trip 幂等：简单属性用 `.key`，数组下标用 `[N]`，特殊字符键用 `[key]` 或 `["key"]`）
    fn formatCanonical(out: []u8): usize! {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            const seg = _.segments[i];
            if (seg.kind == .array_index) {
                pos = writeByte(out, pos, '[')?;
                pos = writeUsize(out, pos, seg.index)?;
                pos = writeByte(out, pos, ']')?;
            } else if (isSimpleDotKey(seg.key) && parseStrictUsize(seg.key) == null) {
                if (i > 0) {
                    pos = writeByte(out, pos, '.')?;
                }
                pos = writeBytes(out, pos, seg.key)?;
            } else {
                pos = writeByte(out, pos, '[')?;
                // 若包含点号、空白、方括号或为纯数字字符串，使用双引号包裹以保证往返无歧义
                let need_quote = parseStrictUsize(seg.key) != null;
                let k: usize = 0;
                while (k < seg.key.len) {
                    const ch = seg.key[k];
                    if (ch == '.' || ch == '[' || ch == ']' || isWs(ch) || ch == '"' || ch == '\'') {
                        need_quote = true;
                        break;
                    }
                    k += 1;
                }
                if (need_quote) {
                    pos = writeByte(out, pos, '"')?;
                    pos = writeBytes(out, pos, seg.key)?;
                    pos = writeByte(out, pos, '"')?;
                } else {
                    pos = writeBytes(out, pos, seg.key)?;
                }
                pos = writeByte(out, pos, ']')?;
            }
            i += 1;
        }
        return pos;
    }
}

/// `pm pkg set <key>=<value>` 赋值解析结果
pub struct PmPkgAssignment {
    key: []const u8,
    value: []const u8,
    path: PmPkgKeyPath,

    /// 解析单条 `key=value` 赋值表达式
    static fn parse(expr: []const u8): PmPkgAssignment! {
        let eq_pos: usize = expr.len;
        let in_bracket = false;
        let in_quote: u8 = 0;
        let i: usize = 0;
        while (i < expr.len) {
            const c = expr[i];
            if (in_quote != 0) {
                if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '[') {
                in_bracket = true;
            } else if (c == ']') {
                in_bracket = false;
            } else if (c == '=' && !in_bracket) {
                eq_pos = i;
                break;
            }
            i += 1;
        }
        if (eq_pos == expr.len || eq_pos == 0) {
            return err.InvalidAssignment;
        }
        const key_slice = trimWs(expr[0..eq_pos]);
        const val_slice = expr[eq_pos + 1..];
        const parsed_path = PmPkgKeyPath.parse(key_slice)?;
        return PmPkgAssignment{
            key: key_slice,
            value: val_slice,
            path: parsed_path,
        };
    }
}

/// `pm pkg set` 值类型分类（区分普通字符串与 `--json` 模式下的 JSON 字面量）
pub enum PmPkgValueKind: u8 {
    raw_string = 0,
    json_null = 1,
    json_bool = 2,
    json_number = 3,
    json_string = 4,
    json_array = 5,
    json_object = 6,
}

fn isValidJsonNumber(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    if (s[i] == '-') {
        i += 1;
        if (i >= s.len) return false;
    }
    if (s[i] == '0') {
        i += 1;
    } else if (s[i] >= '1' && s[i] <= '9') {
        while (i < s.len && isDigit(s[i])) {
            i += 1;
        }
    } else {
        return false;
    }
    if (i < s.len && s[i] == '.') {
        i += 1;
        if (i >= s.len || !isDigit(s[i])) return false;
        while (i < s.len && isDigit(s[i])) {
            i += 1;
        }
    }
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        if (i < s.len && (s[i] == '+' || s[i] == '-')) {
            i += 1;
        }
        if (i >= s.len || !isDigit(s[i])) return false;
        while (i < s.len && isDigit(s[i])) {
            i += 1;
        }
    }
    return i == s.len;
}

/// 根据 `parse_json`（对应 `--json` 标志）分类并校验 `pm pkg set` 的值（对应上游 `parseValue`）
pub fn classifyPmPkgValue(raw: []const u8, parse_json: bool): PmPkgValueKind! {
    if (!parse_json) {
        return .raw_string;
    }
    const s = trimWs(raw);
    if (s.len == 0) {
        return err.InvalidJsonValue;
    }
    if (bytesEql(s, "null")) return .json_null;
    if (bytesEql(s, "true") || bytesEql(s, "false")) return .json_bool;
    if (s[0] == '"' && s.len >= 2 && s[s.len - 1] == '"') return .json_string;
    if (s[0] == '[' && s.len >= 2 && s[s.len - 1] == ']') return .json_array;
    if (s[0] == '{' && s.len >= 2 && s[s.len - 1] == '}') return .json_object;
    if (isValidJsonNumber(s)) return .json_number;
    return err.InvalidJsonValue;
}
