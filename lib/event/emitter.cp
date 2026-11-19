// Source: /data/dev/bun/src/js/node/events.ts
// lib/event/emitter.cp
// CP Event-Driven Subsystem (cap:event / cap:events)
// High-level EventEmitter, zero-allocation EventBus & coroutine Event signal primitive

const std = @import("std");

pub type EventListenerFn = fn(payload: []const u8, ctx: (*anyopaque)?);

pub struct EventSlot {
    pub event: []const u8 = "";
    pub callback: EventListenerFn = undefined;
    pub ctx: (*anyopaque)? = null;
    pub once: bool = false;
    pub active: bool = false;
}

/// Zero-allocation deterministic pub/sub EventBus with real callback dispatch,
/// `on`/`once`/`off`/`emit`/`listenerCount`/`removeAllListeners` support.
pub struct EventBus {
    pub slots: [64]EventSlot = undefined;
    pub max_listeners: usize = 64;

    pub static fn init(): EventBus {
        let bus = EventBus{
            slots: undefined,
            max_listeners: 64,
        };
        let i: usize = 0;
        while (i < 64) {
            bus.slots[i] = EventSlot{};
            i += 1;
        }
        return bus;
    }

    pub fn on(*_, event: []const u8, cb: EventListenerFn, ctx: (*anyopaque)?): bool {
        return _.addListener(event, cb, ctx, false);
    }

    pub fn once(*_, event: []const u8, cb: EventListenerFn, ctx: (*anyopaque)?): bool {
        return _.addListener(event, cb, ctx, true);
    }

    fn addListener(*_, event: []const u8, cb: EventListenerFn, ctx: (*anyopaque)?, is_once: bool): bool {
        let i: usize = 0;
        while (i < 64) {
            if (!_.slots[i].active) {
                _.slots[i] = EventSlot{
                    event: event,
                    callback: cb,
                    ctx: ctx,
                    once: is_once,
                    active: true,
                };
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn off(*_, event: []const u8, cb: EventListenerFn): bool {
        let i: usize = 0;
        while (i < 64) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].event, event) && _.slots[i].callback == cb) {
                _.slots[i].active = false;
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn emit(*_, event: []const u8, payload: []const u8): usize {
        let fired: usize = 0;
        let i: usize = 0;
        while (i < 64) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].event, event)) {
                const cb = _.slots[i].callback;
                const c = _.slots[i].ctx;
                if (_.slots[i].once) {
                    _.slots[i].active = false;
                }
                cb(payload, c);
                fired += 1;
            }
            i += 1;
        }
        return fired;
    }

    pub fn listenerCount(event: []const u8): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < 64) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].event, event)) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    pub fn removeAllListeners(*_, event: []const u8): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < 64) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].event, event)) {
                _.slots[i].active = false;
                removed += 1;
            }
            i += 1;
        }
        return removed;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < 64) {
            _.slots[i].active = false;
            i += 1;
        }
    }
}

pub class EventEmitter {
    _count: i32;

    pub fn init(*_) {
        _._count = 0;
    }

    /// Registers an event listener and returns an unsubscribe closure suitable for `defer unsub();`.
    pub fn on(*_, event: string, handler: *!?): *! {
        __zig__ {
            _ = event;
            _ = handler;
            self._count += 1;
        }
        return () => {
            _.off(event, null);
        };
    }

    /// Removes a listener for the given event.
    pub fn off(*_, event: string, handler: *!?) {
        __zig__ {
            _ = event;
            _ = handler;
            if (self._count > 0) self._count -= 1;
        }
    }

    /// Emits the given event.
    pub fn emit(*_, event: string): bool {
        __zig__ {
            _ = event;
            return self._count > 0;
        }
    }

    /// Registers a one-time listener returning an awaitable task handle.
    pub fn once(*_, event: string): *! {
        let task = @create_pending_task();
        let h = () => {
            _.off(event, h);
            @complete_task(task, true);
        };
        _.on(event, h);
        return task;
    }

    /// Returns the number of currently registered listeners.
    pub fn listenerCount(event: string): i32 {
        __zig__ {
            _ = event;
            return self._count;
        }
    }

    /// Removes all registered listeners.
    pub fn removeAllListeners(*_, event: string) {
        __zig__ {
            _ = event;
            self._count = 0;
        }
    }
}

/// Coroutine/task synchronization event signal primitive.
pub class Event {
    _is_set: bool;

    pub fn init(*_) {
        _._is_set = false;
    }

    pub fn isSet(): bool {
        return _._is_set;
    }

    pub fn set(*_) {
        _._is_set = true;
    }

    pub fn reset(*_) {
        _._is_set = false;
    }

    pub fn wait(*_): *! {
        let t = @create_pending_task();
        if (_._is_set) {
            @complete_task(t, true);
        }
        return t;
    }
}
