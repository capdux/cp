// Source: cp/lib/syslog/formatter.cp (RFC 5424 & RFC 3164 Syslog Formatter)
// Pure CP zero-C-dependency serializer with automatic PRI calculation,
// NILVALUE '-' handling, and strict SD-PARAM value escaping.

const std = @import("std");
import {
    SyslogError,
    SdParam,
    SdElement,
    SyslogMessage,
} from "./types.cp";

fn appendByte(out: *std.ArrayList(u8), alloc: std.mem.Allocator, b: u8) ! SyslogError {
    out.append(alloc, b) catch return SyslogError.OutOfMemory;
}

fn appendSlice(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8) ! SyslogError {
    out.appendSlice(alloc, s) catch return SyslogError.OutOfMemory;
}

fn appendOptOrNil(out: *std.ArrayList(u8), alloc: std.mem.Allocator, opt: ([]const u8)?) ! SyslogError {
    if (val := opt) {
        if (val.len > 0) {
            appendSlice(out, alloc, val)?;
            return;
        }
    }
    appendByte(out, alloc, '-')?;
}

fn appendU8Decimal(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u8) ! SyslogError {
    let buf: [4]u8 = undefined;
    let s = std.fmt.bufPrint(&buf, "{d}", .{v}) catch return SyslogError.OutOfMemory;
    appendSlice(out, alloc, s)?;
}

fn appendEscapedSdValue(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: []const u8) ! SyslogError {
    let i: usize = 0;
    while (i < val.len) : (i += 1) {
        let ch = val[i];
        if (ch == '"' || ch == '\\' || ch == ']') {
            appendByte(out, alloc, '\\')?;
        }
        appendByte(out, alloc, ch)?;
    }
}

pub fn formatRfc5424(alloc: std.mem.Allocator, msg: SyslogMessage): ([]u8) ! SyslogError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let pri: u8 = msg.facility.toCode() * 8 + msg.severity.toCode();
    appendByte(&out, alloc, '<')?;
    appendU8Decimal(&out, alloc, pri)?;
    appendSlice(&out, alloc, ">1 ")?;

    appendOptOrNil(&out, alloc, msg.timestamp)?;
    appendByte(&out, alloc, ' ')?;
    appendOptOrNil(&out, alloc, msg.hostname)?;
    appendByte(&out, alloc, ' ')?;
    appendOptOrNil(&out, alloc, msg.app_name)?;
    appendByte(&out, alloc, ' ')?;
    appendOptOrNil(&out, alloc, msg.proc_id)?;
    appendByte(&out, alloc, ' ')?;
    appendOptOrNil(&out, alloc, msg.msg_id)?;
    appendByte(&out, alloc, ' ')?;

    if (msg.structured_data.len == 0) {
        appendByte(&out, alloc, '-')?;
    } else {
        let i: usize = 0;
        while (i < msg.structured_data.len) : (i += 1) {
            let elem = msg.structured_data[i];
            appendByte(&out, alloc, '[')?;
            appendSlice(&out, alloc, elem.id)?;
            let j: usize = 0;
            while (j < elem.params.len) : (j += 1) {
                let p = elem.params[j];
                appendByte(&out, alloc, ' ')?;
                appendSlice(&out, alloc, p.name)?;
                appendSlice(&out, alloc, "=\"")?;
                appendEscapedSdValue(&out, alloc, p.value)?;
                appendByte(&out, alloc, '"')?;
            }
            appendByte(&out, alloc, ']')?;
        }
    }

    if (msg.message.len > 0) {
        appendByte(&out, alloc, ' ')?;
        appendSlice(&out, alloc, msg.message)?;
    }

    return out.toOwnedSlice(alloc) catch return SyslogError.OutOfMemory;
}

pub fn formatRfc3164(alloc: std.mem.Allocator, msg: SyslogMessage): ([]u8) ! SyslogError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let pri: u8 = msg.facility.toCode() * 8 + msg.severity.toCode();
    appendByte(&out, alloc, '<')?;
    appendU8Decimal(&out, alloc, pri)?;
    appendByte(&out, alloc, '>')?;

    if (ts := msg.timestamp) {
        appendSlice(&out, alloc, ts)?;
    } else {
        appendSlice(&out, alloc, "Jan  1 00:00:00")?;
    }
    appendByte(&out, alloc, ' ')?;

    if (h := msg.hostname) {
        appendSlice(&out, alloc, h)?;
    } else {
        appendSlice(&out, alloc, "localhost")?;
    }
    appendByte(&out, alloc, ' ')?;

    if (app := msg.app_name) {
        appendSlice(&out, alloc, app)?;
        if (pid := msg.proc_id) {
            appendByte(&out, alloc, '[')?;
            appendSlice(&out, alloc, pid)?;
            appendByte(&out, alloc, ']')?;
        }
        appendSlice(&out, alloc, ": ")?;
    }

    appendSlice(&out, alloc, msg.message)?;
    return out.toOwnedSlice(alloc) catch return SyslogError.OutOfMemory;
}
