// cap/test/testCapSseParse.cp
// Comprehensive unit tests for @cap/sse-parse

import { test, expect_eq } from "cap:test";
import {
    SseParser,
} from "../../packages/eventsource-parser/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn feedAll(p: *SseParser, input: []const u8): i32 {
    let i: usize = 0;
    let last: i32 = 0 as i32;
    while (i < input.len) {
        last = p.*.pushByte(input[i]);
        if (last != (0 as i32)) {
            return last;
        }
        i += 1;
    }
    return last;
}

fn main() {
    test("1. @cap/sse-parse — OpenAI-style data event with default 'message' type", () => {
        let p = SseParser.init();
        const stream = "data: {\"delta\":\"hi\"}\n\n";
        const rc = feedAll(&p, stream);
        expect_eq(rc, 1 as i32);
        expect_eq(sliceEq(p.eventType(), "message"), true);
        expect_eq(sliceEq(p.eventData(), "{\"delta\":\"hi\"}"), true);
        expect_eq(p.retryMs(), -1 as i64);
        p.consumeEvent();
        expect_eq(p.eventData().len, 0 as usize);
    });

    test("2. @cap/sse-parse — Custom event type & multi-line data joined by LF", () => {
        let p = SseParser.init();
        const stream = "event: ping\ndata: line1\ndata: line2\n\n";
        const rc = feedAll(&p, stream);
        expect_eq(rc, 1 as i32);
        expect_eq(sliceEq(p.eventType(), "ping"), true);
        expect_eq(sliceEq(p.eventData(), "line1\nline2"), true);
        p.consumeEvent();
        expect_eq(sliceEq(p.eventType(), "message"), true);
    });

    test("3. @cap/sse-parse — Comment heartbeat ignored & single leading space stripped", () => {
        let p = SseParser.init();
        const stream = ":keep-alive\n: another comment\ndata:  two\n\n";
        const rc = feedAll(&p, stream);
        expect_eq(rc, 1 as i32);
        expect_eq(sliceEq(p.eventData(), " two"), true);
    });

    test("4. @cap/sse-parse — Persistent lastEventId across events & retry parsing", () => {
        let p = SseParser.init();
        const ev1 = "id: evt-42\nretry: 3000\ndata: first\n\n";
        expect_eq(feedAll(&p, ev1), 1 as i32);
        expect_eq(sliceEq(p.lastEventId(), "evt-42"), true);
        expect_eq(p.retryMs(), 3000 as i64);
        expect_eq(sliceEq(p.eventData(), "first"), true);
        p.consumeEvent();

        // Second event omits id:, lastEventId must persist.
        const ev2 = "data: second\n\n";
        expect_eq(feedAll(&p, ev2), 1 as i32);
        expect_eq(sliceEq(p.lastEventId(), "evt-42"), true);
        expect_eq(sliceEq(p.eventData(), "second"), true);

        p.reset();
        expect_eq(p.lastEventId().len, 0 as usize);
        expect_eq(p.retryMs(), -1 as i64);
    });

    test("5. @cap/sse-parse — CRLF line endings compatibility", () => {
        let p = SseParser.init();
        const stream = "event: chunk\r\ndata: hello\r\ndata: world\r\n\r\n";
        const rc = feedAll(&p, stream);
        expect_eq(rc, 1 as i32);
        expect_eq(sliceEq(p.eventType(), "chunk"), true);
        expect_eq(sliceEq(p.eventData(), "hello\nworld"), true);
    });

    test("6. @cap/sse-parse — Empty blocks do not dispatch & line buffer overflow returns -1", () => {
        let p = SseParser.init();
        expect_eq(feedAll(&p, "\n\n:ping\n\nevent: stray\n\n"), 0 as i32);
        // After empty block, stray event name must be cleared.
        expect_eq(sliceEq(p.eventType(), "message"), true);

        // Overflow line_buf (> 512 bytes without newline).
        let idx: usize = 0;
        let overflowRc: i32 = 0 as i32;
        while (idx < 520) {
            overflowRc = p.pushByte('a' as u8);
            if (overflowRc == (-1 as i32)) {
                break;
            }
            idx += 1;
        }
        expect_eq(overflowRc, -1 as i32);
    });
}
