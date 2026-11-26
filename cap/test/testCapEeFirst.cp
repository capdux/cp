import { test, expect_eq } from "cap:test";
import {
    FIRST_MAX_WATCHERS,
    FirstResult,
    FirstRace
} from "../../packages/@cap/ee-first/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ee-first — first matching event wins and cancels remaining watchers", () => {
        let race = FirstRace.new();
        expect_eq(race.add(1 as u32, "close"), true);
        expect_eq(race.add(1 as u32, "error"), true);
        expect_eq(race.add(2 as u32, "finish"), true);
        expect_eq(race.activeCount(), 3);
        expect_eq(race.isSettled(), false);

        let res: FirstResult = race.emit(2 as u32, "finish", 200 as i32);
        expect_eq(res.triggered, true);
        expect_eq(res.emitter_id, 2 as u32);
        expect_eq(strEq(res.event_name, "finish"), true);
        expect_eq(res.payload_code, 200 as i32);
        expect_eq(res.cancelled_count, 2);

        expect_eq(race.isSettled(), true);
        expect_eq(race.activeCount(), 0);
        expect_eq(race.winner().triggered, true);
        expect_eq(race.winner().payload_code, 200 as i32);
    });

    test("2. @cap/ee-first — subsequent emits after settlement are ignored", () => {
        let race = FirstRace.new();
        expect_eq(race.add(1 as u32, "end"), true);
        expect_eq(race.add(1 as u32, "error"), true);

        let r1 = race.emit(1 as u32, "end", 0 as i32);
        expect_eq(r1.triggered, true);

        let r2 = race.emit(1 as u32, "error", -1 as i32);
        expect_eq(r2.triggered, false);
        expect_eq(strEq(race.winner().event_name, "end"), true);
    });

    test("3. @cap/ee-first — unmatched emitter or event name does not settle race", () => {
        let race = FirstRace.new();
        expect_eq(race.add(10 as u32, "data"), true);

        let r1 = race.emit(99 as u32, "data", 1 as i32);
        expect_eq(r1.triggered, false);
        expect_eq(race.isSettled(), false);
        expect_eq(race.activeCount(), 1);

        let r2 = race.emit(10 as u32, "other", 2 as i32);
        expect_eq(r2.triggered, false);
        expect_eq(race.isSettled(), false);
        expect_eq(race.activeCount(), 1);
    });

    test("4. @cap/ee-first — manual cancel clears all active watchers and blocks future emits", () => {
        let race = FirstRace.new();
        expect_eq(race.add(1 as u32, "close"), true);
        expect_eq(race.add(2 as u32, "close"), true);
        expect_eq(race.add(3 as u32, "error"), true);

        let cancelled = race.cancel();
        expect_eq(cancelled, 3);
        expect_eq(race.activeCount(), 0);
        expect_eq(race.isSettled(), true);
        expect_eq(race.winner().triggered, false);

        // Second cancel returns 0 and subsequent emits are ignored
        expect_eq(race.cancel(), 0);
        let after = race.emit(1 as u32, "close", 7 as i32);
        expect_eq(after.triggered, false);
    });

    test("5. @cap/ee-first — rejects empty event name and additions after settlement", () => {
        let race = FirstRace.new();
        expect_eq(race.add(1 as u32, ""), false);
        expect_eq(race.add(1 as u32, "ready"), true);

        let r = race.emit(1 as u32, "ready", 1 as i32);
        expect_eq(r.triggered, true);
        expect_eq(race.add(2 as u32, "late"), false);
    });

    test("6. @cap/ee-first — enforces FIRST_MAX_WATCHERS (16) capacity boundary", () => {
        let race = FirstRace.new();
        let i: usize = 0;
        while (i < FIRST_MAX_WATCHERS) {
            expect_eq(race.add(i as u32, "tick"), true);
            i = i + 1;
        }
        expect_eq(race.activeCount(), 16);
        expect_eq(race.add(99 as u32, "overflow"), false);

        let res = race.emit(15 as u32, "tick", 42 as i32);
        expect_eq(res.triggered, true);
        expect_eq(res.cancelled_count, 15);
        expect_eq(race.activeCount(), 0);
    });
}
