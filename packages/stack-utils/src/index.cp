// @cap/stack-utils — Zero-heap V8 stack trace frame parser
// Clean-room CP implementation inspired by npm:stack-utils (tapjs/stack-utils)

import { isSpace, trim, strEq, rfindByte } from "cap:ascii";
import { parseU32 } from "cap:fmt";

pub struct StackFrame {
    function_name: string,
    file: string,
    line: u32,
    column: u32,
    is_native: bool,
    valid: bool,
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
    if (strEq(loc, "native")) {
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

    if (c1 := rfindByte(loc, ':')) {
        const tail1 = loc[c1 + 1..loc.len];
        if (n1 := parseU32(tail1)) {
            const prefix1 = loc[0..c1];
            if (c2 := rfindByte(prefix1, ':')) {
                if (n2 := parseU32(prefix1[c2 + 1..prefix1.len])) {
                    line_num = n2;
                    col_num = n1;
                    file = prefix1[0..c2];
                } else {
                    line_num = n1;
                    col_num = 0;
                    file = prefix1;
                }
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
    const trimmed = trim(line);
    if (trimmed.len < 4) {
        return stkuInvalid();
    }
    if (trimmed[0] != 'a' || trimmed[1] != 't' || !isSpace(trimmed[2])) {
        return stkuInvalid();
    }

    const rest = trim(trimmed[3..trimmed.len]);
    if (rest.len == 0) {
        return stkuInvalid();
    }

    if (strEq(rest, "native")) {
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
                if (rest[i] == '(' && isSpace(rest[i - 1])) {
                    open_paren = i;
                    break;
                }
            }
        }
        if (open_paren < rest.len) {
            const fn_name = trim(rest[0..open_paren]);
            const loc = trim(rest[open_paren + 1..rest.len - 1]);
            return stkuParseLoc(fn_name, loc);
        }
    }

    return stkuParseLoc("", rest);
}
