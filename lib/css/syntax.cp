// Source: Absorbed from /data/dev/bun/src/css/values/syntax.cp | Synced: 2026-11-21T17:10:30Z

pub err Error {
    EmptySyntax,
    UnclosedTypeName,
    UnknownTypeName,
    MultiplierNotAllowed,
    InvalidComponent,
    UnexpectedToken,
    BufferTooSmall,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isIdentStart(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 0x80 || c == '_';
}

fn isNameCodePoint(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9') || c == '-';
}

fn trimLeftSpaces(s: string): string {
    let i: usize = 0;
    while (i < s.len && isSpace(s[i])) {
        i += 1;
    }
    return s[i..];
}

fn trimSpaces(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// CSS `<syntax>` 组件重复乘数
pub enum Multiplier: u8 {
    none = 0,
    space = 1, // '+'
    comma = 2, // '#'

    fn asStr(): string {
        return switch (_) {
            .none => "",
            .space => "+",
            .comma => "#",
        };
    }
}

/// CSS Properties and Values API Level 1 标准类型与自定义字面量枚举
pub enum SyntaxType: u8 {
    length = 0,
    number = 1,
    percentage = 2,
    length_percentage = 3,
    color = 4,
    image = 5,
    url = 6,
    integer = 7,
    angle = 8,
    time = 9,
    resolution = 10,
    transform_function = 11,
    transform_list = 12,
    custom_ident = 13,
    string_type = 14,
    literal = 15,

    /// 从 `<name>` 内部的类型名解析（ASCII 大小写无关）
    static fn fromTypeName(type_name: string): SyntaxType? {
        if (eqlIgnoreCaseAscii(type_name, "length")) { return .length; }
        if (eqlIgnoreCaseAscii(type_name, "number")) { return .number; }
        if (eqlIgnoreCaseAscii(type_name, "percentage")) { return .percentage; }
        if (eqlIgnoreCaseAscii(type_name, "length-percentage")) { return .length_percentage; }
        if (eqlIgnoreCaseAscii(type_name, "color")) { return .color; }
        if (eqlIgnoreCaseAscii(type_name, "image")) { return .image; }
        if (eqlIgnoreCaseAscii(type_name, "url")) { return .url; }
        if (eqlIgnoreCaseAscii(type_name, "integer")) { return .integer; }
        if (eqlIgnoreCaseAscii(type_name, "angle")) { return .angle; }
        if (eqlIgnoreCaseAscii(type_name, "time")) { return .time; }
        if (eqlIgnoreCaseAscii(type_name, "resolution")) { return .resolution; }
        if (eqlIgnoreCaseAscii(type_name, "transform-function")) { return .transform_function; }
        if (eqlIgnoreCaseAscii(type_name, "transform-list")) { return .transform_list; }
        if (eqlIgnoreCaseAscii(type_name, "custom-ident")) { return .custom_ident; }
        if (eqlIgnoreCaseAscii(type_name, "string")) { return .string_type; }
        return null;
    }

    fn asTypeStr(): string {
        return switch (_) {
            .length => "<length>",
            .number => "<number>",
            .percentage => "<percentage>",
            .length_percentage => "<length-percentage>",
            .color => "<color>",
            .image => "<image>",
            .url => "<url>",
            .integer => "<integer>",
            .angle => "<angle>",
            .time => "<time>",
            .resolution => "<resolution>",
            .transform_function => "<transform-function>",
            .transform_list => "<transform-list>",
            .custom_ident => "<custom-ident>",
            .string_type => "<string>",
            .literal => "",
        };
    }

    /// 预乘类型 `<transform-list>` 禁止再附加 `+` 或 `#` 乘数
    fn allowsMultiplier(): bool {
        return _ != .transform_list;
    }
}

/// 单个 CSS `<syntax>` 组件（类型 + 可选字面量 + 重复乘数）
pub struct SyntaxComponent {
    kind: SyntaxType,
    literal: string = "",
    multiplier: Multiplier = .none,

    fn eql(other: SyntaxComponent): bool {
        if (_.kind != other.kind || _.multiplier != other.multiplier) {
            return false;
        }
        if (_.kind == .literal) {
            return std.mem.eql(u8, _.literal, other.literal);
        }
        return true;
    }

    /// 序列化单个组件写入缓冲区
    fn toCss(buf: []u8): string! {
        const base = if (_.kind == .literal) _.literal else _.kind.asTypeStr();
        const mult = _.multiplier.asStr();
        const total = base.len + mult.len;
        if (buf.len < total) {
            return err.BufferTooSmall;
        }
        @memcpy(buf[0..base.len], base);
        if (mult.len > 0) {
            @memcpy(buf[base.len..total], mult);
        }
        return buf[0..total];
    }
}

/// 已解析的 CSS `<syntax>` 语法定义（支持 `*` 通用语法与 `|` 多分支，含 `~init()` RAII 自动释放）
pub struct SyntaxString {
    allocator: std.mem.Allocator? = null,
    is_universal: bool = false,
    components: ([]const SyntaxComponent)? = null,

    ~init() {
        if (slice := _.components) {
            if (alloc := _.allocator) {
                alloc.free(slice);
            }
            _.components = null;
        }
    }

    /// 解析 CSS `<syntax>` 字符串
    static fn parse(allocator: std.mem.Allocator, input: string): SyntaxString! {
        const trimmed = trimSpaces(input);
        if (trimmed.len == 0) {
            return err.EmptySyntax;
        }

        if (std.mem.eql(u8, trimmed, "*")) {
            return SyntaxString{
                allocator: null,
                is_universal: true,
                components: null,
            };
        }

        let list = std.array_list.Managed(SyntaxComponent).init(allocator);
        defer list.deinit();

        let rem = trimmed;
        while (true) {
            rem = trimLeftSpaces(rem);
            if (rem.len == 0) {
                return err.InvalidComponent;
            }

            let kind: SyntaxType = .length;
            let lit: string = "";

            if (rem[0] == '<') {
                let close_idx: usize? = null;
                let j: usize = 1;
                while (j < rem.len) {
                    if (rem[j] == '>') {
                        close_idx = j;
                        break;
                    }
                    j += 1;
                }
                const end_pos = close_idx ?? return err.UnclosedTypeName;
                const type_name = rem[1..end_pos];
                if (parsed_kind := SyntaxType.fromTypeName(type_name)) {
                    kind = parsed_kind;
                } else {
                    return err.UnknownTypeName;
                }
                rem = rem[end_pos + 1 ..];
            } else if (isIdentStart(rem[0]) || (rem[0] == '-' && rem.len > 1 && (isIdentStart(rem[1]) || rem[1] == '-'))) {
                let end_pos: usize = 0;
                while (end_pos < rem.len && isNameCodePoint(rem[end_pos])) {
                    end_pos += 1;
                }
                kind = .literal;
                lit = rem[0..end_pos];
                rem = rem[end_pos..];
            } else {
                return err.InvalidComponent;
            }

            let mult: Multiplier = .none;
            if (rem.len > 0 && (rem[0] == '+' || rem[0] == '#')) {
                if (!kind.allowsMultiplier()) {
                    return err.MultiplierNotAllowed;
                }
                mult = if (rem[0] == '+') .space else .comma;
                rem = rem[1..];
            }

            list.append(SyntaxComponent{
                kind: kind,
                literal: lit,
                multiplier: mult,
            })?;

            rem = trimLeftSpaces(rem);
            if (rem.len == 0) {
                break;
            }
            if (rem[0] == '|') {
                rem = rem[1..];
                continue;
            }
            return err.UnexpectedToken;
        }

        const owned = list.toOwnedSlice()?;
        return SyntaxString{
            allocator: allocator,
            is_universal: false,
            components: owned,
        };
    }

    fn isUniversal(): bool {
        return _.is_universal;
    }

    fn len(): usize {
        if (slice := _.components) {
            return slice.len;
        }
        return 0;
    }

    fn items(): []const SyntaxComponent {
        if (slice := _.components) {
            return slice;
        }
        const empty: [0]SyntaxComponent = [];
        return empty[0..0];
    }

    /// 规范序列化（不含外层双引号），如 `"*"` 或 `"<length>+ | <color>"`
    fn toCss(buf: []u8): string! {
        if (_.is_universal) {
            if (buf.len < 1) {
                return err.BufferTooSmall;
            }
            buf[0] = '*';
            return buf[0..1];
        }

        const comps = _.items();
        let pos: usize = 0;
        let i: usize = 0;
        while (i < comps.len) {
            if (i > 0) {
                if (pos + 3 > buf.len) {
                    return err.BufferTooSmall;
                }
                buf[pos] = ' ';
                buf[pos + 1] = '|';
                buf[pos + 2] = ' ';
                pos += 3;
            }
            const written = comps[i].toCss(buf[pos..])?;
            pos += written.len;
            i += 1;
        }
        return buf[0..pos];
    }

    /// 规范序列化（带 CSS 字符串外层双引号），如 `"\"<length>+ | <color>\""`
    fn toCssQuoted(buf: []u8): string! {
        if (buf.len < 2) {
            return err.BufferTooSmall;
        }
        buf[0] = '"';
        const inner = _.toCss(buf[1 .. buf.len - 1])?;
        buf[1 + inner.len] = '"';
        return buf[0 .. inner.len + 2];
    }
}
