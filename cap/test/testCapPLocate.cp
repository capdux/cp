import { test, expect_eq } from "cap:test";
import {
    PLocateOptions,
    PLocatePlanner,
} from "../../packages/p-locate/src/index.cp";

fn main() {
    test("1. @cap/p-locate — rejects zero concurrency and respects defaults", () => {
        let bad_opts: PLocateOptions = PLocateOptions { concurrency: 0, preserve_order: true };
        expect_eq(PLocatePlanner.new(bad_opts) == null, true);

        let def_opts: PLocateOptions = PLocateOptions.defaults();
        expect_eq(def_opts.concurrency, 4);
        expect_eq(def_opts.preserve_order, true);

        let p_opt: PLocatePlanner? = PLocatePlanner.new(def_opts);
        expect_eq(p_opt != null, true);
        let p: PLocatePlanner = p_opt ?? PLocatePlanner.defaultPlanner();
        expect_eq(p.isSettled(), false);
        expect_eq(p.winnerId() == null, true);
    });

    test("2. @cap/p-locate — preserve_order waits for earlier candidates before resolving", () => {
        let opts: PLocateOptions = PLocateOptions { concurrency: 3, preserve_order: true };
        let p: PLocatePlanner = PLocatePlanner.new(opts) ?? PLocatePlanner.defaultPlanner();
        expect_eq(p.addCandidate(10), true);
        expect_eq(p.addCandidate(20), true);
        expect_eq(p.addCandidate(30), true);

        expect_eq(p.dispatchNext() ?? 0, 10);
        expect_eq(p.dispatchNext() ?? 0, 20);
        expect_eq(p.dispatchNext() ?? 0, 30);

        // Candidate 20 matches first, but 10 is still in flight
        expect_eq(p.reportResult(20, true) == null, true);
        expect_eq(p.isSettled(), false);

        // Once 10 finishes with false, 20 becomes the earliest winner
        expect_eq(p.reportResult(10, false) ?? 0, 20);
        expect_eq(p.isSettled(), true);
        expect_eq(p.winnerId() ?? 0, 20);
    });

    test("3. @cap/p-locate — preserve_order picks earliest match when multiple candidates match", () => {
        let opts: PLocateOptions = PLocateOptions { concurrency: 3, preserve_order: true };
        let p: PLocatePlanner = PLocatePlanner.new(opts) ?? PLocatePlanner.defaultPlanner();
        expect_eq(p.addCandidate(100), true);
        expect_eq(p.addCandidate(200), true);
        expect_eq(p.addCandidate(300), true);

        expect_eq(p.dispatchNext() ?? 0, 100);
        expect_eq(p.dispatchNext() ?? 0, 200);
        expect_eq(p.dispatchNext() ?? 0, 300);

        expect_eq(p.reportResult(200, true) == null, true);
        expect_eq(p.reportResult(100, true) ?? 0, 100);
        expect_eq(p.isSettled(), true);
        expect_eq(p.winnerId() ?? 0, 100);
    });

    test("4. @cap/p-locate — unordered mode resolves immediately on first match and cancels queue", () => {
        let opts: PLocateOptions = PLocateOptions { concurrency: 2, preserve_order: false };
        let p: PLocatePlanner = PLocatePlanner.new(opts) ?? PLocatePlanner.defaultPlanner();
        expect_eq(p.addCandidate(1), true);
        expect_eq(p.addCandidate(2), true);
        expect_eq(p.addCandidate(3), true);
        expect_eq(p.addCandidate(4), true);

        expect_eq(p.dispatchNext() ?? 0, 1);
        expect_eq(p.dispatchNext() ?? 0, 2);
        expect_eq(p.dispatchNext() == null, true);

        expect_eq(p.reportResult(2, true) ?? 0, 2);
        expect_eq(p.isSettled(), true);
        expect_eq(p.winnerId() ?? 0, 2);
        // Pending queue (3, 4) was cleared
        expect_eq(p.dispatchNext() == null, true);
        expect_eq(p.addCandidate(5), false);
    });

    test("5. @cap/p-locate — enforces bounded concurrency via PLimit across batches", () => {
        let opts: PLocateOptions = PLocateOptions { concurrency: 2, preserve_order: true };
        let p: PLocatePlanner = PLocatePlanner.new(opts) ?? PLocatePlanner.defaultPlanner();
        expect_eq(p.addCandidate(11), true);
        expect_eq(p.addCandidate(22), true);
        expect_eq(p.addCandidate(33), true);

        expect_eq(p.dispatchNext() ?? 0, 11);
        expect_eq(p.dispatchNext() ?? 0, 22);
        expect_eq(p.dispatchNext() == null, true);

        expect_eq(p.reportResult(11, false) == null, true);
        expect_eq(p.dispatchNext() ?? 0, 33);
        expect_eq(p.reportResult(22, false) == null, true);
        expect_eq(p.reportResult(33, true) ?? 0, 33);
        expect_eq(p.isSettled(), true);
    });

    test("6. @cap/p-locate — settles with null winner when all candidates fail to match", () => {
        let opts: PLocateOptions = PLocateOptions { concurrency: 2, preserve_order: false };
        let p: PLocatePlanner = PLocatePlanner.new(opts) ?? PLocatePlanner.defaultPlanner();
        expect_eq(p.addCandidate(7), true);
        expect_eq(p.addCandidate(8), true);

        expect_eq(p.dispatchNext() ?? 0, 7);
        expect_eq(p.dispatchNext() ?? 0, 8);

        expect_eq(p.reportResult(7, false) == null, true);
        expect_eq(p.isSettled(), false);
        expect_eq(p.reportResult(8, false) == null, true);
        expect_eq(p.isSettled(), true);
        expect_eq(p.winnerId() == null, true);
    });
}
