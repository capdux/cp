import { test, expect_eq } from "cap:test";
import {
    DESTROY_ACTION_NONE, DESTROY_ACTION_DESTROYED,
    DESTROY_ACTION_DEFERRED_CLOSE, DESTROY_ACTION_ALREADY_DESTROYED,
    DestroyableStream, destroyStream, destroyWithError
} from "../../packages/@cap/destroy/src/index.cp";
import { REASON_ERROR, ListenerSlot } from "../../packages/@cap/on-finished/src/index.cp";

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
}
