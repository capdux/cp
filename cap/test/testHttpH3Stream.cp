import { test, expect } from "cap:test";
import {
    H3_REQ_STREAM_MAX_HEADERS,
    H3_REQ_STREAM_MAX_BODY_BUF,
    H3RequestStreamError,
    H3StreamPhase,
    H3RequestStream,
} from "cap:http";

test("H3RequestStream buffered request/response full lifecycle", () => {
    let s = H3RequestStream.init(4, 101, false);
    expect(s.phase == H3StreamPhase.Open).toBe(true);
    expect(s.isTerminal()).toBe(false);

    const req_len = s.setPendingBody("hello-world")?;
    expect(req_len).toBe(11);
    expect(s.phase == H3StreamPhase.RequestBodySending).toBe(true);
    expect(s.pendingBodyRemaining()).toBe("hello-world");

    const rem1 = s.advancePendingBody(6)?;
    expect(rem1).toBe(5);
    expect(s.request_body_done).toBe(false);
    expect(s.pendingBodyRemaining()).toBe("world");

    const rem2 = s.advancePendingBody(5)?;
    expect(rem2).toBe(0);
    expect(s.request_body_done).toBe(true);

    * = s.onStatusCode(200)?;
    * = s.addDecodedHeader("Content-Type", "application/json")?;
    * = s.addDecodedHeader("X-Trace-Id", "abc123")?;
    expect(s.header_count).toBe(2);

    if (ct := s.findHeader("content-type")) {
        expect(ct).toBe("application/json");
    } else {
        expect(false).toBe(true);
    }
    expect(s.findHeader("X-Missing") == null).toBe(true);

    expect(s.deliverHeaders()?).toBe(true);
    expect(s.headers_delivered).toBe(true);
    expect(s.phase == H3StreamPhase.HeadersReceived).toBe(true);

    * = s.appendResponseBody("{\"ok\":")?;
    * = s.appendResponseBody("true}")?;
    expect(s.phase == H3StreamPhase.ReceivingBody).toBe(true);
    expect(s.responseBody()).toBe("{\"ok\":true}");
    expect(s.total_body_bytes).toBe(11);

    expect(s.complete()?).toBe(true);
    expect(s.phase == H3StreamPhase.Completed).toBe(true);
    expect(s.isTerminal()).toBe(true);
});

test("H3RequestStream 1xx informational response followed by 200 final headers", () => {
    let s = H3RequestStream.init(8, 102, false);
    * = s.markRequestBodyDone()?;
    expect(s.request_body_done).toBe(true);

    // 103 Early Hints
    * = s.onStatusCode(103)?;
    expect(s.isInformational()).toBe(true);
    * = s.addDecodedHeader("Link", "</app.css>; rel=preload")?;
    expect(s.deliverHeaders()?).toBe(false);
    expect(s.headers_delivered).toBe(false);

    // Final 200 resets informational headers
    * = s.onStatusCode(200)?;
    expect(s.isInformational()).toBe(false);
    expect(s.header_count).toBe(0);
    * = s.addDecodedHeader("Server", "cp-h3")?;
    expect(s.deliverHeaders()?).toBe(true);
    expect(s.header_count).toBe(1);
});

test("H3RequestStream streaming mode bypasses inline buffer cap", () => {
    let s = H3RequestStream.init(12, 103, true);
    * = s.onStatusCode(200)?;
    * = s.deliverHeaders()?;

    let chunk: [1024]u8 = [0x41; 1024];
    let i: usize = 0;
    while (i < 6) {
        * = s.appendResponseBody(chunk[0..])?;
        i += 1;
    }
    expect(s.body_len).toBe(0);
    expect(s.total_body_bytes).toBe(6144);
    expect(s.complete()?).toBe(true);
});

test("H3RequestStream idempotent abort clears pending state and blocks further transitions", () => {
    let s = H3RequestStream.init(16, 104, false);
    * = s.setPendingBody("pending-payload")?;
    s.abort(0x010C); // H3_REQUEST_CANCELLED
    expect(s.phase == H3StreamPhase.Aborted).toBe(true);
    expect(s.isTerminal()).toBe(true);
    expect(s.abort_error_code).toBe(0x010C);
    expect(s.pendingBodyRemaining().len).toBe(0);

    // Second abort is idempotent and preserves original error code
    s.abort(0x0100);
    expect(s.abort_error_code).toBe(0x010C);

    let got_aborted = false;
    * = s.onStatusCode(200) catch |e| {
        got_aborted = (e == H3RequestStreamError.StreamAborted);
        0
    };
    expect(got_aborted).toBe(true);
});

test("H3RequestStream error guards: InvalidStatusCode, TooManyHeaders, BodyBufferOverflow, InvalidStateTransition", () => {
    let s = H3RequestStream.init(20, 105, false);

    // InvalidStatusCode (99 / 600)
    let got_bad_status = false;
    * = s.onStatusCode(99) catch |e| {
        got_bad_status = (e == H3RequestStreamError.InvalidStatusCode);
        0
    };
    expect(got_bad_status).toBe(true);

    // appendResponseBody before deliverHeaders -> InvalidStateTransition
    let got_trans = false;
    * = s.appendResponseBody("early") catch |e| {
        got_trans = (e == H3RequestStreamError.InvalidStateTransition);
        0
    };
    expect(got_trans).toBe(true);

    // TooManyHeaders (> 32)
    * = s.onStatusCode(200)?;
    let i: usize = 0;
    while (i < H3_REQ_STREAM_MAX_HEADERS) {
        * = s.addDecodedHeader("x-k", "v")?;
        i += 1;
    }
    let got_many = false;
    * = s.addDecodedHeader("x-over", "v") catch |e| {
        got_many = (e == H3RequestStreamError.TooManyHeaders);
        0
    };
    expect(got_many).toBe(true);

    // HeadersAlreadyDelivered
    * = s.deliverHeaders()?;
    let got_dup = false;
    * = s.deliverHeaders() catch |e| {
        got_dup = (e == H3RequestStreamError.HeadersAlreadyDelivered);
        false
    };
    expect(got_dup).toBe(true);

    // BodyBufferOverflow (> 4096 in buffered mode)
    let big: [2048]u8 = [0x42; 2048];
    * = s.appendResponseBody(big[0..])?;
    * = s.appendResponseBody(big[0..])?;
    expect(s.body_len).toBe(H3_REQ_STREAM_MAX_BODY_BUF);
    let got_ovf = false;
    * = s.appendResponseBody("x") catch |e| {
        got_ovf = (e == H3RequestStreamError.BodyBufferOverflow);
        0
    };
    expect(got_ovf).toBe(true);
});
