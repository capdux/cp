// @cap/stacktrace — Zero-allocation multi-language stack trace parser & cleaner for CP
// Clean-room MIT implementation inspired by npm `error-stack-parser`, `stacktrace-parser`, and `clean-stack`.
//
// Features:
// - Parses V8/Node/Chrome/Bun, SpiderMonkey/Firefox/Safari, and Rust/Zig/CP stack frames.
// - Zero heap allocation: all `StackFrame` strings are zero-copy slices into the original input.
// - Runtime internal frame detection (`node:internal/`, `internal/`, `node:`, `<anonymous>`, `native`).
// - Clean stack formatter (`cleanStack`, `formatFrame`) writing directly into caller-provided `out: []u8`.

import { trim, strEq, startsWith, findByte, rfindByte } from "cap:ascii";
import { writeStr, writeU64, parseU32 } from "cap:fmt";
import { indexOfBytes } from "cap:bin";

const MAX_FRAMES: usize = 32;

pub struct StackFrame {
    pub func_name: string,
    pub file: string,
    pub line: u32,
    pub column: u32,
    pub is_native: bool,
    pub is_internal: bool,

    pub static fn empty() -> StackFrame {
        return StackFrame{
            func_name: "",
            file: "",
            line: 0,
            column: 0,
            is_native: false,
            is_internal: false,
        };
    }
}

pub struct ParsedStack {
    pub frames: [MAX_FRAMES]StackFrame,
    pub count: usize,
    pub message: string,

    pub static fn empty() -> ParsedStack {
        let arr: [MAX_FRAMES]StackFrame = undefined;
        return ParsedStack{
            frames: arr,
            count: 0,
            message: "",
        };
    }

    pub fn frameAt(i: usize) -> StackFrame {
        if (i >= _.count) {
            return StackFrame.empty();
        }
        return _.frames[i];
    }
}

fn isInternalLocation(file: string, is_native: bool) -> bool {
    if (is_native) {
        return true;
    }
    if (startsWith(file, "node:internal/") || startsWith(file, "internal/") || startsWith(file, "node:")) {
        return true;
    }
    if (strEq(file, "<anonymous>") || strEq(file, "native") || strEq(file, "[native code]")) {
        return true;
    }
    return false;
}

struct LocationInfo {
    file: string,
    line: u32,
    column: u32,
    is_native: bool,
}

fn parseLocation(raw_loc: string) -> LocationInfo {
    const loc = trim(raw_loc);
    if (strEq(loc, "native") || strEq(loc, "[native code]")) {
        return LocationInfo{ file: loc, line: 0, column: 0, is_native: true };
    }

    // Find rightmost ':'
    if (c1 := rfindByte(loc, ':')) {
        const n1_opt = parseU32(loc[(c1 + 1)..loc.len]);
        if (n1_opt != null) {
            const n1 = n1_opt.?;
            // Check for a second ':' before c1 (file:line:column)
            if (c2 := rfindByte(loc[0..c1], ':')) {
                const n2_opt = parseU32(loc[(c2 + 1)..c1]);
                if (n2_opt != null) {
                    return LocationInfo{
                        file: loc[0..c2],
                        line: n2_opt.?,
                        column: n1,
                        is_native: false,
                    };
                }
            }
            return LocationInfo{
                file: loc[0..c1],
                line: n1,
                column: 0,
                is_native: false,
            };
        }
    }

    return LocationInfo{ file: loc, line: 0, column: 0, is_native: false };
}

/// Parses a single stack trace line from V8/Node/Bun, SpiderMonkey/Safari, or Rust/Zig/CP formats.
/// Returns `null` if the line is not a recognized stack frame.
pub fn parseFrameLine(raw_line: string) -> StackFrame? {
    const s = trim(raw_line);
    if (s.len == 0) {
        return null;
    }

    // 1. V8 / Node / Chrome / Bun format: `at ...`
    if (startsWith(s, "at ")) {
        const rest = trim(s[3..s.len]);
        if (rest.len == 0) {
            return null;
        }
        if (strEq(rest, "native")) {
            return StackFrame{
                func_name: "<native>",
                file: "native",
                line: 0,
                column: 0,
                is_native: true,
                is_internal: true,
            };
        }

        // `at funcName (location)`
        if (rest[rest.len - 1] == ')') {
            let open_p: usize = rest.len;
            let k: usize = rest.len - 1;
            while (k > 0) {
                k -= 1;
                if (rest[k] == '(') {
                    open_p = k;
                    break;
                }
            }
            if (open_p < rest.len) {
                const func_part = trim(rest[0..open_p]);
                const loc_part = trim(rest[(open_p + 1)..(rest.len - 1)]);
                const loc = parseLocation(loc_part);
                const fname = if (func_part.len > 0) { func_part } else { "<anonymous>" };
                return StackFrame{
                    func_name: fname,
                    file: loc.file,
                    line: loc.line,
                    column: loc.column,
                    is_native: loc.is_native,
                    is_internal: isInternalLocation(loc.file, loc.is_native),
                };
            }
        }

        // `at /path/to/file.ts:42:13`
        const loc = parseLocation(rest);
        return StackFrame{
            func_name: "<anonymous>",
            file: loc.file,
            line: loc.line,
            column: loc.column,
            is_native: loc.is_native,
            is_internal: isInternalLocation(loc.file, loc.is_native),
        };
    }

    // 2. Zig / Rust / CP address frame: `0x1234 in handleRequest (src/server.zig:120)`
    const in_idx = indexOfBytes(s, " in ", 0) ?? s.len;
    if (in_idx < s.len && s[s.len - 1] == ')') {
        let open_p: usize = s.len;
        let k: usize = s.len - 1;
        while (k > in_idx + 4) {
            k -= 1;
            if (s[k] == '(') {
                open_p = k;
                break;
            }
        }
        if (open_p < s.len && open_p > in_idx + 4) {
            const func_part = trim(s[(in_idx + 4)..open_p]);
            const loc_part = trim(s[(open_p + 1)..(s.len - 1)]);
            const loc = parseLocation(loc_part);
            if (loc.file.len > 0) {
                return StackFrame{
                    func_name: func_part,
                    file: loc.file,
                    line: loc.line,
                    column: loc.column,
                    is_native: loc.is_native,
                    is_internal: isInternalLocation(loc.file, loc.is_native),
                };
            }
        }
    }

    // 3. SpiderMonkey / Firefox / Safari format: `myFunc@https://example.com/app.js:42:13`
    if (at_idx := findByte(s, '@', 0)) {
        if (at_idx + 1 < s.len) {
            const func_part = trim(s[0..at_idx]);
            const loc_part = trim(s[(at_idx + 1)..s.len]);
            // Avoid matching email addresses in error messages (require line > 0 or [native code])
            const loc = parseLocation(loc_part);
            if (loc.is_native || loc.line > 0) {
                const fname = if (func_part.len > 0) { func_part } else { "<anonymous>" };
                return StackFrame{
                    func_name: fname,
                    file: loc.file,
                    line: loc.line,
                    column: loc.column,
                    is_native: loc.is_native,
                    is_internal: isInternalLocation(loc.file, loc.is_native),
                };
            }
        }
    }

    // 4. Bare Rust / Zig / CP location line: `src/main.cp:88:12` or `src/server.zig:120`
    if (findByte(s, ' ', 0) == null) {
        const loc = parseLocation(s);
        if (loc.line > 0 && loc.file.len > 0 && (findByte(loc.file, '.', 0) != null || findByte(loc.file, '/', 0) != null)) {
            return StackFrame{
                func_name: "<anonymous>",
                file: loc.file,
                line: loc.line,
                column: loc.column,
                is_native: false,
                is_internal: isInternalLocation(loc.file, false),
            };
        }
    }

    return null;
}

/// Parses a full multi-line stack trace into a zero-allocation `ParsedStack`.
pub fn parseStack(raw: string) -> ParsedStack {
    let result = ParsedStack.empty();
    let cur: usize = 0;

    while (cur < raw.len) {
        const ls = cur;
        while (cur < raw.len && raw[cur] != '\n') {
            cur += 1;
        }
        let le = cur;
        if (le > ls && raw[le - 1] == '\r') {
            le -= 1;
        }
        if (cur < raw.len && raw[cur] == '\n') {
            cur += 1;
        }

        const line = raw[ls..le];
        const frame_opt = parseFrameLine(line);
        if (frame_opt != null) {
            if (result.count < MAX_FRAMES) {
                result.frames[result.count] = frame_opt.?;
                result.count += 1;
            }
        } else if (result.count == 0 && result.message.len == 0) {
            const t = trim(line);
            if (t.len > 0) {
                result.message = t;
            }
        }
    }

    return result;
}

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn formatFrameAt(f: StackFrame, out: []u8, start_pos: usize) -> usize {
    let pos = writeStr("    at ", out, start_pos);
    const has_named_func = f.func_name.len > 0 && !strEq(f.func_name, "<anonymous>") && !strEq(f.func_name, "<native>");

    if (has_named_func) {
        pos = writeStr(f.func_name, out, pos);
        pos = writeStr(" (", out, pos);
    }

    if (f.is_native) {
        pos = writeStr("native", out, pos);
    } else {
        pos = writeStr(f.file, out, pos);
        if (f.line > 0) {
            pos = pushByte(out, pos, ':');
            pos = writeU64(f.line as u64, out, pos);
            if (f.column > 0) {
                pos = pushByte(out, pos, ':');
                pos = writeU64(f.column as u64, out, pos);
            }
        }
    }

    if (has_named_func) {
        pos = pushByte(out, pos, ')');
    }
    return pos;
}

/// Formats a single `StackFrame` into canonical V8-style output inside `out`.
pub fn formatFrame(f: StackFrame, out: []u8) -> string {
    const end = formatFrameAt(f, out, 0);
    return out[0..end];
}

/// Parses `raw`, strips runtime internal frames (`is_internal == true`), and formats a clean stack trace into `out`.
pub fn cleanStack(raw: string, out: []u8) -> string {
    const parsed = parseStack(raw);
    let pos: usize = 0;

    if (parsed.message.len > 0) {
        pos = writeStr(parsed.message, out, pos);
    }

    let i: usize = 0;
    while (i < parsed.count) {
        const f = parsed.frames[i];
        i += 1;
        if (f.is_internal) {
            continue;
        }
        if (pos > 0) {
            pos = pushByte(out, pos, '\n');
        }
        pos = formatFrameAt(f, out, pos);
    }

    return out[0..pos];
}
