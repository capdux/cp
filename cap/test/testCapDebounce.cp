import { test, expect_eq } from "cap:test";
import { Debouncer, Throttler } from "../../packages/@cap/debounce/src/index.cp";

fn main() {
    test("1. @cap/debounce — Standard trailing debounce coalesces burst into 1 execution", () => {
        let d = Debouncer.init(100, false, true, 0);
        expect_eq(d.trigger(1000), false);
        expect_eq(d.isPending(), true);
        expect_eq(d.nextDueMs(), 1100 as u64);

        // Rapid follow-ups push the deadline forward
        expect_eq(d.trigger(1050), false);
        expect_eq(d.nextDueMs(), 1150 as u64);
        expect_eq(d.trigger(1120), false);
        expect_eq(d.nextDueMs(), 1220 as u64);

        // Not due yet at 1219
        expect_eq(d.poll(1219), false);
        expect_eq(d.execCount(), 0 as u64);

        // Fires exactly once at 1220
        expect_eq(d.poll(1220), true);
        expect_eq(d.isPending(), false);
        expect_eq(d.nextDueMs(), 0 as u64);
        expect_eq(d.execCount(), 1 as u64);
        expect_eq(d.poll(1500), false);
    });

    test("2. @cap/debounce — leading=true immediate fire & cooldown suppression", () => {
        let d = Debouncer.init(100, true, true, 0);
        // First trigger fires immediately
        expect_eq(d.trigger(500), true);
        expect_eq(d.isPending(), false);
        expect_eq(d.execCount(), 1 as u64);

        // Follow-up within cooldown is suppressed but arms trailing
        expect_eq(d.trigger(540), false);
        expect_eq(d.isPending(), true);
        expect_eq(d.poll(639), false);
        expect_eq(d.poll(640), true);
        expect_eq(d.execCount(), 2 as u64);

        // Leading-only mode (trailing=false) never fires on poll
        let lead_only = Debouncer.init(100, true, false, 0);
        expect_eq(lead_only.trigger(100), true);
        expect_eq(lead_only.trigger(150), false);
        expect_eq(lead_only.isPending(), false);
        expect_eq(lead_only.poll(300), false);
        expect_eq(lead_only.execCount(), 1 as u64);
    });

    test("3. @cap/debounce — maxWait forces periodic execution under continuous input", () => {
        // wait=100ms, maxWait=250ms, trailing=true
        let d = Debouncer.init(100, false, true, 250);
        let t: u64 = 1000;
        let fired: u64 = 0;

        // Trigger every 50ms for 600ms (13 events: 1000, 1050, ..., 1600)
        let i: usize = 0;
        while (i <= 12) {
            if (d.trigger(t)) {
                fired += 1;
            }
            t += 50;
            i += 1;
        }
        // Should have fired at 1250 and 1500 via maxWait
        expect_eq(fired, 2 as u64);
        expect_eq(d.isPending(), true);

        // Final trailing fires 100ms after last trigger (1600 + 100 = 1700)
        expect_eq(d.poll(1700), true);
        expect_eq(d.execCount(), 3 as u64);
    });

    test("4. @cap/debounce — flush & cancel semantics", () => {
        let d = Debouncer.init(200, false, true, 0);
        expect_eq(d.trigger(100), false);
        expect_eq(d.isPending(), true);

        // Flush executes immediately
        expect_eq(d.flush(120), true);
        expect_eq(d.isPending(), false);
        expect_eq(d.execCount(), 1 as u64);
        // Flushing when idle returns false
        expect_eq(d.flush(130), false);

        // Cancel drops pending invocation
        expect_eq(d.trigger(200), false);
        expect_eq(d.isPending(), true);
        d.cancel();
        expect_eq(d.isPending(), false);
        expect_eq(d.poll(500), false);
        expect_eq(d.execCount(), 1 as u64);
    });

    test("5. @cap/debounce — Throttler fixed-interval limiting & trailing call", () => {
        let th = Throttler.init(100, true, true);
        // t=1000: leading fires immediately
        expect_eq(th.trigger(1000), true);
        expect_eq(th.execCount(), 1 as u64);

        // t=1020, 1060: suppressed inside [1000, 1100), trailing armed
        expect_eq(th.trigger(1020), false);
        expect_eq(th.trigger(1060), false);
        expect_eq(th.isPending(), true);

        // t=1099: still inside window
        expect_eq(th.poll(1099), false);

        // t=1100: window ends, trailing fires and opens next window [1100, 1200)
        expect_eq(th.poll(1100), true);
        expect_eq(th.isPending(), false);
        expect_eq(th.execCount(), 2 as u64);

        // t=1150: suppressed inside new window, then cancelled
        expect_eq(th.trigger(1150), false);
        expect_eq(th.isPending(), true);
        th.cancel();
        expect_eq(th.isPending(), false);
        expect_eq(th.poll(1300), false);
        expect_eq(th.execCount(), 2 as u64);
    });

    test("6. @cap/debounce — Zero wait/interval & execCount tracking", () => {
        let d0 = Debouncer.init(0, false, true, 0);
        expect_eq(d0.trigger(10), true);
        expect_eq(d0.trigger(10), true);
        expect_eq(d0.trigger(11), true);
        expect_eq(d0.execCount(), 3 as u64);

        let th0 = Throttler.init(0, true, false);
        expect_eq(th0.trigger(10), true);
        expect_eq(th0.trigger(10), true);
        expect_eq(th0.execCount(), 2 as u64);

        // Neither leading nor trailing never fires
        let noop = Debouncer.init(50, false, false, 0);
        expect_eq(noop.trigger(100), false);
        expect_eq(noop.poll(500), false);
        expect_eq(noop.execCount(), 0 as u64);
    });
}
