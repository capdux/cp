// Source: cp/lib/syslog/parser.cp (RFC 5424 & RFC 3164 Syslog Parser)
// Pure CP zero-C-dependency parser with automatic RFC 5424 / RFC 3164 detection,
// strict PRI/version/timestamp validation, SD-ELEMENT unescaping, and UTF-8 BOM handling.

const std = @import("std");
import {
    SyslogError,
    Facility,
    Severity,
    SdParam,
    SdElement,
    SyslogMessage,
} from "./types.cp";

type SyslogVec16 = @Vector(16, u8);

fn makeSyslogCharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let flags: u8 = 0;
        if (i >= '0' && i <= '9') flags |= 1;
        if (i >= 33 && i <= 126) {
            flags |= 2;
            if (i != '=' && i != ']' && i != '"') flags |= 4;
        }
        lut[i] = flags;
    }
    return lut;
}

const SYSLOG_CHAR_LUT: [u8; 256] = makeSyslogCharLut();

fn isDigit(c: u8): bool {
    return (SYSLOG_CHAR_LUT[usize(c)] & 1) != 0;
}

fn isPrintableAscii(c: u8): bool {
    return (SYSLOG_CHAR_LUT[usize(c)] & 2) != 0;
}

fn isSdNameChar(c: u8): bool {
    return (SYSLOG_CHAR_LUT[usize(c)] & 4) != 0;
}

fn scanSdName(raw: []const u8, start: usize): usize {
    let p = start;
    const v33: SyslogVec16 = @splat(33 as u8);
    const v126: SyslogVec16 = @splat(126 as u8);
    const veq: SyslogVec16 = @splat('=' as u8);
    const vrb: SyslogVec16 = @splat(']' as u8);
    const vqt: SyslogVec16 = @splat('"' as u8);
    while (p + 16 <= raw.len) : (p += 16) {
        const v: SyslogVec16 = raw[p .. p + 16][0..16].*;
        const stop = (v < v33) | (v > v126) | (v == veq) | (v == vrb) | (v == vqt);
        if (@reduce(.Or, stop)) break;
    }
    while (p < raw.len && isSdNameChar(raw[p])) : (p += 1) {}
    return p;
}

fn dupSlice(alloc: std.mem.Allocator, s: []const u8): ([]const u8) ! SyslogError {
    if (s.len == 0) return "";
    return alloc.dupe(u8, s) catch return SyslogError.OutOfMemory;
}

fn dupOptSlice(alloc: std.mem.Allocator, s: []const u8): (([]const u8)?) ! SyslogError {
    if (std.mem.eql(u8, s, "-")) return null;
    return alloc.dupe(u8, s) catch return SyslogError.OutOfMemory;
}

fn parseTwoDigits(a: u8, b: u8): u8? {
    if (!isDigit(a) || !isDigit(b)) return null;
    return (a - '0') * 10 + (b - '0');
}

fn parsePri(raw: []const u8, out_pos: *usize): u8 ! SyslogError {
    if (raw.len < 3 || raw[0] != '<') {
        return SyslogError.InvalidPriority;
    }
    let i: usize = 1;
    let val: u16 = 0;
    let digits: usize = 0;
    while (i < raw.len && raw[i] != '>') : (i += 1) {
        if (!isDigit(raw[i])) {
            return SyslogError.InvalidPriority;
        }
        digits += 1;
        if (digits > 3) {
            return SyslogError.InvalidPriority;
        }
        val = val * 10 + ((raw[i] - '0') as u16);
    }
    if (digits == 0 || i >= raw.len || raw[i] != '>') {
        return SyslogError.InvalidPriority;
    }
    if (val > 191) {
        return SyslogError.InvalidPriority;
    }
    out_pos.* = i + 1;
    return val as u8;
}

fn readHeaderToken(raw: []const u8, pos: *usize): ([]const u8) ! SyslogError {
    if (pos.* >= raw.len) return SyslogError.MalformedHeader;
    let start = pos.*;
    let p = start;
    const v33: SyslogVec16 = @splat(33 as u8);
    const v126: SyslogVec16 = @splat(126 as u8);
    while (p + 16 <= raw.len) : (p += 16) {
        const v: SyslogVec16 = raw[p .. p + 16][0..16].*;
        if (@reduce(.Or, (v < v33) | (v > v126))) break;
    }
    pos.* = p;
    while (pos.* < raw.len && raw[pos.*] != ' ') : (pos.* += 1) {
        if (!isPrintableAscii(raw[pos.*])) {
            return SyslogError.MalformedHeader;
        }
    }
    if (pos.* == start) return SyslogError.MalformedHeader;
    return raw[start..pos.*];
}

fn isValidRfc5424Timestamp(ts: []const u8): bool {
    if (ts.len < 20) return false;
    // YYYY-MM-DDTHH:MM:SS
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        if (!isDigit(ts[i])) return false;
    }
    if (ts[4] != '-' || ts[7] != '-') return false;
    if (ts[10] != 'T' && ts[10] != 't') return false;
    if (ts[13] != ':' || ts[16] != ':') return false;

    let mon = parseTwoDigits(ts[5], ts[6]) orelse return false;
    let day = parseTwoDigits(ts[8], ts[9]) orelse return false;
    let hour = parseTwoDigits(ts[11], ts[12]) orelse return false;
    let min = parseTwoDigits(ts[14], ts[15]) orelse return false;
    let sec = parseTwoDigits(ts[17], ts[18]) orelse return false;

    if (mon < 1 || mon > 12) return false;
    if (day < 1 || day > 31) return false;
    if (hour > 23 || min > 59 || sec > 60) return false;

    let pos: usize = 19;
    if (pos < ts.len && ts[pos] == '.') {
        pos += 1;
        let frac_digits: usize = 0;
        while (pos < ts.len && isDigit(ts[pos])) : (pos += 1) {
            frac_digits += 1;
        }
        if (frac_digits == 0 || frac_digits > 6) return false;
    }

    if (pos >= ts.len) return false;
    if (ts[pos] == 'Z' || ts[pos] == 'z') {
        return pos + 1 == ts.len;
    }
    if (ts[pos] == '+' || ts[pos] == '-') {
        if (pos + 6 != ts.len) return false;
        let off_h = parseTwoDigits(ts[pos + 1], ts[pos + 2]) orelse return false;
        if (ts[pos + 3] != ':') return false;
        let off_m = parseTwoDigits(ts[pos + 4], ts[pos + 5]) orelse return false;
        return off_h <= 23 && off_m <= 59;
    }
    return false;
}

fn isValidMonthAbbr(m: []const u8): bool {
    if (m.len != 3) return false;
    const months: [12][]const u8 = [
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    ];
    let i: usize = 0;
    while (i < months.len) : (i += 1) {
        if (std.mem.eql(u8, m, months[i])) return true;
    }
    return false;
}

fn isValidRfc3164Timestamp(ts: []const u8): bool {
    if (ts.len != 15) return false;
    if (!isValidMonthAbbr(ts[0..3])) return false;
    if (ts[3] != ' ') return false;
    if (ts[4] == ' ') {
        if (!isDigit(ts[5]) || ts[5] == '0') return false;
    } else {
        let day = parseTwoDigits(ts[4], ts[5]) orelse return false;
        if (day < 1 || day > 31) return false;
    }
    if (ts[6] != ' ') return false;
    let hh = parseTwoDigits(ts[7], ts[8]) orelse return false;
    if (ts[9] != ':') return false;
    let mm = parseTwoDigits(ts[10], ts[11]) orelse return false;
    if (ts[12] != ':') return false;
    let ss = parseTwoDigits(ts[13], ts[14]) orelse return false;
    return hh <= 23 && mm <= 59 && ss <= 60;
}

fn freeSdParams(alloc: std.mem.Allocator, list: *std.ArrayList(SdParam)) {
    let i: usize = 0;
    while (i < list.items.len) : (i += 1) {
        list.items[i].deinit(alloc);
    }
    list.deinit(alloc);
}

fn freeSdElements(alloc: std.mem.Allocator, list: *std.ArrayList(SdElement)) {
    let i: usize = 0;
    while (i < list.items.len) : (i += 1) {
        list.items[i].deinit(alloc);
    }
    list.deinit(alloc);
}

fn parseSdElement(alloc: std.mem.Allocator, raw: []const u8, pos: *usize): SdElement ! SyslogError {
    if (pos.* >= raw.len || raw[pos.*] != '[') {
        return SyslogError.MalformedStructuredData;
    }
    pos.* += 1;

    let id_start = pos.*;
    pos.* = scanSdName(raw, pos.*);
    if (pos.* == id_start) {
        return SyslogError.MalformedStructuredData;
    }
    let id_copy = dupSlice(alloc, raw[id_start..pos.*])?;
    errdefer alloc.free(id_copy);

    let params: std.ArrayList(SdParam) = .empty;
    errdefer freeSdParams(alloc, &params);

    const vqt: SyslogVec16 = @splat('"' as u8);
    const vbs: SyslogVec16 = @splat('\\' as u8);
    const vrb: SyslogVec16 = @splat(']' as u8);

    while (true) {
        if (pos.* >= raw.len) {
            return SyslogError.MalformedStructuredData;
        }
        if (raw[pos.*] == ']') {
            pos.* += 1;
            break;
        }
        if (raw[pos.*] != ' ') {
            return SyslogError.MalformedStructuredData;
        }
        pos.* += 1;

        let name_start = pos.*;
        pos.* = scanSdName(raw, pos.*);
        if (pos.* == name_start) {
            return SyslogError.MalformedStructuredData;
        }
        if (pos.* + 1 >= raw.len || raw[pos.*] != '=' || raw[pos.* + 1] != '"') {
            return SyslogError.MalformedStructuredData;
        }
        let pname_slice = raw[name_start..pos.*];
        pos.* += 2; // skip ="

        let val_buf: std.ArrayList(u8) = .empty;
        errdefer val_buf.deinit(alloc);

        let closed = false;
        while (pos.* < raw.len) {
            const chunk_start = pos.*;
            let scan = chunk_start;
            while (scan + 16 <= raw.len) : (scan += 16) {
                const v: SyslogVec16 = raw[scan .. scan + 16][0..16].*;
                if (@reduce(.Or, (v == vqt) | (v == vbs) | (v == vrb))) break;
            }
            if (scan > chunk_start) {
                val_buf.appendSlice(alloc, raw[chunk_start..scan]) catch return SyslogError.OutOfMemory;
                pos.* = scan;
                if (pos.* >= raw.len) break;
            }
            let ch = raw[pos.*];
            if (ch == '"') {
                pos.* += 1;
                closed = true;
                break;
            } else if (ch == '\\') {
                pos.* += 1;
                if (pos.* >= raw.len) {
                    return SyslogError.MalformedStructuredData;
                }
                let esc = raw[pos.*];
                if (esc == '"' || esc == '\\' || esc == ']') {
                    val_buf.append(alloc, esc) catch return SyslogError.OutOfMemory;
                } else {
                    val_buf.append(alloc, '\\') catch return SyslogError.OutOfMemory;
                    val_buf.append(alloc, esc) catch return SyslogError.OutOfMemory;
                }
                pos.* += 1;
            } else if (ch == ']') {
                return SyslogError.MalformedStructuredData;
            } else {
                val_buf.append(alloc, ch) catch return SyslogError.OutOfMemory;
                pos.* += 1;
            }
        }
        if (!closed) {
            return SyslogError.MalformedStructuredData;
        }

        let pname = dupSlice(alloc, pname_slice)?;
        errdefer alloc.free(pname);
        let pval = val_buf.toOwnedSlice(alloc) catch return SyslogError.OutOfMemory;
        errdefer alloc.free(pval);

        params.append(alloc, SdParam.init(pname, pval)) catch return SyslogError.OutOfMemory;
    }

    let owned_params = params.toOwnedSlice(alloc) catch return SyslogError.OutOfMemory;
    return SdElement.init(id_copy, owned_params);
}

pub fn parseRfc5424(alloc: std.mem.Allocator, raw: []const u8): SyslogMessage ! SyslogError {
    let pos: usize = 0;
    let pri = parsePri(raw, &pos)?;

    // VERSION
    let ver_start = pos;
    while (pos < raw.len && isDigit(raw[pos])) : (pos += 1) {}
    if (pos == ver_start || pos - ver_start > 3) {
        return SyslogError.InvalidVersion;
    }
    let ver: u16 = 0;
    let vi = ver_start;
    while (vi < pos) : (vi += 1) {
        ver = ver * 10 + ((raw[vi] - '0') as u16);
    }
    if (ver != 1) {
        return SyslogError.InvalidVersion;
    }
    if (pos >= raw.len || raw[pos] != ' ') {
        return SyslogError.MalformedHeader;
    }
    pos += 1;

    // TIMESTAMP
    let ts_tok = readHeaderToken(raw, &pos)?;
    if (!std.mem.eql(u8, ts_tok, "-") && !isValidRfc5424Timestamp(ts_tok)) {
        return SyslogError.InvalidTimestamp;
    }
    if (pos >= raw.len || raw[pos] != ' ') return SyslogError.MalformedHeader;
    pos += 1;

    // HOSTNAME
    let host_tok = readHeaderToken(raw, &pos)?;
    if (pos >= raw.len || raw[pos] != ' ') return SyslogError.MalformedHeader;
    pos += 1;

    // APP-NAME
    let app_tok = readHeaderToken(raw, &pos)?;
    if (pos >= raw.len || raw[pos] != ' ') return SyslogError.MalformedHeader;
    pos += 1;

    // PROCID
    let proc_tok = readHeaderToken(raw, &pos)?;
    if (pos >= raw.len || raw[pos] != ' ') return SyslogError.MalformedHeader;
    pos += 1;

    // MSGID
    let msgid_tok = readHeaderToken(raw, &pos)?;
    if (pos >= raw.len || raw[pos] != ' ') return SyslogError.MalformedHeader;
    pos += 1;

    // Allocate header fields with errdefer protection
    let timestamp = dupOptSlice(alloc, ts_tok)?;
    errdefer if (ts := timestamp) alloc.free(ts);

    let hostname = dupOptSlice(alloc, host_tok)?;
    errdefer if (h := hostname) alloc.free(h);

    let app_name = dupOptSlice(alloc, app_tok)?;
    errdefer if (a := app_name) alloc.free(a);

    let proc_id = dupOptSlice(alloc, proc_tok)?;
    errdefer if (p := proc_id) alloc.free(p);

    let msg_id = dupOptSlice(alloc, msgid_tok)?;
    errdefer if (m := msg_id) alloc.free(m);

    // STRUCTURED-DATA
    let sd_list: std.ArrayList(SdElement) = .empty;
    errdefer freeSdElements(alloc, &sd_list);

    if (pos >= raw.len) {
        return SyslogError.MalformedStructuredData;
    }
    if (raw[pos] == '-') {
        pos += 1;
        if (pos < raw.len && raw[pos] != ' ') {
            return SyslogError.MalformedStructuredData;
        }
    } else if (raw[pos] == '[') {
        while (pos < raw.len && raw[pos] == '[') {
            let elem = parseSdElement(alloc, raw, &pos)?;
            sd_list.append(alloc, elem) catch {
                let mut_elem = elem;
                mut_elem.deinit(alloc);
                return SyslogError.OutOfMemory;
            };
        }
        if (pos < raw.len && raw[pos] != ' ') {
            return SyslogError.MalformedStructuredData;
        }
    } else {
        return SyslogError.MalformedStructuredData;
    }

    // Optional MSG
    let message: []const u8 = "";
    if (pos < raw.len) {
        if (raw[pos] != ' ') {
            return SyslogError.MalformedHeader;
        }
        pos += 1;
        let msg_slice = raw[pos..];
        if (msg_slice.len >= 3 && msg_slice[0] == 0xEF && msg_slice[1] == 0xBB && msg_slice[2] == 0xBF) {
            msg_slice = msg_slice[3..];
        }
        message = dupSlice(alloc, msg_slice)?;
    }
    errdefer if (message.len > 0) alloc.free(message);

    let owned_sd = sd_list.toOwnedSlice(alloc) catch return SyslogError.OutOfMemory;

    return SyslogMessage{
        facility: Facility.fromCode(pri / 8).?,
        severity: Severity.fromCode(pri % 8).?,
        priority: pri,
        version: 1,
        timestamp: timestamp,
        hostname: hostname,
        app_name: app_name,
        proc_id: proc_id,
        msg_id: msg_id,
        structured_data: owned_sd,
        message: message,
        is_rfc5424: true,
        allocator: alloc,
    };
}

pub fn parseRfc3164(alloc: std.mem.Allocator, raw: []const u8): SyslogMessage ! SyslogError {
    let pos: usize = 0;
    let pri = parsePri(raw, &pos)?;

    // TIMESTAMP: "Mmm dd hh:mm:ss" (15 bytes)
    if (pos + 15 > raw.len) {
        return SyslogError.InvalidTimestamp;
    }
    let ts_slice = raw[pos..pos + 15];
    if (!isValidRfc3164Timestamp(ts_slice)) {
        return SyslogError.InvalidTimestamp;
    }
    pos += 15;
    if (pos >= raw.len || raw[pos] != ' ') {
        return SyslogError.MalformedHeader;
    }
    pos += 1;

    // HOSTNAME
    let host_start = pos;
    const vsp: SyslogVec16 = @splat(' ' as u8);
    while (pos + 16 <= raw.len) : (pos += 16) {
        const v: SyslogVec16 = raw[pos .. pos + 16][0..16].*;
        if (@reduce(.Or, v == vsp)) break;
    }
    while (pos < raw.len && raw[pos] != ' ') : (pos += 1) {}
    if (pos == host_start) {
        return SyslogError.MalformedHeader;
    }
    let host_slice = raw[host_start..pos];
    if (pos < raw.len && raw[pos] == ' ') {
        pos += 1;
    }

    // TAG[PID]: MSG
    let rest = raw[pos..];
    let app_slice: ([]const u8)? = null;
    let proc_slice: ([]const u8)? = null;
    let msg_slice: []const u8 = rest;

    // Find ':' before any space in tag
    let colon_idx: usize? = null;
    let k: usize = 0;
    const vcol: SyslogVec16 = @splat(':' as u8);
    while (k + 16 <= rest.len) : (k += 16) {
        const v: SyslogVec16 = rest[k .. k + 16][0..16].*;
        if (@reduce(.Or, (v == vcol) | (v == vsp))) break;
    }
    while (k < rest.len) : (k += 1) {
        if (rest[k] == ':') {
            colon_idx = k;
            break;
        }
        if (rest[k] == ' ') {
            break;
        }
    }

    if (ci := colon_idx) {
        let tag_part = rest[0..ci];
        let lbracket: usize? = null;
        let b: usize = 0;
        while (b < tag_part.len) : (b += 1) {
            if (tag_part[b] == '[') {
                lbracket = b;
                break;
            }
        }
        if (lb := lbracket) {
            if (tag_part.len > lb + 1 && tag_part[tag_part.len - 1] == ']') {
                app_slice = tag_part[0..lb];
                proc_slice = tag_part[lb + 1 .. tag_part.len - 1];
            } else {
                app_slice = tag_part;
            }
        } else {
            app_slice = tag_part;
        }
        let msg_start = ci + 1;
        if (msg_start < rest.len && rest[msg_start] == ' ') {
            msg_start += 1;
        }
        msg_slice = rest[msg_start..];
    }

    let timestamp: ([]const u8)? = dupSlice(alloc, ts_slice)?;
    errdefer if (ts := timestamp) alloc.free(ts);

    let hostname: ([]const u8)? = dupSlice(alloc, host_slice)?;
    errdefer if (h := hostname) alloc.free(h);

    let app_name: ([]const u8)? = null;
    if (a := app_slice) {
        app_name = dupSlice(alloc, a)?;
    }
    errdefer if (a := app_name) alloc.free(a);

    let proc_id: ([]const u8)? = null;
    if (p := proc_slice) {
        proc_id = dupSlice(alloc, p)?;
    }
    errdefer if (p := proc_id) alloc.free(p);

    let message = dupSlice(alloc, msg_slice)?;

    return SyslogMessage{
        facility: Facility.fromCode(pri / 8).?,
        severity: Severity.fromCode(pri % 8).?,
        priority: pri,
        version: 0,
        timestamp: timestamp,
        hostname: hostname,
        app_name: app_name,
        proc_id: proc_id,
        msg_id: null,
        structured_data: &[],
        message: message,
        is_rfc5424: false,
        allocator: alloc,
    };
}

pub fn parse(alloc: std.mem.Allocator, raw: []const u8): SyslogMessage ! SyslogError {
    let pos: usize = 0;
    * = parsePri(raw, &pos)?;
    if (pos < raw.len && isDigit(raw[pos])) {
        return parseRfc5424(alloc, raw);
    }
    return parseRfc3164(alloc, raw);
}
