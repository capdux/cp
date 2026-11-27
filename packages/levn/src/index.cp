// @cap/levn — Zero-heap Light Value Notation (levn) scalar, list, and object parser.

pub enum LevnKind {
    Null,
    Bool,
    Int,
    Float,
    String,
    Invalid,
}

pub struct LevnValue {
    kind: LevnKind,
    bool_val: bool,
    int_val: i64,
    float_val: f64,
    str_val: string,
}

pub struct LevnPair {
    key: string,
    value: LevnValue,
}

fn lvnIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn lvnTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && lvnIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && lvnIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn lvnStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn lvnMakeInvalid(): LevnValue {
    return LevnValue{
        kind: LevnKind.Invalid,
        bool_val: false,
        int_val: 0,
        float_val: 0.0,
        str_val: "",
    };
}

fn lvnTryParseNumber(s: string): LevnValue {
    if (s.len == 0) return lvnMakeInvalid();

    let i: usize = 0;
    let neg: bool = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }
    if (i >= s.len) return lvnMakeInvalid();

    let int_digits: usize = 0;
    let int_acc: i64 = 0;
    let float_acc: f64 = 0.0;

    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        const d: i64 = i64(s[i] - '0');
        int_acc = int_acc * 10 + d;
        float_acc = float_acc * 10.0 + f64(d);
        int_digits += 1;
        i += 1;
    }

    if (i == s.len && int_digits > 0) {
        const signed_int: i64 = if (neg) -int_acc else int_acc;
        return LevnValue{
            kind: LevnKind.Int,
            bool_val: false,
            int_val: signed_int,
            float_val: 0.0,
            str_val: s,
        };
    }

    let is_float: bool = false;
    let frac_digits: usize = 0;
    let frac_div: f64 = 1.0;
    if (i < s.len && s[i] == '.') {
        is_float = true;
        i += 1;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            const d: i64 = i64(s[i] - '0');
            float_acc = float_acc * 10.0 + f64(d);
            frac_div *= 10.0;
            frac_digits += 1;
            i += 1;
        }
        if (frac_digits > 0) {
            float_acc = float_acc / frac_div;
        }
    }

    if (int_digits == 0 && frac_digits == 0) {
        return lvnMakeInvalid();
    }

    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        is_float = true;
        i += 1;
        let exp_neg: bool = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        let exp_digits: usize = 0;
        let exp_val: i32 = 0;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            exp_val = exp_val * 10 + i32(s[i] - '0');
            exp_digits += 1;
            i += 1;
        }
        if (exp_digits == 0) return lvnMakeInvalid();
        let e: i32 = 0;
        while (e < exp_val) {
            if (exp_neg) {
                float_acc *= 0.1;
            } else {
                float_acc *= 10.0;
            }
            e += 1;
        }
    }

    if (i != s.len || !is_float) {
        return lvnMakeInvalid();
    }

    const signed_float: f64 = if (neg) -float_acc else float_acc;
    return LevnValue{
        kind: LevnKind.Float,
        bool_val: false,
        int_val: 0,
        float_val: signed_float,
        str_val: s,
    };
}

pub fn parseLevnScalar(input: string): LevnValue {
    const s = lvnTrim(input);
    if (s.len == 0) {
        return lvnMakeInvalid();
    }

    if (lvnStrEq(s, "null")) {
        return LevnValue{
            kind: LevnKind.Null,
            bool_val: false,
            int_val: 0,
            float_val: 0.0,
            str_val: s,
        };
    }

    if (lvnStrEq(s, "true")) {
        return LevnValue{
            kind: LevnKind.Bool,
            bool_val: true,
            int_val: 0,
            float_val: 0.0,
            str_val: s,
        };
    }

    if (lvnStrEq(s, "false")) {
        return LevnValue{
            kind: LevnKind.Bool,
            bool_val: false,
            int_val: 0,
            float_val: 0.0,
            str_val: s,
        };
    }

    const first = s[0];
    if (first == '"' || first == '\'') {
        if (s.len >= 2 && s[s.len - 1] == first) {
            return LevnValue{
                kind: LevnKind.String,
                bool_val: false,
                int_val: 0,
                float_val: 0.0,
                str_val: s[1..s.len - 1],
            };
        }
        return lvnMakeInvalid();
    }

    const num_val = lvnTryParseNumber(s);
    if (num_val.kind != LevnKind.Invalid) {
        return num_val;
    }

    return LevnValue{
        kind: LevnKind.String,
        bool_val: false,
        int_val: 0,
        float_val: 0.0,
        str_val: s,
    };
}

fn lvnFindTopLevelChar(s: string, start: usize, target: u8): usize {
    let i = start;
    let quote: u8 = 0;
    let depth: i32 = 0;

    while (i < s.len) {
        const c = s[i];
        if (quote != 0) {
            if (c == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (c == quote) {
                quote = 0;
            }
            i += 1;
            continue;
        }
        if (c == '"' || c == '\'') {
            quote = c;
            i += 1;
            continue;
        }
        if (c == '[' || c == '{' || c == '(') {
            depth += 1;
            i += 1;
            continue;
        }
        if (c == ']' || c == '}' || c == ')') {
            if (depth > 0) depth -= 1;
            i += 1;
            continue;
        }
        if (depth == 0 && c == target) {
            return i;
        }
        i += 1;
    }
    return s.len;
}

fn lvnUnquoteKey(s: string): string {
    const t = lvnTrim(s);
    if (t.len >= 2 && ((t[0] == '"' && t[t.len - 1] == '"') || (t[0] == '\'' && t[t.len - 1] == '\''))) {
        return t[1..t.len - 1];
    }
    return t;
}

pub fn parseLevnList(input: string, out: []LevnValue): usize {
    let s = lvnTrim(input);
    if (s.len >= 2 && s[0] == '[' && s[s.len - 1] == ']') {
        s = lvnTrim(s[1..s.len - 1]);
    }
    if (s.len == 0) return 0;

    let count: usize = 0;
    let start: usize = 0;
    while (start <= s.len && count < out.len) {
        const comma = lvnFindTopLevelChar(s, start, ',');
        const item = lvnTrim(s[start..comma]);
        if (item.len > 0) {
            out[count] = parseLevnScalar(item);
            count += 1;
        }
        if (comma >= s.len) break;
        start = comma + 1;
    }
    return count;
}

pub fn parseLevnObject(input: string, out: []LevnPair): usize {
    let s = lvnTrim(input);
    if (s.len >= 2 && s[0] == '{' && s[s.len - 1] == '}') {
        s = lvnTrim(s[1..s.len - 1]);
    }
    if (s.len == 0) return 0;

    let count: usize = 0;
    let start: usize = 0;
    while (start <= s.len && count < out.len) {
        const comma = lvnFindTopLevelChar(s, start, ',');
        const entry = lvnTrim(s[start..comma]);
        if (entry.len > 0) {
            const colon = lvnFindTopLevelChar(entry, 0, ':');
            if (colon < entry.len) {
                const key = lvnUnquoteKey(entry[0..colon]);
                const val_str = entry[colon + 1..entry.len];
                if (key.len > 0) {
                    out[count] = LevnPair{
                        key: key,
                        value: parseLevnScalar(val_str),
                    };
                    count += 1;
                }
            }
        }
        if (comma >= s.len) break;
        start = comma + 1;
    }
    return count;
}
