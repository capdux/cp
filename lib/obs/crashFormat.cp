// Source: cp/lib/crash_handler/format.cp (Human-readable text & RFC 8259 JSON crash report formatting)

const std = @import("std");
import {
    SignalKind,
    CrashReason,
    StackFrame,
    Breadcrumb,
    MetadataEntry,
    CrashReport,
    signalName,
    signalDescription,
    reasonName,
} from "./crashTypes.cp";
import { FixedCrashWriter } from "./crashRing.cp";

fn appendHex(list: *std.ArrayList(u8), alloc: std.mem.Allocator, val: usize): ! {
    list.appendSlice(alloc, "0x")?;
    if (val == 0) {
        list.append(alloc, '0')?;
        return;
    }
    const hex_digits = "0123456789abcdef";
    let tmp: [32]u8 = undefined;
    let count: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[count] = hex_digits[v & 0xf];
        v = v >> 4;
        count += 1;
    }
    while (count > 0) {
        count -= 1;
        list.append(alloc, tmp[count])?;
    }
}

fn appendUint(list: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u64): ! {
    if (val == 0) {
        list.append(alloc, '0')?;
        return;
    }
    let tmp: [32]u8 = undefined;
    let count: usize = 0;
    let v = val;
    while (v > 0) {
        let d: u8 = @intCast(v % 10);
        tmp[count] = @as(u8, 48) + d;
        v = v / 10;
        count += 1;
    }
    while (count > 0) {
        count -= 1;
        list.append(alloc, tmp[count])?;
    }
}

fn appendInt(list: *std.ArrayList(u8), alloc: std.mem.Allocator, val: i64): ! {
    if (val < 0) {
        list.append(alloc, '-')?;
        let abs_val: u64 = @intCast(-val);
        appendUint(list, alloc, abs_val)?;
    } else {
        let pos_val: u64 = @intCast(val);
        appendUint(list, alloc, pos_val)?;
    }
}

fn appendJsonString(list: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): ! {
    const hex_digits = "0123456789abcdef";
    list.append(alloc, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        let c = s[i];
        switch (c) {
            '"' => list.appendSlice(alloc, "\\\"")?,
            '\\' => list.appendSlice(alloc, "\\\\")?,
            '\n' => list.appendSlice(alloc, "\\n")?,
            '\r' => list.appendSlice(alloc, "\\r")?,
            '\t' => list.appendSlice(alloc, "\\t")?,
            0x08 => list.appendSlice(alloc, "\\b")?,
            0x0c => list.appendSlice(alloc, "\\f")?,
            else => {
                if (c < 0x20) {
                    list.appendSlice(alloc, "\\u00")?;
                    list.append(alloc, hex_digits[(c >> 4) & 0xf])?;
                    list.append(alloc, hex_digits[c & 0xf])?;
                } else {
                    list.append(alloc, c)?;
                }
            },
        }
        i += 1;
    }
    list.append(alloc, '"')?;
}

fn appendReasonDetail(list: *std.ArrayList(u8), alloc: std.mem.Allocator, reason: CrashReason): ! {
    switch (reason) {
        .Panic msg => list.appendSlice(alloc, msg)?,
        .Unreachable => list.appendSlice(alloc, "reached unreachable code")?,
        .SegmentationFault addr => {
            list.appendSlice(alloc, "segmentation fault at ")?;
            appendHex(list, alloc, addr)?;
        },
        .IllegalInstruction addr => {
            list.appendSlice(alloc, "illegal instruction at ")?;
            appendHex(list, alloc, addr)?;
        },
        .BusError addr => {
            list.appendSlice(alloc, "bus error at ")?;
            appendHex(list, alloc, addr)?;
        },
        .FloatingPointError addr => {
            list.appendSlice(alloc, "floating point error at ")?;
            appendHex(list, alloc, addr)?;
        },
        .Trap addr => {
            list.appendSlice(alloc, "trap at ")?;
            appendHex(list, alloc, addr)?;
        },
        .Abort => list.appendSlice(alloc, "process aborted")?,
        .OutOfMemory => list.appendSlice(alloc, "out of memory")?,
        .AssertionFailed msg => list.appendSlice(alloc, msg)?,
        .ErrorCode code => {
            list.appendSlice(alloc, "error code ")?;
            appendInt(list, alloc, @intCast(code))?;
        },
    }
}

pub fn formatCrashTextFixed(comptime N: usize, report: *const CrashReport, writer: *FixedCrashWriter(N)) {
    writer.writeBytes("=== CRASH REPORT ===\n");
    writer.writeSignalSummary(report.signal, report.fault_addr);
    writer.writeBytes("Reason: ");
    writer.writeBytes(reasonName(report.reason));
    writer.writeBytes("\nThread: ");
    writer.writeUint(report.thread_id);
    writer.writeBytes("  Timestamp(ms): ");
    writer.writeUint(report.timestamp_ms);
    writer.writeByte('\n');

    if (report.metadata.len > 0) {
        writer.writeBytes("Metadata:\n");
        let mi: usize = 0;
        while (mi < report.metadata.len) {
            let m = report.metadata[mi];
            writer.writeBytes("  ");
            writer.writeRedactedEnv(m.key, m.value);
            mi += 1;
        }
    }

    if (report.frames.len > 0) {
        writer.writeBytes("Stack trace:\n");
        let fi: usize = 0;
        while (fi < report.frames.len) {
            writer.writeFrame(fi, report.frames[fi]);
            fi += 1;
        }
    }

    if (report.breadcrumbs.len > 0) {
        writer.writeBytes("Breadcrumbs:\n");
        let bi: usize = 0;
        while (bi < report.breadcrumbs.len) {
            let b = report.breadcrumbs[bi];
            writer.writeBytes("  [");
            writer.writeUint(b.timestamp_ms);
            writer.writeBytes("] ");
            writer.writeBytes(b.category);
            writer.writeBytes(": ");
            writer.writeBytes(b.message);
            writer.writeByte('\n');
            bi += 1;
        }
    }
}

pub fn formatCrashText(report: *const CrashReport, alloc: std.mem.Allocator): ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    out.appendSlice(alloc, "=== CRASH REPORT ===\n")?;
    out.appendSlice(alloc, signalName(report.signal))?;
    out.appendSlice(alloc, ": ")?;
    out.appendSlice(alloc, signalDescription(report.signal))?;
    if (report.fault_addr != 0) {
        out.appendSlice(alloc, " at ")?;
        appendHex(&out, alloc, report.fault_addr)?;
    }
    out.append(alloc, '\n')?;

    out.appendSlice(alloc, "Reason: ")?;
    out.appendSlice(alloc, reasonName(report.reason))?;
    out.appendSlice(alloc, " (")?;
    appendReasonDetail(&out, alloc, report.reason)?;
    out.appendSlice(alloc, ")\n")?;

    out.appendSlice(alloc, "Thread: ")?;
    appendUint(&out, alloc, report.thread_id)?;
    out.appendSlice(alloc, "  Timestamp(ms): ")?;
    appendUint(&out, alloc, report.timestamp_ms)?;
    out.append(alloc, '\n')?;

    if (report.metadata.len > 0) {
        out.appendSlice(alloc, "Metadata:\n")?;
        let mi: usize = 0;
        while (mi < report.metadata.len) {
            let m = report.metadata[mi];
            out.appendSlice(alloc, "  ")?;
            out.appendSlice(alloc, m.key)?;
            out.appendSlice(alloc, "=")?;
            out.appendSlice(alloc, m.value)?;
            out.append(alloc, '\n')?;
            mi += 1;
        }
    }

    if (report.frames.len > 0) {
        out.appendSlice(alloc, "Stack trace:\n")?;
        let fi: usize = 0;
        while (fi < report.frames.len) {
            let f = report.frames[fi];
            out.appendSlice(alloc, "  #")?;
            appendUint(&out, alloc, @intCast(fi))?;
            out.append(alloc, ' ')?;
            appendHex(&out, alloc, f.addr)?;
            if (f.symbol.len > 0) {
                out.appendSlice(alloc, " in ")?;
                out.appendSlice(alloc, f.symbol)?;
                if (f.offset > 0) {
                    out.append(alloc, '+')?;
                    appendHex(&out, alloc, f.offset)?;
                }
            }
            if (f.file.len > 0) {
                out.appendSlice(alloc, " (")?;
                out.appendSlice(alloc, f.file)?;
                if (f.line > 0) {
                    out.append(alloc, ':')?;
                    appendUint(&out, alloc, @intCast(f.line))?;
                    if (f.col > 0) {
                        out.append(alloc, ':')?;
                        appendUint(&out, alloc, @intCast(f.col))?;
                    }
                }
                out.append(alloc, ')')?;
            }
            out.append(alloc, '\n')?;
            fi += 1;
        }
    }

    if (report.breadcrumbs.len > 0) {
        out.appendSlice(alloc, "Breadcrumbs:\n")?;
        let bi: usize = 0;
        while (bi < report.breadcrumbs.len) {
            let b = report.breadcrumbs[bi];
            out.appendSlice(alloc, "  [")?;
            appendUint(&out, alloc, b.timestamp_ms)?;
            out.appendSlice(alloc, "] ")?;
            out.appendSlice(alloc, b.category)?;
            out.appendSlice(alloc, ": ")?;
            out.appendSlice(alloc, b.message)?;
            out.append(alloc, '\n')?;
            bi += 1;
        }
    }

    return out.toOwnedSlice(alloc);
}

pub fn formatCrashJson(report: *const CrashReport, alloc: std.mem.Allocator): ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    out.appendSlice(alloc, "{\"signal\":")?;
    appendJsonString(&out, alloc, signalName(report.signal))?;
    out.appendSlice(alloc, ",\"signal_description\":")?;
    appendJsonString(&out, alloc, signalDescription(report.signal))?;
    out.appendSlice(alloc, ",\"reason\":")?;
    appendJsonString(&out, alloc, reasonName(report.reason))?;
    out.appendSlice(alloc, ",\"reason_detail\":")?;
    let detail_buf = std.ArrayList(u8).empty;
    defer detail_buf.deinit(alloc);
    appendReasonDetail(&detail_buf, alloc, report.reason)?;
    appendJsonString(&out, alloc, detail_buf.items)?;

    out.appendSlice(alloc, ",\"fault_addr\":\"")?;
    appendHex(&out, alloc, report.fault_addr)?;
    out.appendSlice(alloc, "\",\"thread_id\":")?;
    appendUint(&out, alloc, report.thread_id)?;
    out.appendSlice(alloc, ",\"timestamp_ms\":")?;
    appendUint(&out, alloc, report.timestamp_ms)?;

    out.appendSlice(alloc, ",\"frames\":[")?;
    let fi: usize = 0;
    while (fi < report.frames.len) {
        if (fi > 0) {
            out.append(alloc, ',')?;
        }
        let f = report.frames[fi];
        out.appendSlice(alloc, "{\"addr\":\"")?;
        appendHex(&out, alloc, f.addr)?;
        out.appendSlice(alloc, "\",\"symbol\":")?;
        appendJsonString(&out, alloc, f.symbol)?;
        out.appendSlice(alloc, ",\"file\":")?;
        appendJsonString(&out, alloc, f.file)?;
        out.appendSlice(alloc, ",\"line\":")?;
        appendUint(&out, alloc, @intCast(f.line))?;
        out.appendSlice(alloc, ",\"col\":")?;
        appendUint(&out, alloc, @intCast(f.col))?;
        out.appendSlice(alloc, ",\"offset\":")?;
        appendUint(&out, alloc, @intCast(f.offset))?;
        out.append(alloc, '}')?;
        fi += 1;
    }

    out.appendSlice(alloc, "],\"breadcrumbs\":[")?;
    let bi: usize = 0;
    while (bi < report.breadcrumbs.len) {
        if (bi > 0) {
            out.append(alloc, ',')?;
        }
        let b = report.breadcrumbs[bi];
        out.appendSlice(alloc, "{\"timestamp_ms\":")?;
        appendUint(&out, alloc, b.timestamp_ms)?;
        out.appendSlice(alloc, ",\"level\":")?;
        appendUint(&out, alloc, @intCast(b.level))?;
        out.appendSlice(alloc, ",\"category\":")?;
        appendJsonString(&out, alloc, b.category)?;
        out.appendSlice(alloc, ",\"message\":")?;
        appendJsonString(&out, alloc, b.message)?;
        out.append(alloc, '}')?;
        bi += 1;
    }

    out.appendSlice(alloc, "],\"metadata\":[")?;
    let mi: usize = 0;
    while (mi < report.metadata.len) {
        if (mi > 0) {
            out.append(alloc, ',')?;
        }
        let m = report.metadata[mi];
        out.appendSlice(alloc, "{\"key\":")?;
        appendJsonString(&out, alloc, m.key)?;
        out.appendSlice(alloc, ",\"value\":")?;
        appendJsonString(&out, alloc, m.value)?;
        out.append(alloc, '}')?;
        mi += 1;
    }

    out.appendSlice(alloc, "]}")?;
    return out.toOwnedSlice(alloc);
}
