import { test, expect_eq } from "cap:test";
import {
    DESTROY_ACTION_NONE, DESTROY_ACTION_DESTROYED,
    DESTROY_ACTION_DEFERRED_CLOSE, DESTROY_ACTION_ALREADY_DESTROYED,
    DestroyableStream, DestroyState,
    destroyStream, destroyWithError,
    destroy, isDestroyed, destroyAll, destroyAllStates,
    isAnyDestroyed, areAllDestroyed,
} from "../../packages/destroy/src/index.cp";
import { REASON_ERROR, ListenerSlot } from "../../packages/on-finished/src/index.cp";

fn main() {
    test("1. @cap/destroy — generic stream destroy marks lifecycle closed idempotently", () => {
        let s = DestroyableStream.generic();
        expect_eq(s.isDestroyed(), false);
        expect_eq(destroyStream(*s), DESTROY_ACTION_DESTROYED);
        expect_eq(s.isDestroyed(), true);
        expect_eq(s.destroy_calls, 1);
        expect_eq(destroyStream(*s), DESTROY_ACTION_ALREADY_DESTROYED);
        expect_eq(s.destroy_calls, 1);
    });

    test("2. @cap/destroy — open ReadStream closes fd immediately", () => {
        let s = DestroyableStream.readFile(7, true);
        expect_eq(destroyStream(*s), DESTROY_ACTION_DESTROYED);
        expect_eq(s.fd_open, false);
        expect_eq(s.close_calls, 1);
        expect_eq(s.lifecycle.isFinished(), true);
    });

    test("3. @cap/destroy — pending ReadStream defers close until onOpen()", () => {
        let s = DestroyableStream.readFile(-1, false);
        expect_eq(destroyStream(*s), DESTROY_ACTION_DEFERRED_CLOSE);
        expect_eq(s.close_calls, 0);
        // Late fd open triggers immediate close
        expect_eq(s.onOpen(12), DESTROY_ACTION_DESTROYED);
        expect_eq(s.fd_open, false);
        expect_eq(s.close_calls, 1);
        expect_eq(s.lifecycle.isFinished(), true);
    });

    test("4. @cap/destroy — normal onOpen() on active ReadStream keeps fd open", () => {
        let s = DestroyableStream.readFile(-1, false);
        expect_eq(s.onOpen(9), DESTROY_ACTION_NONE);
        expect_eq(s.fd_open, true);
        expect_eq(s.fd, 9);
        expect_eq(s.close_calls, 0);
    });

    test("5. @cap/destroy — Zlib stream invokes close and marks finished", () => {
        let s = DestroyableStream.zlib();
        expect_eq(destroyStream(*s), DESTROY_ACTION_DESTROYED);
        expect_eq(s.close_calls, 1);
        expect_eq(s.lifecycle.isFinished(), true);
    });

    test("6. @cap/destroy — destroyWithError notifies on-finished listeners with error code", () => {
        let s = DestroyableStream.readFile(5, true);
        * = s.lifecycle.onFinished(42);
        expect_eq(destroyWithError(*s, 32), DESTROY_ACTION_DESTROYED);
        expect_eq(s.fd_open, false);
        let slot = s.lifecycle.getListener(42) ?? ListenerSlot { id: 0, fired: false, reason: 0, err_code: 0 };
        expect_eq(slot.fired, true);
        expect_eq(slot.reason, REASON_ERROR);
        expect_eq(slot.err_code, 32);
    });

    test("7. @cap/destroy — DestroyState idempotent lifecycle & error tracking", () => {
        let st = DestroyState.new(1001);
        expect_eq(st.isDestroyed(), false);
        expect_eq(st.destroy(), true);
        expect_eq(st.isDestroyed(), true);
        expect_eq(st.destroy_calls, 1 as usize);
        // Second destroy is a no-op
        expect_eq(st.destroy(), false);
        expect_eq(st.destroy_calls, 1 as usize);

        st.reset();
        expect_eq(st.isDestroyed(), false);
        expect_eq(st.destroyWithError(500), true);
        expect_eq(st.err_code, 500 as i32);
        expect_eq(st.destroyWithError(501), false);
        expect_eq(st.err_code, 500 as i32);
    });

    test("8. @cap/destroy — destroy, isDestroyed, destroyAll, destroyAllStates & predicates", () => {
        let streams: [3]DestroyableStream = [
            DestroyableStream.generic(),
            DestroyableStream.readFile(8, true),
            DestroyableStream.zlib(),
        ];
        expect_eq(isAnyDestroyed(streams[0..3]), false);
        expect_eq(areAllDestroyed(streams[0..3]), false);

        // Destroy first stream individually via top-level destroy()
        expect_eq(destroy(*streams[0]), DESTROY_ACTION_DESTROYED);
        expect_eq(isDestroyed(streams[0]), true);
        expect_eq(isAnyDestroyed(streams[0..3]), true);
        expect_eq(areAllDestroyed(streams[0..3]), false);

        // destroyAll skips already-destroyed stream[0], transitions remaining 2
        expect_eq(destroyAll(streams[0..3]), 2 as usize);
        expect_eq(areAllDestroyed(streams[0..3]), true);
        expect_eq(destroyAll(streams[0..3]), 0 as usize);

        let states: [2]DestroyState = [DestroyState.new(1), DestroyState.new(2)];
        expect_eq(destroyAllStates(states[0..2]), 2 as usize);
        expect_eq(destroyAllStates(states[0..2]), 0 as usize);
    });
}
