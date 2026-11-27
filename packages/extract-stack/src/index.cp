// @cap/extract-stack — Zero-allocation & zero-copy Error.stack frame extractor for CP
// Clean-room MIT implementation with parity to npm `extract-stack` v2.

fn exsIsHspace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn exsIsFrameLine(line: string): bool {
    // Matches /^\s+at / (at least one leading space or tab followed by "at ")
    let i: usize = 0;
    while (i < line.len && exsIsHspace(line[i])) : (i += 1) {}
    if (i == 0) {
        return false;
    }
    if (i + 3 <= line.len && line[i] == 'a' && line[i + 1] == 't' && line[i + 2] == ' ') {
        return true;
    }
    return false;
}

fn exsFindFirstFrameOffset(stack: string): usize {
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= stack.len) {
        if (i == stack.len || stack[i] == '\n') {
            let line_end = i;
            if (line_end > line_start && stack[line_end - 1] == '\r') {
                line_end -= 1;
            }
            if (exsIsFrameLine(stack[line_start..line_end])) {
                return line_start;
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return stack.len;
}

fn exsTrimLine(line: string): string {
    let start: usize = 0;
    let end: usize = line.len;
    while (start < end && (line[start] == ' ' || line[start] == '\t' || line[start] == '\r')) {
        start += 1;
    }
    while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t' || line[end - 1] == '\r')) {
        end -= 1;
    }
    return line[start..end];
}

/// Zero-copy extraction of the actual stack frames portion from `Error.stack`,
/// skipping any single-line or multi-line `Error: message` header before the first
/// indented `    at ...` line. Returns `""` if no stack frame lines are found.
pub fn extractStack(stack: string): string {
    if (stack.len == 0) {
        return "";
    }
    const offset = exsFindFirstFrameOffset(stack);
    if (offset >= stack.len) {
        return "";
    }
    return stack[offset..stack.len];
}

/// Zero-copy extraction of the error header (name + possibly multi-line message)
/// preceding the first `    at ...` stack frame line, with trailing `\r`/`\n` stripped.
pub fn extractErrorHeader(stack: string): string {
    if (stack.len == 0) {
        return "";
    }
    let end = exsFindFirstFrameOffset(stack);
    while (end > 0 && (stack[end - 1] == '\n' || stack[end - 1] == '\r')) {
        end -= 1;
    }
    return stack[0..end];
}

/// Extracts individual stack frame lines from `stack`, stripping leading/trailing
/// whitespace and the leading `"at "` prefix from each frame, and writes slices into `out`.
/// Returns the number of frame lines written.
pub fn extractStackLines(stack: string, out: []string): usize {
    const frames = extractStack(stack);
    if (frames.len == 0 || out.len == 0) {
        return 0;
    }

    let count: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= frames.len && count < out.len) {
        if (i == frames.len || frames[i] == '\n') {
            const trimmed = exsTrimLine(frames[line_start..i]);
            if (trimmed.len > 0) {
                if (trimmed.len >= 3 && trimmed[0] == 'a' && trimmed[1] == 't' && trimmed[2] == ' ') {
                    out[count] = trimmed[3..trimmed.len];
                } else {
                    out[count] = trimmed;
                }
                count += 1;
            }
            line_start = i + 1;
        }
        i += 1;
    }

    return count;
}

pub const STACK_FMT_UNKNOWN: u8 = 0;
pub const STACK_FMT_V8: u8 = 1;
pub const STACK_FMT_ZIG: u8 = 2;

/// Parsed representation of a single V8/Node.js or Zig stack frame.
pub struct StackFrame {
    pub function_name: string,
    pub file: string,
    pub line: u32,
    pub column: u32,
    pub is_native: bool,
    pub is_async: bool,
    pub format: u8,
    pub valid: bool,
}

fn exsEmptyFrame(): StackFrame {
    return StackFrame {
        function_name: "",
        file: "",
        line: 0,
        column: 0,
        is_native: false,
        is_async: false,
        format: STACK_FMT_UNKNOWN,
        valid: false,
    };
}

fn exsParseU32(s: string): u32 {
    if (s.len == 0) {
        return 0;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return 0;
        }
        acc = acc * 10 + ((c - '0') as u32);
        i += 1;
    }
    return acc;
}

fn exsIsAllDigits(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn exsFindLastByte(s: string, ch: u8): i32 {
    let i: usize = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) {
            return i as i32;
        }
    }
    return -1;
}

fn exsStartsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn exsStrEq(a: string, b: string): bool {
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

fn exsFindSubstr(s: string, needle: string): i32 {
    if (needle.len == 0 || needle.len > s.len) {
        return -1;
    }
    let i: usize = 0;
    while (i + needle.len <= s.len) {
        if (exsStartsWith(s[i..s.len], needle)) {
            return i as i32;
        }
        i += 1;
    }
    return -1;
}

fn exsParseLocation(loc: string, frame: *StackFrame): bool {
    if (loc.len == 0) {
        return false;
    }
    if (exsStrEq(loc, "native")) {
        frame.file = "native";
        frame.is_native = true;
        return true;
    }
    const c2 = exsFindLastByte(loc, ':');
    if (c2 <= 0) {
        frame.file = loc;
        return false;
    }
    const last_part = loc[((c2 as usize) + 1)..loc.len];
    if (!exsIsAllDigits(last_part)) {
        frame.file = loc;
        return false;
    }
    const before_c2 = loc[0..(c2 as usize)];
    const c1 = exsFindLastByte(before_c2, ':');
    if (c1 > 0) {
        const mid_part = before_c2[((c1 as usize) + 1)..before_c2.len];
        if (exsIsAllDigits(mid_part)) {
            frame.file = before_c2[0..(c1 as usize)];
            frame.line = exsParseU32(mid_part);
            frame.column = exsParseU32(last_part);
            return true;
        }
    }
    frame.file = before_c2;
    frame.line = exsParseU32(last_part);
    frame.column = 0;
    return true;
}

/// Parses a single V8/Node.js (`    at fn (file:line:col)`, `    at file:line:col`)
/// or Zig (`/path/file.zig:line:col: 0xaddr in fnName (bin)`) stack frame line.
pub fn parseStackFrame(frame_line: string): StackFrame {
    let f = exsEmptyFrame();
    let s = exsTrimLine(frame_line);
    if (s.len == 0) {
        return f;
    }

    // Check Zig format: "<file>:<line>:<col>: 0x... in <fn> (<bin>)"
    const zig_in = exsFindSubstr(s, ": 0x");
    if (zig_in > 0) {
        const loc = s[0..(zig_in as usize)];
        if (exsParseLocation(loc, *f)) {
            f.format = STACK_FMT_ZIG;
            f.valid = true;
            const after_addr = s[((zig_in as usize) + 2)..s.len];
            const in_pos = exsFindSubstr(after_addr, " in ");
            if (in_pos >= 0) {
                let fn_part = after_addr[((in_pos as usize) + 4)..after_addr.len];
                const paren = exsFindLastByte(fn_part, '(');
                if (paren > 0) {
                    fn_part = exsTrimLine(fn_part[0..(paren as usize)]);
                }
                f.function_name = fn_part;
            }
            return f;
        }
    }

    // V8/Node format: optional leading "at ", optional "async "
    let is_v8 = false;
    if (exsStartsWith(s, "at ")) {
        s = exsTrimLine(s[3..s.len]);
        is_v8 = true;
    }
    if (exsStartsWith(s, "async ")) {
        f.is_async = true;
        s = exsTrimLine(s[6..s.len]);
        is_v8 = true;
    }

    // Pattern 1: "fnName (location)"
    if (s.len >= 3 && s[s.len - 1] == ')') {
        const open = exsFindLastByte(s, '(');
        if (open > 0) {
            const fn_name = exsTrimLine(s[0..(open as usize)]);
            const loc = s[((open as usize) + 1)..(s.len - 1)];
            if (exsParseLocation(loc, *f)) {
                f.function_name = fn_name;
                f.format = STACK_FMT_V8;
                f.valid = true;
                return f;
            }
        }
    }

    // Pattern 2: bare "file:line:col" (V8 top-level or Zig location-only)
    if (exsParseLocation(s, *f)) {
        f.format = if (is_v8) STACK_FMT_V8 else STACK_FMT_ZIG;
        f.valid = true;
        return f;
    }

    return f;
}

/// Parses all V8/Node.js or Zig stack frame lines from `stack` into `out`.
/// Returns the number of valid `StackFrame` entries written.
pub fn parseStackFrames(stack: string, out: []StackFrame): usize {
    if (stack.len == 0 || out.len == 0) {
        return 0;
    }
    let count: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= stack.len && count < out.len) {
        if (i == stack.len || stack[i] == '\n') {
            const line = stack[line_start..i];
            const parsed = parseStackFrame(line);
            if (parsed.valid) {
                out[count] = parsed;
                count += 1;
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return count;
}

/// Counts the number of valid V8/Node.js or Zig stack frames present in `stack`.
pub fn countStackFrames(stack: string): usize {
    if (stack.len == 0) {
        return 0;
    }
    let count: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= stack.len) {
        if (i == stack.len || stack[i] == '\n') {
            if (parseStackFrame(stack[line_start..i]).valid) {
                count += 1;
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return count;
}
