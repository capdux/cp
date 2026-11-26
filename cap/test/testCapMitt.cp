import { test, expect_eq } from "cap:test";
import {
    MIT_MAX_LISTENERS,
    Emitter,
    mitt,
} from "../../packages/@cap/mitt/src/index.cp";

fn main() {
    test("1. @cap/mitt — Basic on/emit registration order and dispatch log", () => {
        let bus: Emitter = Emitter.new();
        expect_eq(bus.totalListeners(), 0);
        expect_eq(bus.listenerCount("foo"), 0);

        expect_eq(bus.on("foo", 10), true);
        expect_eq(bus.on("foo", 20), true);
        expect_eq(bus.on("bar", 30), true);

        expect_eq(bus.totalListeners(), 3);
        expect_eq(bus.listenerCount("foo"), 2);
        expect_eq(bus.listenerCount("bar"), 1);
        expect_eq(bus.hasListener("foo", 10), true);
        expect_eq(bus.hasListener("foo", 99), false);

        const fired: usize = bus.emit("foo", 42);
        expect_eq(fired, 2);
        expect_eq(bus.last_dispatch_count, 2);
        expect_eq(bus.emit_count, 2);
        expect_eq(bus.last_dispatched[0].handler_id, 10);
        expect_eq(bus.last_dispatched[0].payload, 42);
        expect_eq(bus.last_dispatched[0].is_wildcard, false);
        expect_eq(bus.last_dispatched[1].handler_id, 20);
        expect_eq(bus.last_dispatched[1].payload, 42);
    });

    test("2. @cap/mitt — Wildcard '*' receives all events after exact listeners", () => {
        let bus: Emitter = mitt();
        expect_eq(bus.on("*", 900), true);
        expect_eq(bus.on("ping", 100), true);
        expect_eq(bus.on("*", 901), true);

        const fired1: usize = bus.emit("ping", 777);
        expect_eq(fired1, 3);
        // Exact match listener runs first, then wildcards in registration order
        expect_eq(bus.last_dispatched[0].handler_id, 100);
        expect_eq(bus.last_dispatched[0].is_wildcard, false);
        expect_eq(bus.last_dispatched[1].handler_id, 900);
        expect_eq(bus.last_dispatched[1].is_wildcard, true);
        expect_eq(bus.last_dispatched[2].handler_id, 901);
        expect_eq(bus.last_dispatched[2].is_wildcard, true);

        // Unmatched named event still triggers wildcard listeners
        const fired2: usize = bus.emit("unknown", -5);
        expect_eq(fired2, 2);
        expect_eq(bus.last_dispatched[0].handler_id, 900);
        expect_eq(bus.last_dispatched[0].payload, -5);
        expect_eq(bus.last_dispatched[1].handler_id, 901);
        expect_eq(bus.emit_count, 5);
    });

    test("3. @cap/mitt — off(event, handler_id) removes specific listener and preserves order", () => {
        let bus: Emitter = Emitter.new();
        * = bus.on("tick", 1);
        * = bus.on("tick", 2);
        * = bus.on("tick", 3);

        expect_eq(bus.off("tick", 2), true);
        expect_eq(bus.off("tick", 2), false);
        expect_eq(bus.listenerCount("tick"), 2);
        expect_eq(bus.hasListener("tick", 2), false);

        const fired: usize = bus.emit("tick", 123);
        expect_eq(fired, 2);
        expect_eq(bus.last_dispatched[0].handler_id, 1);
        expect_eq(bus.last_dispatched[1].handler_id, 3);
    });

    test("4. @cap/mitt — off(event, 0) removes all listeners for that event only", () => {
        let bus: Emitter = Emitter.new();
        * = bus.on("a", 1);
        * = bus.on("a", 2);
        * = bus.on("b", 3);
        * = bus.on("*", 9);

        expect_eq(bus.off("a", 0), true);
        expect_eq(bus.off("a", 0), false);
        expect_eq(bus.listenerCount("a"), 0);
        expect_eq(bus.hasListener("a", 0), false);
        expect_eq(bus.listenerCount("b"), 1);
        expect_eq(bus.listenerCount("*"), 1);
        expect_eq(bus.totalListeners(), 2);

        const fired: usize = bus.emit("b", 88);
        expect_eq(fired, 2);
        expect_eq(bus.last_dispatched[0].handler_id, 3);
        expect_eq(bus.last_dispatched[1].handler_id, 9);
    });

    test("5. @cap/mitt — clear() removes all listeners and resets counts", () => {
        let bus: Emitter = Emitter.new();
        * = bus.on("x", 10);
        * = bus.on("y", 20);
        * = bus.on("*", 30);
        expect_eq(bus.totalListeners(), 3);

        bus.clear();
        expect_eq(bus.totalListeners(), 0);
        expect_eq(bus.listenerCount("x"), 0);
        expect_eq(bus.listenerCount("*"), 0);
        expect_eq(bus.hasListener("x", 10), false);
        expect_eq(bus.emit("x", 1), 0);
    });

    test("6. @cap/mitt — Capacity boundary (32 slots) and invalid argument guards", () => {
        let bus: Emitter = Emitter.new();
        // Reject empty event name or handler_id == 0
        expect_eq(bus.on("", 1), false);
        expect_eq(bus.on("evt", 0), false);

        // Fill all 32 slots
        let i: usize = 0;
        while (i < MIT_MAX_LISTENERS) {
            expect_eq(bus.on("burst", (i + 1) as u32), true);
            i += 1;
        }
        expect_eq(bus.totalListeners(), 32);

        // 33rd registration must fail cleanly
        expect_eq(bus.on("burst", 999), false);

        // Removing one frees a slot for re-registration
        expect_eq(bus.off("burst", 1), true);
        expect_eq(bus.totalListeners(), 31);
        expect_eq(bus.on("burst", 999), true);
        expect_eq(bus.totalListeners(), 32);

        const fired: usize = bus.emit("burst", 55);
        expect_eq(fired, 32);
        expect_eq(bus.last_dispatched[31].handler_id, 999);
    });
}
