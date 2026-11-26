// @cap/stack-utils — Zero-heap V8 stack trace frame parser
// Clean-room CP implementation inspired by npm:stack-utils (tapjs/stack-utils)

pub struct StackFrame {
    function_name: string,
    file: string,
    line: u32,
    column: u32,
    is_native: bool,
    valid: bool,
}

fn stkuIsSpace(ch: u8): bool {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

fn stkuTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && stkuIsSpace(s[start])) : (start += 1) {}
    let end = s.len;
    while (end > start && stkuIsSpace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn stkuEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn stkuIsDigits(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
    }
    return true;
}

fn stkuParseU32(s: string): u32 {
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const d = u32(s[i] - '0');
        acc = acc * 10 + d;
    }
    return acc;
}

fn stkuLastColon(s: string): usize {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ':') {
            return i;
        }
    }
    return s.len;
}

fn stkuInvalid(): StackFrame {
    return StackFrame{
        function_name: "",
        file: "",
        line: 0,
        column: 0,
        is_native: false,
        valid: false,
    };
}

fn stkuParseLoc(fn_name: string, loc: string): StackFrame {
    if (loc.len == 0) {
        return stkuInvalid();
    }
    if (stkuEq(loc, "native")) {
        return StackFrame{
            function_name: fn_name,
            file: "",
            line: 0,
            column: 0,
            is_native: true,
            valid: true,
        };
    }

    let file = loc;
    let line_num: u32 = 0;
    let col_num: u32 = 0;

    const c1 = stkuLastColon(loc);
    if (c1 < loc.len) {
        const tail1 = loc[c1 + 1..loc.len];
        if (stkuIsDigits(tail1)) {
            const n1 = stkuParseU32(tail1);
            const prefix1 = loc[0..c1];
            const c2 = stkuLastColon(prefix1);
            if (c2 < prefix1.len && stkuIsDigits(prefix1[c2 + 1..prefix1.len])) {
                line_num = stkuParseU32(prefix1[c2 + 1..prefix1.len]);
                col_num = n1;
                file = prefix1[0..c2];
            } else {
                line_num = n1;
                col_num = 0;
                file = prefix1;
            }
        }
    }

    return StackFrame{
        function_name: fn_name,
        file: file,
        line: line_num,
        column: col_num,
        is_native: false,
        valid: true,
    };
}

/// Parse a single V8 stack trace line into a `StackFrame` with zero heap allocation.
/// Non-frame lines (e.g. error headers or blank lines) return `valid: false`.
pub fn parseStackLine(line: string): StackFrame {
    const trimmed = stkuTrim(line);
    if (trimmed.len < 4) {
        return stkuInvalid();
    }
    if (trimmed[0] != 'a' || trimmed[1] != 't' || !stkuIsSpace(trimmed[2])) {
        return stkuInvalid();
    }

    const rest = stkuTrim(trimmed[3..trimmed.len]);
    if (rest.len == 0) {
        return stkuInvalid();
    }

    if (stkuEq(rest, "native")) {
        return StackFrame{
            function_name: "",
            file: "",
            line: 0,
            column: 0,
            is_native: true,
            valid: true,
        };
    }

    // Check for `fnName (location)` or `(location)` form
    if (rest[rest.len - 1] == ')') {
        let open_paren = rest.len;
        if (rest[0] == '(') {
            open_paren = 0;
        } else {
            let i: usize = 1;
            while (i < rest.len) : (i += 1) {
                if (rest[i] == '(' && stkuIsSpace(rest[i - 1])) {
                    open_paren = i;
                    break;
                }
            }
        }
        if (open_paren < rest.len) {
            const fn_name = stkuTrim(rest[0..open_paren]);
            const loc = stkuTrim(rest[open_paren + 1..rest.len - 1]);
            return stkuParseLoc(fn_name, loc);
        }
    }

    return stkuParseLoc("", rest);
}
