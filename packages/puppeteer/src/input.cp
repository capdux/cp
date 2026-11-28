// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/Input.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/input.cp — Keyboard, Mouse & Touchscreen Controllers
// Built directly on cap:cdp InputDomain & CDPSession with USKeyboardLayout
// ============================================================================

const std = @import("std");
import {
    CdpRequest,
    CdpFrameScanner,
    InputMouseEventParams,
    InputKeyEventParams,
    InputDomain,
} from "cap:cdp";
import { PuppeteerError, ClickOptions, TypeOptions, mapCdpError } from "./types";
import { CDPSession } from "./cdp_session";
import { USKeyboardLayout, KeyDefinition } from "./keyboard_layout";

pub struct Keyboard {
    session: *CDPSession = undefined,
    modifiers: i32 = 0,

    pub static fn init(session: *CDPSession): Keyboard {
        return Keyboard{
            session: session,
            modifiers: 0,
        };
    }

    fn dispatchKey(*_, event_type: []const u8, key: []const u8, code: []const u8, text: []const u8): void!PuppeteerError {
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const req = InputDomain.dispatchKeyEvent(
            _.session.endpoint.allocateId(),
            _.session.id(),
            InputKeyEventParams{
                event_type: event_type,
                key: key,
                code: code,
                text: text,
                modifiers: _.modifiers,
            },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const _ign_1 = _.session.send(req.method, req.params_json, &fbuf)?;
        * = _ign_1;
    }

    pub fn down(*_, key: []const u8): void!PuppeteerError {
        const shift_down = (_.modifiers & 8) != 0;
        const def: KeyDefinition = USKeyboardLayout.resolve(key, shift_down);
        if (std.mem.eql(u8, def.key, "Shift")) _.modifiers |= 8;
        if (std.mem.eql(u8, def.key, "Control")) _.modifiers |= 2;
        if (std.mem.eql(u8, def.key, "Alt")) _.modifiers |= 1;
        if (std.mem.eql(u8, def.key, "Meta")) _.modifiers |= 4;
        const text = if ((_.modifiers & (2 | 1 | 4)) != 0) "" else def.text;
        _.dispatchKey("keyDown", def.key, def.code, text)?;
    }

    pub fn up(*_, key: []const u8): void!PuppeteerError {
        const shift_down = (_.modifiers & 8) != 0;
        const def: KeyDefinition = USKeyboardLayout.resolve(key, shift_down);
        if (std.mem.eql(u8, def.key, "Shift")) _.modifiers &= ~@as(i32, 8);
        if (std.mem.eql(u8, def.key, "Control")) _.modifiers &= ~@as(i32, 2);
        if (std.mem.eql(u8, def.key, "Alt")) _.modifiers &= ~@as(i32, 1);
        if (std.mem.eql(u8, def.key, "Meta")) _.modifiers &= ~@as(i32, 4);
        _.dispatchKey("keyUp", def.key, def.code, "")?;
    }

    pub fn press(*_, key: []const u8, options: TypeOptions): void!PuppeteerError {
        const _ign_2 = options;
        * = _ign_2;
        _.down(key)?;
        _.up(key)?;
    }

    pub fn sendCharacter(*_, ch: []const u8): void!PuppeteerError {
        _.dispatchKey("char", ch, "", ch)?;
    }

    pub fn typeText(*_, text: []const u8, options: TypeOptions): void!PuppeteerError {
        const _ign_3 = options;
        * = _ign_3;
        let i: usize = 0;
        while (i < text.len) {
            const slice = text[i .. i + 1];
            _.down(slice)?;
            _.up(slice)?;
            i += 1;
        }
    }
}

pub struct Mouse {
    session: *CDPSession = undefined,
    x: f64 = 0.0,
    y: f64 = 0.0,
    button: []const u8 = "none",

    pub static fn init(session: *CDPSession): Mouse {
        return Mouse{
            session: session,
            x: 0.0,
            y: 0.0,
            button: "none",
        };
    }

    fn dispatchMouse(*_, event_type: []const u8, x: f64, y: f64, button: []const u8, click_count: i32): void!PuppeteerError {
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const req = InputDomain.dispatchMouseEvent(
            _.session.endpoint.allocateId(),
            _.session.id(),
            InputMouseEventParams{
                event_type: event_type,
                x: x,
                y: y,
                button: button,
                click_count: click_count,
            },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const _ign_4 = _.session.send(req.method, req.params_json, &fbuf)?;
        * = _ign_4;
    }

    pub fn move(*_, x: f64, y: f64): void!PuppeteerError {
        _.x = x;
        _.y = y;
        _.dispatchMouse("mouseMoved", x, y, _.button, 0)?;
    }

    pub fn down(*_, options: ClickOptions): void!PuppeteerError {
        _.button = options.button;
        _.dispatchMouse("mousePressed", _.x, _.y, options.button, options.click_count)?;
    }

    pub fn up(*_, options: ClickOptions): void!PuppeteerError {
        _.dispatchMouse("mouseReleased", _.x, _.y, options.button, options.click_count)?;
        _.button = "none";
    }

    pub fn click(*_, x: f64, y: f64, options: ClickOptions): void!PuppeteerError {
        _.move(x, y)?;
        _.down(options)?;
        _.up(options)?;
    }

    pub fn dblclick(*_, x: f64, y: f64): void!PuppeteerError {
        const opts = ClickOptions{ button: "left", click_count: 2, delay_ms: 0 };
        _.click(x, y, opts)?;
    }

    pub fn wheel(*_, delta_x: f64, delta_y: f64): void!PuppeteerError {
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"type\":\"mouseWheel\",\"x\":{d:.1},\"y\":{d:.1},\"deltaX\":{d:.1},\"deltaY\":{d:.1}}}",
            .{ _.x, _.y, delta_x, delta_y },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_5 = _.session.send("Input.dispatchMouseEvent", params, &fbuf)?;
        * = _ign_5;
    }

    pub fn reset(*_): void!PuppeteerError {
        _.x = 0.0;
        _.y = 0.0;
        _.button = "none";
    }
}

pub struct Touchscreen {
    session: *CDPSession = undefined,

    pub static fn init(session: *CDPSession): Touchscreen {
        return Touchscreen{ session: session };
    }

    fn dispatchTouch(*_, event_type: []const u8, x: f64, y: f64): void!PuppeteerError {
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const params = if (std.mem.eql(u8, event_type, "touchEnd"))
            "{\"type\":\"touchEnd\",\"touchPoints\":[]}"
        else
            std.fmt.bufPrint(
                &pbuf,
                "{{\"type\":\"{s}\",\"touchPoints\":[{{\"x\":{d:.1},\"y\":{d:.1}}}]}}",
                .{ event_type, x, y },
            ) catch return PuppeteerError.BufferTooSmall;
        const _ign_6 = _.session.send("Input.dispatchTouchEvent", params, &fbuf)?;
        * = _ign_6;
    }

    pub fn touchStart(*_, x: f64, y: f64): void!PuppeteerError {
        _.dispatchTouch("touchStart", x, y)?;
    }

    pub fn touchMove(*_, x: f64, y: f64): void!PuppeteerError {
        _.dispatchTouch("touchMove", x, y)?;
    }

    pub fn touchEnd(*_): void!PuppeteerError {
        _.dispatchTouch("touchEnd", 0.0, 0.0)?;
    }

    pub fn tap(*_, x: f64, y: f64): void!PuppeteerError {
        _.touchStart(x, y)?;
        _.touchEnd()?;
    }
}
