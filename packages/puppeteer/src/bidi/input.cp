// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/bidi/Input.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/bidi/input.cp — BidiKeyboard, BidiMouse & BidiInput
// Implements input.performActions, input.releaseActions, input.setFiles and
// W3C WebDriver Unicode PUA key value mapping.
// ============================================================================

const std = @import("std");
import {
    BidiError,
    BidiMouseButton,
    bidiMouseButtonCode,
    bidiWriteStr,
    bidiWriteJsonEscaped,
} from "./protocol";
import { BidiSession } from "./session";

/// Map Puppeteer / W3C KeyInput names to WebDriver BiDi JSON-escaped Unicode key values.
pub fn getBidiKeyJsonValue(key: []const u8): []const u8 {
    if (std.mem.eql(u8, key, "\r") || std.mem.eql(u8, key, "\n") || std.mem.eql(u8, key, "Enter")) return "\\uE006";
    if (std.mem.eql(u8, key, "Cancel") || std.mem.eql(u8, key, "Abort")) return "\\uE001";
    if (std.mem.eql(u8, key, "Help")) return "\\uE002";
    if (std.mem.eql(u8, key, "Backspace")) return "\\uE003";
    if (std.mem.eql(u8, key, "Tab")) return "\\uE004";
    if (std.mem.eql(u8, key, "Clear")) return "\\uE005";
    if (std.mem.eql(u8, key, "NumpadEnter")) return "\\uE007";
    if (std.mem.eql(u8, key, "Shift") || std.mem.eql(u8, key, "ShiftLeft")) return "\\uE008";
    if (std.mem.eql(u8, key, "Control") || std.mem.eql(u8, key, "ControlLeft")) return "\\uE009";
    if (std.mem.eql(u8, key, "Alt") || std.mem.eql(u8, key, "AltLeft")) return "\\uE00A";
    if (std.mem.eql(u8, key, "Pause")) return "\\uE00B";
    if (std.mem.eql(u8, key, "Escape")) return "\\uE00C";
    if (std.mem.eql(u8, key, "PageUp")) return "\\uE00E";
    if (std.mem.eql(u8, key, "PageDown")) return "\\uE00F";
    if (std.mem.eql(u8, key, "End")) return "\\uE010";
    if (std.mem.eql(u8, key, "Home")) return "\\uE011";
    if (std.mem.eql(u8, key, "ArrowLeft")) return "\\uE012";
    if (std.mem.eql(u8, key, "ArrowUp")) return "\\uE013";
    if (std.mem.eql(u8, key, "ArrowRight")) return "\\uE014";
    if (std.mem.eql(u8, key, "ArrowDown")) return "\\uE015";
    if (std.mem.eql(u8, key, "Insert")) return "\\uE016";
    if (std.mem.eql(u8, key, "Delete")) return "\\uE017";
    if (std.mem.eql(u8, key, "F1")) return "\\uE031";
    if (std.mem.eql(u8, key, "F2")) return "\\uE032";
    if (std.mem.eql(u8, key, "F3")) return "\\uE033";
    if (std.mem.eql(u8, key, "F4")) return "\\uE034";
    if (std.mem.eql(u8, key, "F5")) return "\\uE035";
    if (std.mem.eql(u8, key, "F6")) return "\\uE036";
    if (std.mem.eql(u8, key, "F7")) return "\\uE037";
    if (std.mem.eql(u8, key, "F8")) return "\\uE038";
    if (std.mem.eql(u8, key, "F9")) return "\\uE039";
    if (std.mem.eql(u8, key, "F10")) return "\\uE03A";
    if (std.mem.eql(u8, key, "F11")) return "\\uE03B";
    if (std.mem.eql(u8, key, "F12")) return "\\uE03C";
    if (std.mem.eql(u8, key, "Meta") || std.mem.eql(u8, key, "MetaLeft")) return "\\uE03D";
    if (std.mem.eql(u8, key, "ShiftRight")) return "\\uE050";
    if (std.mem.eql(u8, key, "ControlRight")) return "\\uE051";
    if (std.mem.eql(u8, key, "AltRight")) return "\\uE052";
    if (std.mem.eql(u8, key, "MetaRight")) return "\\uE053";
    if (std.mem.eql(u8, key, "Space")) return " ";
    return key;
}

// ============================================================================
// BidiKeyboard — Keyboard Actions via input.performActions
// ============================================================================

pub struct BidiKeyboard {
    session: *BidiSession = undefined,
    context_id_buf: [128]u8 = undefined,
    context_id_len: usize = 0,

    pub static fn init(session: *BidiSession, context_id: []const u8): BidiKeyboard {
        let kb = BidiKeyboard{
            session: session,
            context_id_len: 0,
        };
        const n = if (context_id.len <= 128) context_id.len else 128;
        if (n > 0) @memcpy(kb.context_id_buf[0..n], context_id[0..n]);
        kb.context_id_len = n;
        return kb;
    }

    fn ctxId(): []const u8 {
        return _.context_id_buf[0.._.context_id_len];
    }

    fn sendKeyAction(*_, action_type: []const u8, key: []const u8): void!BidiError {
        const mapped = getBidiKeyJsonValue(key);
        let pbuf: [512]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.ctxId())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"actions\":[{\"type\":\"key\",\"id\":\"__puppeteer_keyboard\",\"actions\":[{\"type\":\"")?;
        pos = bidiWriteStr(&pbuf, pos, action_type)?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"value\":\"")?;
        if (std.mem.startsWith(u8, mapped, "\\uE")) {
            pos = bidiWriteStr(&pbuf, pos, mapped)?;
        } else {
            pos = bidiWriteJsonEscaped(&pbuf, pos, mapped)?;
        }
        pos = bidiWriteStr(&pbuf, pos, "\"}]}]}")?;

        let frame_buf: [1024]u8 = undefined;
        * = _.session.sendCommand("input.performActions", pbuf[0..pos], &frame_buf)?;
    }

    pub fn down(*_, key: []const u8): void!BidiError {
        return _.sendKeyAction("keyDown", key);
    }

    pub fn up(*_, key: []const u8): void!BidiError {
        return _.sendKeyAction("keyUp", key);
    }

    pub fn press(*_, key: []const u8, delay_ms: u32): void!BidiError {
        const mapped = getBidiKeyJsonValue(key);
        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.ctxId())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"actions\":[{\"type\":\"key\",\"id\":\"__puppeteer_keyboard\",\"actions\":[{\"type\":\"keyDown\",\"value\":\"")?;
        if (std.mem.startsWith(u8, mapped, "\\uE")) {
            pos = bidiWriteStr(&pbuf, pos, mapped)?;
        } else {
            pos = bidiWriteJsonEscaped(&pbuf, pos, mapped)?;
        }
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;
        if (delay_ms > 0) {
            let dbuf: [64]u8 = undefined;
            const ds = std.fmt.bufPrint(&dbuf, ",{{\"type\":\"pause\",\"duration\":{d}}}", .{delay_ms}) catch return BidiError.BufferTooSmall;
            pos = bidiWriteStr(&pbuf, pos, ds)?;
        }
        pos = bidiWriteStr(&pbuf, pos, ",{\"type\":\"keyUp\",\"value\":\"")?;
        if (std.mem.startsWith(u8, mapped, "\\uE")) {
            pos = bidiWriteStr(&pbuf, pos, mapped)?;
        } else {
            pos = bidiWriteJsonEscaped(&pbuf, pos, mapped)?;
        }
        pos = bidiWriteStr(&pbuf, pos, "\"}]}]}")?;

        let frame_buf: [1536]u8 = undefined;
        * = _.session.sendCommand("input.performActions", pbuf[0..pos], &frame_buf)?;
    }

    /// Type an ASCII/UTF-8 text string by encoding keyDown + keyUp pairs in a single BiDi batch.
    pub fn typeText(*_, text: []const u8, delay_ms: u32): void!BidiError {
        let pbuf: [4096]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.ctxId())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"actions\":[{\"type\":\"key\",\"id\":\"__puppeteer_keyboard\",\"actions\":[")?;

        let i: usize = 0;
        while (i < text.len) {
            if (i > 0) pos = bidiWriteStr(&pbuf, pos, ",")?;
            const ch_slice = text[i .. i + 1];
            const mapped = getBidiKeyJsonValue(ch_slice);
            pos = bidiWriteStr(&pbuf, pos, "{\"type\":\"keyDown\",\"value\":\"")?;
            if (std.mem.startsWith(u8, mapped, "\\uE")) {
                pos = bidiWriteStr(&pbuf, pos, mapped)?;
            } else {
                pos = bidiWriteJsonEscaped(&pbuf, pos, mapped)?;
            }
            pos = bidiWriteStr(&pbuf, pos, "\"}")?;
            if (delay_ms > 0) {
                let dbuf: [64]u8 = undefined;
                const ds = std.fmt.bufPrint(&dbuf, ",{{\"type\":\"pause\",\"duration\":{d}}}", .{delay_ms}) catch return BidiError.BufferTooSmall;
                pos = bidiWriteStr(&pbuf, pos, ds)?;
            }
            pos = bidiWriteStr(&pbuf, pos, ",{\"type\":\"keyUp\",\"value\":\"")?;
            if (std.mem.startsWith(u8, mapped, "\\uE")) {
                pos = bidiWriteStr(&pbuf, pos, mapped)?;
            } else {
                pos = bidiWriteJsonEscaped(&pbuf, pos, mapped)?;
            }
            pos = bidiWriteStr(&pbuf, pos, "\"}")?;
            i += 1;
        }
        pos = bidiWriteStr(&pbuf, pos, "]}]}")?;

        let frame_buf: [4608]u8 = undefined;
        * = _.session.sendCommand("input.performActions", pbuf[0..pos], &frame_buf)?;
    }
}

// ============================================================================
// BidiMouse — Pointer & Wheel Actions via input.performActions
// ============================================================================

pub struct BidiMouse {
    session: *BidiSession = undefined,
    context_id_buf: [128]u8 = undefined,
    context_id_len: usize = 0,
    pub x: f64 = 0.0,
    pub y: f64 = 0.0,
    pub pressed_buttons: u8 = 0,

    pub static fn init(session: *BidiSession, context_id: []const u8): BidiMouse {
        let m = BidiMouse{
            session: session,
            context_id_len: 0,
            x: 0.0,
            y: 0.0,
            pressed_buttons: 0,
        };
        const n = if (context_id.len <= 128) context_id.len else 128;
        if (n > 0) @memcpy(m.context_id_buf[0..n], context_id[0..n]);
        m.context_id_len = n;
        return m;
    }

    fn ctxId(): []const u8 {
        return _.context_id_buf[0.._.context_id_len];
    }

    pub fn move(*_, target_x: f64, target_y: f64, duration_ms: u32): void!BidiError {
        const ix: i64 = @as(i64, @intFromFloat(@round(target_x)));
        const iy: i64 = @as(i64, @intFromFloat(@round(target_y)));
        let pbuf: [512]u8 = undefined;
        const s = std.fmt.bufPrint(
            &pbuf,
            "{{\"context\":\"{s}\",\"actions\":[{{\"type\":\"pointer\",\"id\":\"__puppeteer_mouse\",\"parameters\":{{\"pointerType\":\"mouse\"}},\"actions\":[{{\"type\":\"pointerMove\",\"x\":{d},\"y\":{d},\"duration\":{d}}}]}}]}}",
            .{ _.ctxId(), ix, iy, duration_ms },
        ) catch return BidiError.BufferTooSmall;
        let frame_buf: [1024]u8 = undefined;
        * = _.session.sendCommand("input.performActions", s, &frame_buf)?;
        _.x = target_x;
        _.y = target_y;
    }

    pub fn down(*_, button: BidiMouseButton): void!BidiError {
        const code = bidiMouseButtonCode(button);
        let pbuf: [512]u8 = undefined;
        const s = std.fmt.bufPrint(
            &pbuf,
            "{{\"context\":\"{s}\",\"actions\":[{{\"type\":\"pointer\",\"id\":\"__puppeteer_mouse\",\"parameters\":{{\"pointerType\":\"mouse\"}},\"actions\":[{{\"type\":\"pointerDown\",\"button\":{d}}}]}}]}}",
            .{ _.ctxId(), code },
        ) catch return BidiError.BufferTooSmall;
        let frame_buf: [1024]u8 = undefined;
        * = _.session.sendCommand("input.performActions", s, &frame_buf)?;
        const shift: u3 = code as u3;
        _.pressed_buttons = _.pressed_buttons | ((1 as u8) << shift);
    }

    pub fn up(*_, button: BidiMouseButton): void!BidiError {
        const code = bidiMouseButtonCode(button);
        let pbuf: [512]u8 = undefined;
        const s = std.fmt.bufPrint(
            &pbuf,
            "{{\"context\":\"{s}\",\"actions\":[{{\"type\":\"pointer\",\"id\":\"__puppeteer_mouse\",\"parameters\":{{\"pointerType\":\"mouse\"}},\"actions\":[{{\"type\":\"pointerUp\",\"button\":{d}}}]}}]}}",
            .{ _.ctxId(), code },
        ) catch return BidiError.BufferTooSmall;
        let frame_buf: [1024]u8 = undefined;
        * = _.session.sendCommand("input.performActions", s, &frame_buf)?;
        const shift: u3 = code as u3;
        _.pressed_buttons = _.pressed_buttons & ~((1 as u8) << shift);
    }

    pub fn click(*_, target_x: f64, target_y: f64, button: BidiMouseButton, count: u32): void!BidiError {
        const ix: i64 = @as(i64, @intFromFloat(@round(target_x)));
        const iy: i64 = @as(i64, @intFromFloat(@round(target_y)));
        const code = bidiMouseButtonCode(button);
        const clicks = if (count > 0) count else 1;

        let pbuf: [1536]u8 = undefined;
        let pos: usize = 0;
        let hbuf: [256]u8 = undefined;
        const hdr = std.fmt.bufPrint(
            &hbuf,
            "{{\"context\":\"{s}\",\"actions\":[{{\"type\":\"pointer\",\"id\":\"__puppeteer_mouse\",\"parameters\":{{\"pointerType\":\"mouse\"}},\"actions\":[{{\"type\":\"pointerMove\",\"x\":{d},\"y\":{d}}}",
            .{ _.ctxId(), ix, iy },
        ) catch return BidiError.BufferTooSmall;
        pos = bidiWriteStr(&pbuf, pos, hdr)?;

        let c: u32 = 0;
        while (c < clicks) {
            let abuf: [128]u8 = undefined;
            const act = std.fmt.bufPrint(
                &abuf,
                ",{{\"type\":\"pointerDown\",\"button\":{d}}},{{\"type\":\"pointerUp\",\"button\":{d}}}",
                .{ code, code },
            ) catch return BidiError.BufferTooSmall;
            pos = bidiWriteStr(&pbuf, pos, act)?;
            c += 1;
        }
        pos = bidiWriteStr(&pbuf, pos, "]}]}")?;

        let frame_buf: [2048]u8 = undefined;
        * = _.session.sendCommand("input.performActions", pbuf[0..pos], &frame_buf)?;
        _.x = target_x;
        _.y = target_y;
    }

    pub fn wheel(*_, delta_x: i32, delta_y: i32): void!BidiError {
        const ix: i64 = @as(i64, @intFromFloat(@round(_.x)));
        const iy: i64 = @as(i64, @intFromFloat(@round(_.y)));
        let pbuf: [512]u8 = undefined;
        const s = std.fmt.bufPrint(
            &pbuf,
            "{{\"context\":\"{s}\",\"actions\":[{{\"type\":\"wheel\",\"id\":\"__puppeteer_wheel\",\"actions\":[{{\"type\":\"scroll\",\"x\":{d},\"y\":{d},\"deltaX\":{d},\"deltaY\":{d}}}]}}]}}",
            .{ _.ctxId(), ix, iy, delta_x, delta_y },
        ) catch return BidiError.BufferTooSmall;
        let frame_buf: [1024]u8 = undefined;
        * = _.session.sendCommand("input.performActions", s, &frame_buf)?;
    }
}

// ============================================================================
// BidiInput — Unified Input Controller (performActions / releaseActions / setFiles)
// ============================================================================

pub struct BidiInput {
    session: *BidiSession = undefined,
    context_id_buf: [128]u8 = undefined,
    context_id_len: usize = 0,
    pub keyboard: BidiKeyboard = .{},
    pub mouse: BidiMouse = .{},

    pub static fn init(session: *BidiSession, context_id: []const u8): BidiInput {
        let inp = BidiInput{
            session: session,
            context_id_len: 0,
            keyboard: BidiKeyboard.init(session, context_id),
            mouse: BidiMouse.init(session, context_id),
        };
        const n = if (context_id.len <= 128) context_id.len else 128;
        if (n > 0) @memcpy(inp.context_id_buf[0..n], context_id[0..n]);
        inp.context_id_len = n;
        return inp;
    }

    pub fn contextId(): []const u8 {
        return _.context_id_buf[0.._.context_id_len];
    }

    /// Execute raw `input.performActions` with a caller-supplied JSON actions array.
    pub fn performActionsRaw(*_, actions_json: []const u8): void!BidiError {
        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.contextId())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"actions\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (actions_json.len > 0) actions_json else "[]")?;
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [2560]u8 = undefined;
        * = _.session.sendCommand("input.performActions", pbuf[0..pos], &frame_buf)?;
    }

    /// Reset all pressed keys and pointer buttons via `input.releaseActions`.
    pub fn releaseActions(*_): void!BidiError {
        let pbuf: [256]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.contextId())?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [512]u8 = undefined;
        * = _.session.sendCommand("input.releaseActions", pbuf[0..pos], &frame_buf)?;
        _.mouse.pressed_buttons = 0;
    }

    /// Assign file paths to an `<input type="file">` element via `input.setFiles`.
    pub fn setFiles(*_, element_shared_id: []const u8, files: []const []const u8): void!BidiError {
        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"context\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.contextId())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"element\":{\"sharedId\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, element_shared_id)?;
        pos = bidiWriteStr(&pbuf, pos, "\"},\"files\":[")?;
        let i: usize = 0;
        while (i < files.len) {
            if (i > 0) pos = bidiWriteStr(&pbuf, pos, ",")?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, files[i])?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            i += 1;
        }
        pos = bidiWriteStr(&pbuf, pos, "]}")?;

        let frame_buf: [2560]u8 = undefined;
        * = _.session.sendCommand("input.setFiles", pbuf[0..pos], &frame_buf)?;
    }
}
