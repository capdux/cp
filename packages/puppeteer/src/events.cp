// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/common/PuppeteerEventEmitter.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/events.cp — Typed PuppeteerEventEmitter (on / once / off / emit)
// Fixed-pool zero-heap-allocation event bus with wildcard "*" support
// ============================================================================

const std = @import("std");

pub type EventHandlerFn = fn (event_name: []const u8, payload: []const u8, ctx: usize) void;

pub enum PageEvent {
    Console,
    Dialog,
    Request,
    Response,
    Load,
    DomContentLoaded,
    Close,
    Error,
    FrameAttached,
    FrameDetached,
    FileChooser,
    Wildcard,
}

pub fn pageEventName(ev: PageEvent): []const u8 {
    switch (ev) {
        PageEvent.Console => return "console",
        PageEvent.Dialog => return "dialog",
        PageEvent.Request => return "request",
        PageEvent.Response => return "response",
        PageEvent.Load => return "load",
        PageEvent.DomContentLoaded => return "domcontentloaded",
        PageEvent.Close => return "close",
        PageEvent.Error => return "error",
        PageEvent.FrameAttached => return "frameattached",
        PageEvent.FrameDetached => return "framedetached",
        PageEvent.FileChooser => return "filechooser",
        PageEvent.Wildcard => return "*",
    }
}

pub struct EventListenerEntry {
    event_name: []const u8 = "",
    handler: EventHandlerFn = undefined,
    ctx: usize = 0,
    once: bool = false,
    active: bool = false,
}

pub struct PuppeteerEventEmitter {
    entries: [32]EventListenerEntry = undefined,
    count: usize = 0,

    pub static fn init(): PuppeteerEventEmitter {
        return PuppeteerEventEmitter{ count: 0 };
    }

    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.addListener(event_name, handler, ctx, false);
    }

    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.addListener(event_name, handler, ctx, true);
    }

    fn addListener(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize, is_once: bool): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (!_.entries[i].active) {
                _.entries[i] = EventListenerEntry{
                    event_name: event_name,
                    handler: handler,
                    ctx: ctx,
                    once: is_once,
                    active: true,
                };
                return true;
            }
            i += 1;
        }
        if (_.count >= 32) return false;
        _.entries[_.count] = EventListenerEntry{
            event_name: event_name,
            handler: handler,
            ctx: ctx,
            once: is_once,
            active: true,
        };
        _.count += 1;
        return true;
    }

    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn) {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].event_name, event_name) && _.entries[i].handler == handler) {
                _.entries[i].active = false;
            }
            i += 1;
        }
    }

    pub fn removeAllListeners(*_, event_name: []const u8) {
        let i: usize = 0;
        while (i < _.count) {
            if (event_name.len == 0 || std.mem.eql(u8, _.entries[i].event_name, event_name)) {
                _.entries[i].active = false;
            }
            i += 1;
        }
        if (event_name.len == 0) {
            _.count = 0;
        }
    }

    pub fn listenerCount(event_name: []const u8): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].active) {
                if (event_name.len == 0 || std.mem.eql(u8, _.entries[i].event_name, event_name)) {
                    total += 1;
                }
            }
            i += 1;
        }
        return total;
    }

    pub fn emit(*_, event_name: []const u8, payload: []const u8): bool {
        let dispatched = false;
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].active) {
                const ename = _.entries[i].event_name;
                const is_match = (ename.len == 1 && ename[0] == '*') ||
                    (ename.len == event_name.len && ename.len > 0 && ename[0] == event_name[0] && std.mem.eql(u8, ename, event_name));
                if (is_match) {
                    const cb = _.entries[i].handler;
                    const ctx = _.entries[i].ctx;
                    if (_.entries[i].once) {
                        _.entries[i].active = false;
                    }
                    cb(event_name, payload, ctx);
                    dispatched = true;
                }
            }
            i += 1;
        }
        return dispatched;
    }
}
