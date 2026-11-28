// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/common/FileChooser.ts, ConsoleMessage.ts, SecurityDetails.ts & TimeoutSettings.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/extras.cp — FileChooser, ConsoleMessage, SecurityDetails & TimeoutSettings
// ============================================================================

const std = @import("std");
import { CdpFrameScanner } from "cap:cdp";
import { PuppeteerError } from "./types";
import { ElementHandle } from "./handles";

pub struct FileChooser {
    element: ElementHandle = .{},
    multiple: bool = false,
    handled: bool = false,

    pub static fn init(element: ElementHandle, multiple: bool): FileChooser {
        return FileChooser{
            element: element,
            multiple: multiple,
            handled: false,
        };
    }

    pub fn isMultiple(): bool {
        return _.multiple;
    }

    pub fn isHandled(): bool {
        return _.handled;
    }

    pub fn accept(*_, paths: []const []const u8): void!PuppeteerError {
        if (_.handled) return PuppeteerError.InvalidArgument;
        _.handled = true;
        if (_.element.nodeId() <= 0 || _.element.isDisposed()) {
            return PuppeteerError.ElementNotFound;
        }
        let pbuf: [2048]u8 = undefined;
        let fbuf: [2560]u8 = undefined;
        let pos: usize = 0;
        const prefix = std.fmt.bufPrint(&pbuf, "{{\"nodeId\":{d},\"files\":[", .{_.element.nodeId()}) catch return PuppeteerError.BufferTooSmall;
        pos = prefix.len;
        let i: usize = 0;
        while (i < paths.len) {
            if (i > 0) {
                if (pos >= pbuf.len) return PuppeteerError.BufferTooSmall;
                pbuf[pos] = ',';
                pos += 1;
            }
            const item = std.fmt.bufPrint(pbuf[pos..], "\"{s}\"", .{paths[i]}) catch return PuppeteerError.BufferTooSmall;
            pos += item.len;
            i += 1;
        }
        const suffix = "]}";
        if (pos + suffix.len > pbuf.len) return PuppeteerError.BufferTooSmall;
        @memcpy(pbuf[pos .. pos + suffix.len], suffix);
        pos += suffix.len;
        const _ign = _.element.handle.session.send("DOM.setFileInputFiles", pbuf[0..pos], &fbuf)?;
        * = _ign;
    }

    pub fn cancel(*_): void!PuppeteerError {
        if (_.handled) return PuppeteerError.InvalidArgument;
        _.handled = true;
        let ebuf: [128]u8 = undefined;
        const _ign = _.element.evaluate("this.dispatchEvent(new Event('cancel', {bubbles: true}))", &ebuf)?;
        * = _ign;
    }
}

pub enum ConsoleMessageType {
    Log,
    Debug,
    Info,
    Error,
    Warn,
    Dir,
    Table,
    Trace,
    Assert,
    Verbose,
}

pub struct ConsoleMessageLocation {
    url: []const u8 = "",
    line_number: u32 = 0,
    column_number: u32 = 0,
    script_id: []const u8 = "",
}

pub struct ConsoleMessage {
    msg_type: ConsoleMessageType = ConsoleMessageType.Log,
    text_val: []const u8 = "",
    location_val: ConsoleMessageLocation = .{},
    args_count: usize = 0,

    pub static fn init(
        msg_type: ConsoleMessageType,
        text_val: []const u8,
        loc: ConsoleMessageLocation,
        args_count: usize,
    ): ConsoleMessage {
        return ConsoleMessage{
            msg_type: msg_type,
            text_val: text_val,
            location_val: loc,
            args_count: args_count,
        };
    }

    pub static fn fromCdpPayload(params_json: []const u8): ConsoleMessage {
        const t_str = CdpFrameScanner.extractTopLevelField(params_json, "type", true) orelse "log";
        let mtype = ConsoleMessageType.Log;
        if (std.mem.eql(u8, t_str, "debug")) {
            mtype = ConsoleMessageType.Debug;
        } else if (std.mem.eql(u8, t_str, "info")) {
            mtype = ConsoleMessageType.Info;
        } else if (std.mem.eql(u8, t_str, "error")) {
            mtype = ConsoleMessageType.Error;
        } else if (std.mem.eql(u8, t_str, "warning") || std.mem.eql(u8, t_str, "warn")) {
            mtype = ConsoleMessageType.Warn;
        } else if (std.mem.eql(u8, t_str, "trace")) {
            mtype = ConsoleMessageType.Trace;
        } else if (std.mem.eql(u8, t_str, "assert")) {
            mtype = ConsoleMessageType.Assert;
        }
        const txt = CdpFrameScanner.extractTopLevelField(params_json, "text", true) orelse "";
        const url = CdpFrameScanner.extractTopLevelField(params_json, "url", true) orelse "";
        return ConsoleMessage{
            msg_type: mtype,
            text_val: txt,
            location_val: ConsoleMessageLocation{ url: url },
            args_count: 0,
        };
    }

    pub fn kind(): ConsoleMessageType {
        return _.msg_type;
    }

    pub fn text(): []const u8 {
        return _.text_val;
    }

    pub fn location(): ConsoleMessageLocation {
        return _.location_val;
    }

    pub fn argsCount(): usize {
        return _.args_count;
    }
}

pub struct SecurityDetails {
    subject_name: []const u8 = "",
    issuer_val: []const u8 = "",
    valid_from: f64 = 0.0,
    valid_to: f64 = 0.0,
    protocol_val: []const u8 = "",
    san_list: [8][]const u8 = undefined,
    san_len: usize = 0,

    pub static fn init(
        subject_name: []const u8,
        issuer_val: []const u8,
        valid_from: f64,
        valid_to: f64,
        protocol_val: []const u8,
    ): SecurityDetails {
        return SecurityDetails{
            subject_name: subject_name,
            issuer_val: issuer_val,
            valid_from: valid_from,
            valid_to: valid_to,
            protocol_val: protocol_val,
            san_len: 0,
        };
    }

    pub static fn fromCdpPayload(security_json: []const u8): SecurityDetails {
        const subject = CdpFrameScanner.extractTopLevelField(security_json, "subjectName", true) orelse "";
        const iss = CdpFrameScanner.extractTopLevelField(security_json, "issuer", true) orelse "";
        const proto = CdpFrameScanner.extractTopLevelField(security_json, "protocol", true) orelse "";
        let vf: f64 = 0.0;
        let vt: f64 = 0.0;
        if (vf_str := CdpFrameScanner.extractTopLevelField(security_json, "validFrom", false)) {
            vf = std.fmt.parseFloat(f64, vf_str) catch 0.0;
        }
        if (vt_str := CdpFrameScanner.extractTopLevelField(security_json, "validTo", false)) {
            vt = std.fmt.parseFloat(f64, vt_str) catch 0.0;
        }
        return SecurityDetails{
            subject_name: subject,
            issuer_val: iss,
            valid_from: vf,
            valid_to: vt,
            protocol_val: proto,
            san_len: 0,
        };
    }

    pub fn issuer(): []const u8 {
        return _.issuer_val;
    }

    pub fn validFrom(): f64 {
        return _.valid_from;
    }

    pub fn validTo(): f64 {
        return _.valid_to;
    }

    pub fn protocol(): []const u8 {
        return _.protocol_val;
    }

    pub fn subjectName(): []const u8 {
        return _.subject_name;
    }

    pub fn subjectAlternativeNames(): []const []const u8 {
        return _.san_list[0.._.san_len];
    }
}

pub const DEFAULT_TIMEOUT_MS: u32 = 30000;

pub struct TimeoutSettings {
    default_timeout_ms: u32? = null,
    default_navigation_timeout_ms: u32? = null,

    pub static fn init(): TimeoutSettings {
        return TimeoutSettings{
            default_timeout_ms: null,
            default_navigation_timeout_ms: null,
        };
    }

    pub fn setDefaultTimeout(*_, timeout_ms: u32) {
        _.default_timeout_ms = timeout_ms;
    }

    pub fn setDefaultNavigationTimeout(*_, timeout_ms: u32) {
        _.default_navigation_timeout_ms = timeout_ms;
    }

    pub fn navigationTimeout(): u32 {
        if (nav := _.default_navigation_timeout_ms) {
            return nav;
        }
        if (def := _.default_timeout_ms) {
            return def;
        }
        return DEFAULT_TIMEOUT_MS;
    }

    pub fn timeout(): u32 {
        if (def := _.default_timeout_ms) {
            return def;
        }
        return DEFAULT_TIMEOUT_MS;
    }
}
