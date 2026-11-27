// @cap/svg-path — Zero-allocation SVG `<path d="...">` parser, bounds & formatter for CP
// Clean-room MIT implementation inspired by npm `parse-svg-path`, `svg-path-parser`,
// and Rust `svgtypes`.

import { isSpace, isDigit } from "cap:ascii";
import { writeU64, writeStr } from "cap:fmt";

pub const MAX_PATH_CMDS: usize = 64;

pub struct PathCmd {
    pub code: u8,
    pub relative: bool,
    pub args: [7]f64,
    pub arg_count: u8,

    pub static fn empty() -> PathCmd {
        return PathCmd{
            code: 0,
            relative: false,
            args: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            arg_count: 0,
        };
    }
}

pub struct ParsedPath {
    pub cmds: [64]PathCmd,
    pub count: usize,
    pub valid: bool,

    pub static fn invalid() -> ParsedPath {
        let cmds: [64]PathCmd = undefined;
        let i: usize = 0;
        while (i < MAX_PATH_CMDS) {
            cmds[i] = PathCmd.empty();
            i += 1;
        }
        return ParsedPath{
            cmds: cmds,
            count: 0,
            valid: false,
        };
    }

    pub fn cmdAt(i: usize) -> PathCmd? {
        if (i < _.count) {
            return _.cmds[i];
        }
        return null;
    }
}

pub struct PathBounds {
    pub valid: bool,
    pub min_x: f64,
    pub min_y: f64,
    pub max_x: f64,
    pub max_y: f64,
    pub width: f64,
    pub height: f64,

    pub static fn empty() -> PathBounds {
        return PathBounds{
            valid: false,
            min_x: 0.0,
            min_y: 0.0,
            max_x: 0.0,
            max_y: 0.0,
            width: 0.0,
            height: 0.0,
        };
    }
}

// ---------------------------------------------------------------------------
// Internal Lexer & Number Parsing Helpers
// ---------------------------------------------------------------------------

fn skipWs(s: string, start: usize) -> usize {
    let i = start;
    while (i < s.len && isSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn skipWsComma(s: string, start: usize) -> usize {
    let i = skipWs(s, start);
    if (i < s.len && s[i] == ',') {
        i = skipWs(s, i + 1);
    }
    return i;
}

fn expectedArgCount(code: u8) -> u8? {
    switch (code) {
        'Z', 'z' => return 0,
        'H', 'h', 'V', 'v' => return 1,
        'M', 'm', 'L', 'l', 'T', 't' => return 2,
        'S', 's', 'Q', 'q' => return 4,
        'C', 'c' => return 6,
        'A', 'a' => return 7,
        else => return null,
    }
}

fn isLowerCmd(code: u8) -> bool {
    return code >= 'a' && code <= 'z';
}

fn isNumberStart(c: u8) -> bool {
    return isDigit(c) || c == '+' || c == '-' || c == '.';
}

struct ParsedNum {
    ok: bool,
    val: f64,
    next_pos: usize,
}

fn pow10(exp: i32) -> f64 {
    let res: f64 = 1.0;
    if (exp > 0) {
        let e = exp;
        while (e > 0) {
            res *= 10.0;
            e -= 1;
        }
    } else if (exp < 0) {
        let e = -exp;
        while (e > 0) {
            res /= 10.0;
            e -= 1;
        }
    }
    return res;
}

/// Parses an SVG number: optional sign, integer/fraction digits (supports `.5`), optional `e`/`E` exponent.
fn parseSvgNumber(s: string, start: usize) -> ParsedNum {
    const bad = ParsedNum{ ok: false, val: 0.0, next_pos: start };
    let i = skipWsComma(s, start);
    if (i >= s.len) {
        return bad;
    }

    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }

    let int_part: f64 = 0.0;
    let has_digits = false;
    while (i < s.len && isDigit(s[i])) {
        int_part = int_part * 10.0 + ((s[i] - '0') as f64);
        has_digits = true;
        i += 1;
    }

    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 1.0;
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && isDigit(s[i])) {
            frac_part = frac_part * 10.0 + ((s[i] - '0') as f64);
            frac_scale *= 10.0;
            has_digits = true;
            i += 1;
        }
    }

    if (!has_digits) {
        return bad;
    }

    let val = int_part + (frac_part / frac_scale);

    // Optional scientific notation exponent (e.g. 1e-3, 2.5E+2)
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        const exp_mark = i;
        i += 1;
        let exp_neg = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        let exp_val: i32 = 0;
        let has_exp_digits = false;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 300) {
                exp_val = exp_val * 10 + ((s[i] - '0') as i32);
            }
            has_exp_digits = true;
            i += 1;
        }
        if (!has_exp_digits) {
            i = exp_mark;
        } else {
            val *= pow10(if (exp_neg) -exp_val else exp_val);
        }
    }

    if (neg) {
        val = -val;
    }
    return ParsedNum{ ok: true, val: val, next_pos: i };
}

/// Parses a single SVG arc flag (`'0'` or `'1'`), allowing adjacent flags like `1150`.
fn parseSvgFlag(s: string, start: usize) -> ParsedNum {
    const bad = ParsedNum{ ok: false, val: 0.0, next_pos: start };
    const i = skipWsComma(s, start);
    if (i >= s.len) {
        return bad;
    }
    if (s[i] == '0') {
        return ParsedNum{ ok: true, val: 0.0, next_pos: i + 1 };
    }
    if (s[i] == '1') {
        return ParsedNum{ ok: true, val: 1.0, next_pos: i + 1 };
    }
    return bad;
}

// ---------------------------------------------------------------------------
// Public API: Parser
// ---------------------------------------------------------------------------

/// Parses an SVG 1.1/2.0 `<path d="...">` string into up to 64 commands with zero heap allocation.
pub fn parseSvgPath(d: string) -> ParsedPath {
    let res = ParsedPath.invalid();
    let i = skipWs(d, 0 as usize);
    if (i >= d.len) {
        return res;
    }

    let cur_cmd: u8 = 0;
    while (i < d.len) {
        i = skipWs(d, i);
        if (i >= d.len) {
            break;
        }

        const c = d[i];
        const maybe_argc = expectedArgCount(c);
        if (maybe_argc != null) {
            cur_cmd = c;
            i += 1;
        } else if (cur_cmd != 0 && (isNumberStart(c) || c == ',')) {
            // Implicit repeated command: M->L, m->l; Z/z cannot repeat with args
            if (cur_cmd == 'M') {
                cur_cmd = 'L';
            } else if (cur_cmd == 'm') {
                cur_cmd = 'l';
            } else if (cur_cmd == 'Z' || cur_cmd == 'z') {
                return ParsedPath.invalid();
            }
        } else {
            return ParsedPath.invalid();
        }

        const argc_opt = expectedArgCount(cur_cmd);
        if (argc_opt == null) {
            return ParsedPath.invalid();
        }
        const argc = argc_opt.?;

        if (res.count >= MAX_PATH_CMDS) {
            return ParsedPath.invalid();
        }

        let cmd = PathCmd.empty();
        cmd.code = cur_cmd;
        cmd.relative = isLowerCmd(cur_cmd);
        cmd.arg_count = argc;

        const is_arc = (cur_cmd == 'A' || cur_cmd == 'a');
        let a: u8 = 0;
        while (a < argc) {
            const parsed = if (is_arc && (a == 3 || a == 4))
                parseSvgFlag(d, i)
            else
                parseSvgNumber(d, i);
            if (!parsed.ok) {
                return ParsedPath.invalid();
            }
            cmd.args[a as usize] = parsed.val;
            i = parsed.next_pos;
            a += 1;
        }

        res.cmds[res.count] = cmd;
        res.count += 1;

        // If Z/z was parsed, consume optional trailing whitespace/comma check
        if (argc == 0) {
            i = skipWs(d, i);
        }
    }

    if (res.count > 0) {
        res.valid = true;
    }
    return res;
}

// ---------------------------------------------------------------------------
// Public API: Bounding Box Calculator
// ---------------------------------------------------------------------------

struct BoundsAcc {
    has_point: bool,
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,
}

fn includePoint(acc: BoundsAcc, x: f64, y: f64) -> BoundsAcc {
    if (!acc.has_point) {
        return BoundsAcc{
            has_point: true,
            min_x: x,
            min_y: y,
            max_x: x,
            max_y: y,
        };
    }
    return BoundsAcc{
        has_point: true,
        min_x: if (x < acc.min_x) x else acc.min_x,
        min_y: if (y < acc.min_y) y else acc.min_y,
        max_x: if (x > acc.max_x) x else acc.max_x,
        max_y: if (y > acc.max_y) y else acc.max_y,
    };
}

/// Computes the 2D bounding box of a parsed SVG path, resolving relative commands,
/// `H`/`V` axis moves, subpath origins for `Z`, and control/endpoint coordinates.
pub fn computeBounds(p: ParsedPath) -> PathBounds {
    if (!p.valid || p.count == 0) {
        return PathBounds.empty();
    }

    let acc = BoundsAcc{
        has_point: false,
        min_x: 0.0,
        min_y: 0.0,
        max_x: 0.0,
        max_y: 0.0,
    };
    let cx: f64 = 0.0;
    let cy: f64 = 0.0;
    let sx: f64 = 0.0;
    let sy: f64 = 0.0;

    let i: usize = 0;
    while (i < p.count) {
        const cmd = p.cmds[i];
        const rel = cmd.relative;
        const bx = if (rel) cx else 0.0;
        const by = if (rel) cy else 0.0;

        switch (cmd.code) {
            'M', 'm' => {
                cx = bx + cmd.args[0];
                cy = by + cmd.args[1];
                sx = cx;
                sy = cy;
                acc = includePoint(acc, cx, cy);
            },
            'L', 'l', 'T', 't' => {
                cx = bx + cmd.args[0];
                cy = by + cmd.args[1];
                acc = includePoint(acc, cx, cy);
            },
            'H', 'h' => {
                cx = bx + cmd.args[0];
                acc = includePoint(acc, cx, cy);
            },
            'V', 'v' => {
                cy = by + cmd.args[0];
                acc = includePoint(acc, cx, cy);
            },
            'C', 'c' => {
                acc = includePoint(acc, bx + cmd.args[0], by + cmd.args[1]);
                acc = includePoint(acc, bx + cmd.args[2], by + cmd.args[3]);
                cx = bx + cmd.args[4];
                cy = by + cmd.args[5];
                acc = includePoint(acc, cx, cy);
            },
            'S', 's', 'Q', 'q' => {
                acc = includePoint(acc, bx + cmd.args[0], by + cmd.args[1]);
                cx = bx + cmd.args[2];
                cy = by + cmd.args[3];
                acc = includePoint(acc, cx, cy);
            },
            'A', 'a' => {
                cx = bx + cmd.args[5];
                cy = by + cmd.args[6];
                acc = includePoint(acc, cx, cy);
            },
            'Z', 'z' => {
                cx = sx;
                cy = sy;
            },
            else => {},
        }
        i += 1;
    }

    if (!acc.has_point) {
        return PathBounds.empty();
    }

    return PathBounds{
        valid: true,
        min_x: acc.min_x,
        min_y: acc.min_y,
        max_x: acc.max_x,
        max_y: acc.max_y,
        width: acc.max_x - acc.min_x,
        height: acc.max_y - acc.min_y,
    };
}

// ---------------------------------------------------------------------------
// Public API: Canonical Formatter (Zero Heap Allocation)
// ---------------------------------------------------------------------------

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendFloatCompact(out: []u8, start_pos: usize, val: f64) -> usize {
    let pos = start_pos;
    const neg = val < 0.0;
    const abs_v = if (neg) -val else val;
    const scaled = (abs_v * 10000.0 + 0.5) as u64;
    if (neg && scaled > 0) {
        pos = appendByte(out, pos, '-');
    }
    const int_part = scaled / 10000;
    let frac_part = scaled % 10000;
    pos = writeU64(int_part, out, pos);
    if (frac_part > 0) {
        pos = appendByte(out, pos, '.');
        let fbuf: [4]u8 = [
            ('0' as u64 + ((frac_part / 1000) % 10)) as u8,
            ('0' as u64 + ((frac_part / 100) % 10)) as u8,
            ('0' as u64 + ((frac_part / 10) % 10)) as u8,
            ('0' as u64 + (frac_part % 10)) as u8,
        ];
        let flen: usize = 4;
        while (flen > 0 && fbuf[flen - 1] == '0') {
            flen -= 1;
        }
        pos = writeStr(fbuf[0..flen], out, pos);
    }
    return pos;
}

/// Formats a `ParsedPath` back into a clean canonical SVG `d` string in `out`.
pub fn formatSvgPath(p: ParsedPath, out: []u8) -> string {
    if (!p.valid || p.count == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < p.count) {
        if (i > 0) {
            pos = appendByte(out, pos, ' ');
        }
        const cmd = p.cmds[i];
        pos = appendByte(out, pos, cmd.code);
        let a: u8 = 0;
        while (a < cmd.arg_count) {
            pos = appendByte(out, pos, ' ');
            pos = appendFloatCompact(out, pos, cmd.args[a as usize]);
            a += 1;
        }
        i += 1;
    }
    return out[0..pos];
}
