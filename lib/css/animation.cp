// Source: Absorbed from /data/dev/bun/src/css/properties/animation.cp, /data/dev/bun/src/css/properties/transition.cp
//! CSS Animations Level 1 & Transitions Level 1 shorthand parsers and serializers,
//! composing `./time.cp` and `./easing.cp`.

import std from "std";
import { CssTime } from "./time.cp";
import { EasingFunction } from "./easing.cp";

export enum AnimationDirection: u8 {
    normal = 0,
    reverse = 1,
    alternate = 2,
    alternate_reverse = 3,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .normal => "normal",
            .reverse => "reverse",
            .alternate => "alternate",
            .alternate_reverse => "alternate-reverse",
        };
    }

    pub static fn parse(input: []const u8): AnimationDirection? {
        if (asciiCaseEql(input, "normal")) return .normal;
        if (asciiCaseEql(input, "reverse")) return .reverse;
        if (asciiCaseEql(input, "alternate")) return .alternate;
        if (asciiCaseEql(input, "alternate-reverse")) return .alternate_reverse;
        return null;
    }
}

export enum AnimationPlayState: u8 {
    running = 0,
    paused = 1,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .running => "running",
            .paused => "paused",
        };
    }

    pub static fn parse(input: []const u8): AnimationPlayState? {
        if (asciiCaseEql(input, "running")) return .running;
        if (asciiCaseEql(input, "paused")) return .paused;
        return null;
    }
}

export enum AnimationFillMode: u8 {
    none = 0,
    forwards = 1,
    backwards = 2,
    both = 3,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .none => "none",
            .forwards => "forwards",
            .backwards => "backwards",
            .both => "both",
        };
    }

    pub static fn parse(input: []const u8): AnimationFillMode? {
        if (asciiCaseEql(input, "none")) return .none;
        if (asciiCaseEql(input, "forwards")) return .forwards;
        if (asciiCaseEql(input, "backwards")) return .backwards;
        if (asciiCaseEql(input, "both")) return .both;
        return null;
    }
}

export struct IterationCount {
    infinite: bool = false,
    count: f32 = 1.0,

    pub static fn once(): IterationCount {
        return IterationCount{ infinite: false, count: 1.0 };
    }

    pub static fn forever(): IterationCount {
        return IterationCount{ infinite: true, count: 0.0 };
    }

    pub static fn times(n: f32): IterationCount {
        return IterationCount{ infinite: false, count: n };
    }

    pub fn isDefault(): bool {
        return !_.infinite && _.count == 1.0;
    }

    pub static fn parse(input: []const u8): IterationCount? {
        if (asciiCaseEql(input, "infinite")) return IterationCount.forever();
        const v = std.fmt.parseFloat(f32, input) catch return null;
        if (std.math.isNan(v) || std.math.isInf(v) || v < 0.0) return null;
        return IterationCount.times(v);
    }

    pub fn toCss(out: []u8): []const u8 {
        if (_.infinite) return copyStr(out, "infinite");
        return std.fmt.bufPrint(out, "{d}", .{_.count}) catch out[0..0];
    }
}

/// Parsed CSS `animation` shorthand property.
export struct Animation {
    name: []const u8 = "none",
    duration: CssTime = CssTime.seconds(0.0),
    timing_function: EasingFunction = EasingFunction.ease(),
    delay: CssTime = CssTime.seconds(0.0),
    iteration_count: IterationCount = IterationCount.once(),
    direction: AnimationDirection = .normal,
    fill_mode: AnimationFillMode = .none,
    play_state: AnimationPlayState = .running,

    /// Parse a single CSS `animation` shorthand value (order-independent per CSS Animations 1).
    pub static fn parse(input: []const u8): Animation? {
        const s = trimAscii(input);
        if (s.len == 0) return null;

        let res = Animation{};
        let has_duration = false;
        let has_delay = false;
        let has_easing = false;
        let has_iter = false;
        let has_dir = false;
        let has_fill = false;
        let has_play = false;
        let has_name = false;

        let pos: usize = 0;
        let tokens: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const tok = nextToken(s, *pos);
            if (tok.len == 0) break;
            tokens += 1;

            if (!has_duration && CssTime.parse(tok) != null) {
                res.duration = CssTime.parse(tok).?;
                has_duration = true;
            } else if (!has_delay && CssTime.parse(tok) != null) {
                res.delay = CssTime.parse(tok).?;
                has_delay = true;
            } else if (!has_easing && (EasingFunction.parse(tok) catch null) != null) {
                res.timing_function = EasingFunction.parse(tok) catch unreachable;
                has_easing = true;
            } else if (!has_iter && IterationCount.parse(tok) != null) {
                res.iteration_count = IterationCount.parse(tok).?;
                has_iter = true;
            } else if (!has_dir && AnimationDirection.parse(tok) != null) {
                res.direction = AnimationDirection.parse(tok).?;
                has_dir = true;
            } else if (!has_fill && AnimationFillMode.parse(tok) != null) {
                res.fill_mode = AnimationFillMode.parse(tok).?;
                has_fill = true;
            } else if (!has_play && AnimationPlayState.parse(tok) != null) {
                res.play_state = AnimationPlayState.parse(tok).?;
                has_play = true;
            } else if (!has_name) {
                res.name = tok;
                has_name = true;
            } else {
                return null;
            }
        }
        if (tokens == 0) return null;
        return res;
    }

    pub fn toCss(out: []u8): []const u8 {
        let len: usize = 0;
        let tmp: [64]u8 = undefined;

        if (!_.duration.isZero() || !_.delay.isZero()) {
            len = appendPart(out, len, _.duration.toCss(tmp[0..]));
        }
        if (!_.timing_function.isEase()) {
            const es = _.timing_function.toCss(tmp[0..]) catch "";
            len = appendPart(out, len, es);
        }
        if (!_.delay.isZero()) {
            len = appendPart(out, len, _.delay.toCss(tmp[0..]));
        }
        if (!_.iteration_count.isDefault()) {
            len = appendPart(out, len, _.iteration_count.toCss(tmp[0..]));
        }
        if (_.direction != .normal) {
            len = appendPart(out, len, _.direction.toCss());
        }
        if (_.fill_mode != .none) {
            len = appendPart(out, len, _.fill_mode.toCss());
        }
        if (_.play_state != .running) {
            len = appendPart(out, len, _.play_state.toCss());
        }
        if (!asciiCaseEql(_.name, "none") || len == 0) {
            len = appendPart(out, len, _.name);
        }
        return out[0..len];
    }
}

/// Parsed CSS `transition` shorthand property.
export struct Transition {
    property: []const u8 = "all",
    duration: CssTime = CssTime.seconds(0.0),
    timing_function: EasingFunction = EasingFunction.ease(),
    delay: CssTime = CssTime.seconds(0.0),

    pub static fn parse(input: []const u8): Transition? {
        const s = trimAscii(input);
        if (s.len == 0) return null;

        let res = Transition{};
        let has_duration = false;
        let has_delay = false;
        let has_easing = false;
        let has_prop = false;

        let pos: usize = 0;
        let tokens: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const tok = nextToken(s, *pos);
            if (tok.len == 0) break;
            tokens += 1;

            if (!has_duration && CssTime.parse(tok) != null) {
                res.duration = CssTime.parse(tok).?;
                has_duration = true;
            } else if (!has_delay && CssTime.parse(tok) != null) {
                res.delay = CssTime.parse(tok).?;
                has_delay = true;
            } else if (!has_easing && (EasingFunction.parse(tok) catch null) != null) {
                res.timing_function = EasingFunction.parse(tok) catch unreachable;
                has_easing = true;
            } else if (!has_prop) {
                res.property = tok;
                has_prop = true;
            } else {
                return null;
            }
        }
        if (tokens == 0) return null;
        return res;
    }

    pub fn toCss(out: []u8): []const u8 {
        let len: usize = 0;
        let tmp: [64]u8 = undefined;
        len = appendPart(out, len, _.property);
        if (!_.duration.isZero() || !_.delay.isZero()) {
            len = appendPart(out, len, _.duration.toCss(tmp[0..]));
        }
        if (!_.timing_function.isEase()) {
            const es = _.timing_function.toCss(tmp[0..]) catch "";
            len = appendPart(out, len, es);
        }
        if (!_.delay.isZero()) {
            len = appendPart(out, len, _.delay.toCss(tmp[0..]));
        }
        return out[0..len];
    }
}

/// Extract next whitespace-delimited token, keeping parenthesized functions like `cubic-bezier(0.1, 0.2, 0.3, 1)` intact.
fn nextToken(s: []const u8, pos: *usize): []const u8 {
    const start = pos.*;
    let depth: i32 = 0;
    while (pos.* < s.len) : (pos.* += 1) {
        const c = s[pos.*];
        if (c == '(') {
            depth += 1;
        } else if (c == ')') {
            if (depth > 0) depth -= 1;
        } else if (depth == 0 && isWs(c)) {
            break;
        }
    }
    return s[start..pos.*];
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn trimAscii(s: []const u8): []const u8 {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && isWs(s[a])) : (a += 1) {}
    while (b > a && isWs(s[b - 1])) : (b -= 1) {}
    return s[a..b];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn asciiCaseEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn copyStr(out: []u8, src: []const u8): []const u8 {
    const n = if (src.len < out.len) src.len else out.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[i] = src[i];
    }
    return out[0..n];
}

fn appendPart(out: []u8, offset: usize, part: []const u8): usize {
    if (part.len == 0) return offset;
    let pos = offset;
    if (pos > 0 && pos < out.len) {
        out[pos] = ' ';
        pos += 1;
    }
    let i: usize = 0;
    while (i < part.len && pos < out.len) : (i += 1) {
        out[pos] = part[i];
        pos += 1;
    }
    return pos;
}
