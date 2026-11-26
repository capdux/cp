// @cap/pino-log — Zero-heap high-performance structured NDJSON logger modeled on npm `pino` v9.
// Clean-room MIT implementation; reuses `@cap/itoa` for zero-heap integer serialization.

import { formatU64, formatI64 } from "../../itoa/src/index.cp";

pub const PINO_TRACE: u8 = 10;
pub const PINO_DEBUG: u8 = 20;
pub const PINO_INFO: u8 = 30;
pub const PINO_WARN: u8 = 40;
pub const PINO_ERROR: u8 = 50;
pub const PINO_FATAL: u8 = 60;
pub const PINO_SILENT: u8 = 100;

pub const PINO_MAX_FIELDS: usize = 16;

pub const PINO_VAL_STR: u8 = 1;
pub const PINO_VAL_INT: u8 = 2;
pub const PINO_VAL_BOOL: u8 = 3;

pub struct PinoField {
    pub key: string,
    pub kind: u8,
    pub str_val: string,
    pub int_val: i64,
    pub bool_val: bool,
}

fn pinoPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn pinoPushJsonStr(out: []u8, pos: usize, s: string): usize {
    let p = pinoPush(out, pos, "\"");
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        let b = s[i];
        if (b == 34) { p = pinoPush(out, p, "\\\""); }
        else if (b == 92) { p = pinoPush(out, p, "\\\\"); }
        else if (b == 10) { p = pinoPush(out, p, "\\n"); }
        else if (b == 13) { p = pinoPush(out, p, "\\r"); }
        else if (b == 9) { p = pinoPush(out, p, "\\t"); }
        else if (b < 32) {
            let hex: string = "0123456789abcdef";
            p = pinoPush(out, p, "\\u00");
            if (p < out.len) { out[p] = hex[((b >> 4) & 15) as usize]; p = p + 1; }
            if (p < out.len) { out[p] = hex[(b & 15) as usize]; p = p + 1; }
        } else {
            out[p] = b;
            p = p + 1;
        }
        i = i + 1;
    }
    return pinoPush(out, p, "\"");
}

fn pinoPushU64(out: []u8, pos: usize, v: u64): usize {
    let tmp: [24]u8 = undefined;
    let n = formatU64(v, tmp[0..24]);
    let p = pos;
    let i: usize = 0;
    while (i < n && p < out.len) {
        out[p] = tmp[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn pinoPushI64(out: []u8, pos: usize, v: i64): usize {
    let tmp: [24]u8 = undefined;
    let n = formatI64(v, tmp[0..24]);
    let p = pos;
    let i: usize = 0;
    while (i < n && p < out.len) {
        out[p] = tmp[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

/// Converts a Pino numeric level (10..60) to its canonical lowercase label.
pub fn levelLabel(level: u8): string {
    if (level <= PINO_TRACE) { return "trace"; }
    if (level <= PINO_DEBUG) { return "debug"; }
    if (level <= PINO_INFO) { return "info"; }
    if (level <= PINO_WARN) { return "warn"; }
    if (level <= PINO_ERROR) { return "error"; }
    if (level <= PINO_FATAL) { return "fatal"; }
    return "silent";
}

/// Zero-heap Pino structured JSON logger with base metadata and child bindings.
pub struct PinoLogger {
    pub min_level: u8,
    pub name: string,
    pub pid: u32,
    pub hostname: string,
    pub bindings: [16]PinoField,
    pub binding_count: usize,

    pub static fn new(min_level: u8, name: string, pid: u32, hostname: string): PinoLogger {
        let empty = PinoField { key: "", kind: PINO_VAL_STR, str_val: "", int_val: 0, bool_val: false };
        let arr: [16]PinoField = undefined;
        let i: usize = 0;
        while (i < PINO_MAX_FIELDS) { arr[i] = empty; i = i + 1; }
        return PinoLogger {
            min_level: min_level,
            name: name,
            pid: pid,
            hostname: hostname,
            bindings: arr,
            binding_count: 0,
        };
    }

    pub fn isLevelEnabled(level: u8): bool {
        return level >= _.min_level && _.min_level < PINO_SILENT;
    }

    pub fn bindStr(*_, key: string, val: string): bool {
        if (_.binding_count >= PINO_MAX_FIELDS) { return false; }
        _.bindings[_.binding_count] = PinoField { key: key, kind: PINO_VAL_STR, str_val: val, int_val: 0, bool_val: false };
        _.binding_count = _.binding_count + 1;
        return true;
    }

    pub fn bindInt(*_, key: string, val: i64): bool {
        if (_.binding_count >= PINO_MAX_FIELDS) { return false; }
        _.bindings[_.binding_count] = PinoField { key: key, kind: PINO_VAL_INT, str_val: "", int_val: val, bool_val: false };
        _.binding_count = _.binding_count + 1;
        return true;
    }

    pub fn bindBool(*_, key: string, val: bool): bool {
        if (_.binding_count >= PINO_MAX_FIELDS) { return false; }
        _.bindings[_.binding_count] = PinoField { key: key, kind: PINO_VAL_BOOL, str_val: "", int_val: 0, bool_val: val };
        _.binding_count = _.binding_count + 1;
        return true;
    }

    /// Formats a single NDJSON log line (`{...}\n`) into `out`; returns 0 if filtered out by `min_level`.
    pub fn formatLine(level: u8, time_ms: u64, msg: string, out: []u8): usize {
        if (!_.isLevelEnabled(level)) { return 0; }
        let p: usize = 0;
        p = pinoPush(out, p, "{\"level\":");
        p = pinoPushU64(out, p, level as u64);
        p = pinoPush(out, p, ",\"time\":");
        p = pinoPushU64(out, p, time_ms);
        if (_.pid > 0) {
            p = pinoPush(out, p, ",\"pid\":");
            p = pinoPushU64(out, p, _.pid as u64);
        }
        if (_.hostname.len > 0) {
            p = pinoPush(out, p, ",\"hostname\":");
            p = pinoPushJsonStr(out, p, _.hostname);
        }
        if (_.name.len > 0) {
            p = pinoPush(out, p, ",\"name\":");
            p = pinoPushJsonStr(out, p, _.name);
        }
        let i: usize = 0;
        while (i < _.binding_count) {
            let f = _.bindings[i];
            p = pinoPush(out, p, ",");
            p = pinoPushJsonStr(out, p, f.key);
            p = pinoPush(out, p, ":");
            if (f.kind == PINO_VAL_STR) {
                p = pinoPushJsonStr(out, p, f.str_val);
            } else if (f.kind == PINO_VAL_INT) {
                p = pinoPushI64(out, p, f.int_val);
            } else {
                p = pinoPush(out, p, if (f.bool_val) { "true" } else { "false" });
            }
            i = i + 1;
        }
        p = pinoPush(out, p, ",\"msg\":");
        p = pinoPushJsonStr(out, p, msg);
        p = pinoPush(out, p, "}\n");
        return p;
    }
}
