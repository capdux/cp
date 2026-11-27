// @cap/env-filter — Zero-allocation RUST_LOG-style hierarchical module filter
// Clean-room parity with crates.io:env_filter (MIT OR Apache-2.0)

import { trim, eqIgnoreCase, strEq, startsWith, findByte } from "cap:ascii";

pub enum LogLevel: u8 {
    Off = 0,
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4,
    Trace = 5,
}

pub struct FilterDirective {
    target: string,
    level: LogLevel,
}

fn efltLevelRank(lvl: LogLevel): u8 {
    switch (lvl) {
        LogLevel.Off => return 0,
        LogLevel.Error => return 1,
        LogLevel.Warn => return 2,
        LogLevel.Info => return 3,
        LogLevel.Debug => return 4,
        LogLevel.Trace => return 5,
    }
}

pub fn parseLogLevel(s: string): LogLevel? {
    const trimmed = trim(s);
    if (eqIgnoreCase(trimmed, "off")) {
        return LogLevel.Off;
    }
    if (eqIgnoreCase(trimmed, "error")) {
        return LogLevel.Error;
    }
    if (eqIgnoreCase(trimmed, "warn")) {
        return LogLevel.Warn;
    }
    if (eqIgnoreCase(trimmed, "info")) {
        return LogLevel.Info;
    }
    if (eqIgnoreCase(trimmed, "debug")) {
        return LogLevel.Debug;
    }
    if (eqIgnoreCase(trimmed, "trace")) {
        return LogLevel.Trace;
    }
    return null;
}

fn efltMatchesTarget(target: string, directive_target: string): bool {
    if (directive_target.len == 0) {
        return true;
    }
    if (!startsWith(target, directive_target)) {
        return false;
    }
    if (target.len == directive_target.len) {
        return true;
    }
    if (target.len >= directive_target.len + 2) {
        if (target[directive_target.len] == ':' && target[directive_target.len + 1] == ':') {
            return true;
        }
    }
    return false;
}

pub struct EnvFilter {
    directives: [32]FilterDirective,
    count: usize,

    static fn empty(): EnvFilter {
        let items: [32]FilterDirective = undefined;
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            items[i] = FilterDirective{ target: "", level: LogLevel.Off };
        }
        return EnvFilter{
            directives: items,
            count: 0,
        };
    }

    fn sortDesc(*_) {
        let i: usize = 1;
        while (i < _.count) : (i += 1) {
            const key = _.directives[i];
            let j: usize = i;
            while (j > 0 && _.directives[j - 1].target.len < key.target.len) {
                _.directives[j] = _.directives[j - 1];
                j -= 1;
            }
            _.directives[j] = key;
        }
    }

    fn addDirective(*_, target: string, level: LogLevel): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (strEq(_.directives[i].target, target)) {
                _.directives[i].level = level;
                return true;
            }
        }
        if (_.count >= 32) {
            return false;
        }
        _.directives[_.count] = FilterDirective{
            target: target,
            level: level,
        };
        _.count += 1;
        _.sortDesc();
        return true;
    }

    static fn parse(spec: string): EnvFilter {
        let f = EnvFilter.empty();
        let pos: usize = 0;
        while (pos <= spec.len) {
            let end: usize = pos;
            while (end < spec.len && spec[end] != ',') {
                end += 1;
            }
            const part = trim(spec[pos..end]);
            if (part.len > 0) {
                if (eq_idx := findByte(part, '=', 0)) {
                    const lhs = trim(part[0..eq_idx]);
                    const rhs = trim(part[(eq_idx + 1)..part.len]);
                    if (lvl := parseLogLevel(rhs)) {
                        * = f.addDirective(lhs, lvl);
                    }
                } else {
                    if (lvl := parseLogLevel(part)) {
                        * = f.addDirective("", lvl);
                    } else {
                        * = f.addDirective(part, LogLevel.Trace);
                    }
                }
            }
            if (end >= spec.len) {
                break;
            }
            pos = end + 1;
        }
        return f;
    }

    fn directiveCount(): usize {
        return _.count;
    }

    fn getDirective(idx: usize): FilterDirective? {
        if (idx >= _.count) {
            return null;
        }
        return _.directives[idx];
    }

    fn maxLevelFor(target: string): LogLevel {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (efltMatchesTarget(target, _.directives[i].target)) {
                return _.directives[i].level;
            }
        }
        return LogLevel.Off;
    }

    fn enabled(target: string, level: LogLevel): bool {
        const req = efltLevelRank(level);
        if (req == 0) {
            return false;
        }
        const max_lvl = _.maxLevelFor(target);
        return req <= efltLevelRank(max_lvl);
    }
}
