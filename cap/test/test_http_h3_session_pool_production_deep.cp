import { test, expect, expect_eq } from "cap:test";
import {
    H3_POOL_MAX_SESSIONS,
    H3_SESSION_MAX_STREAMS,
    H3PoolError,
    H3ConnectOutcomeKind,
    H3ConnectOutcome,
    H3PendingRequest,
    H3PooledSession,
    H3SessionPool,
} from "cap:http";

test("http h3 session_pool: coalescing by case-insensitive host, port, and TLS verify policy", fn()! {
    expect_eq(H3_POOL_MAX_SESSIONS, 32);
    expect_eq(H3_SESSION_MAX_STREAMS, 32);

    let pool = H3SessionPool.init();
    const c1 = pool.connect("api.example.com", 443, true, 101, 4)?;
    expect(c1.kind == .OpenedNew);
    expect_eq(pool.count(), 1);
    expect_eq(pool.live_sessions, 1);

    // Same host (different ASCII case), same port, same reject_unauthorized -> ReusedExisting
    const c2 = pool.connect("API.Example.COM", 443, true, 102, 4)?;
    expect(c2.kind == .ReusedExisting);
    expect_eq(c2.session_id, c1.session_id);
    expect_eq(pool.count(), 1);

    // Different TLS reject_unauthorized policy -> MUST NOT coalesce!
    const c3 = pool.connect("api.example.com", 443, false, 103, 4)?;
    expect(c3.kind == .OpenedNew);
    expect(c3.session_id != c1.session_id);
    expect_eq(pool.count(), 2);

    // Different port -> MUST NOT coalesce!
    const c4 = pool.connect("api.example.com", 8443, true, 104, 4)?;
    expect(c4.kind == .OpenedNew);
    expect_eq(pool.count(), 3);
});

test("http h3 session_pool: headroom overflow opens new session and abort restores headroom", fn()! {
    let pool = H3SessionPool.init();
    const c1 = pool.connect("cdn.cp.dev", 443, true, 1, 2)?;
    const c2 = pool.connect("cdn.cp.dev", 443, true, 2, 2)?;
    expect(c1.kind == .OpenedNew);
    expect(c2.kind == .ReusedExisting);
    expect_eq(c1.session_id, c2.session_id);

    // Session 1 now at capacity (2/2) -> 3rd request opens session 2
    const c3 = pool.connect("cdn.cp.dev", 443, true, 3, 2)?;
    expect(c3.kind == .OpenedNew);
    expect(c3.session_id != c1.session_id);
    expect_eq(pool.count(), 2);

    // Aborting http_id=1 frees a slot on session 1 -> next connect reuses session 1
    expect(pool.abortByHttpId(1));
    const c4 = pool.connect("cdn.cp.dev", 443, true, 4, 2)?;
    expect(c4.kind == .ReusedExisting);
    expect_eq(c4.session_id, c1.session_id);
});

test("http h3 session_pool: O(1) swap-remove unregister preserves remaining sessions", fn()! {
    let pool = H3SessionPool.init();
    const s1 = pool.connect("a.example", 443, true, 10, 4)?;
    const s2 = pool.connect("b.example", 443, true, 20, 4)?;
    const s3 = pool.connect("c.example", 443, true, 30, 4)?;
    expect_eq(pool.count(), 3);

    // Unregister middle session s2 -> s3 swapped into slot 1
    expect(pool.unregister(s2.session_id));
    expect_eq(pool.count(), 2);
    expect_eq(pool.live_sessions, 2);
    expect(pool.findSessionIndex(s2.session_id) == null);
    expect(pool.findSessionIndex(s1.session_id) != null);
    expect(pool.findSessionIndex(s3.session_id) != null);

    // Double unregister returns false
    expect(!pool.unregister(s2.session_id));
    expect(!pool.unregister(9999));
});

test("http h3 session_pool: streamBodyByHttpId and abortByHttpId routing across sessions", fn()! {
    let pool = H3SessionPool.init();
    const s1 = pool.connect("alpha.test", 443, true, 501, 4)?;
    const s2 = pool.connect("beta.test", 443, true, 502, 4)?;

    expect(pool.streamBodyByHttpId(502, true));
    const idx2 = pool.findSessionIndex(s2.session_id) ?? return err.TestFailed;
    const req_slot = pool.sessions[idx2].findRequest(502) ?? return err.TestFailed;
    expect(pool.sessions[idx2].pending[req_slot].body_ended);

    expect(!pool.streamBodyByHttpId(999, true));
    expect(pool.abortByHttpId(501));
    expect(!pool.abortByHttpId(501)); // already detached

    const idx1 = pool.findSessionIndex(s1.session_id) ?? return err.TestFailed;
    expect_eq(pool.sessions[idx1].activeCount(), 0);
});

test("http h3 session_pool: failSession drains pending requests and unregisters session", fn()! {
    let pool = H3SessionPool.init();
    const c1 = pool.connect("edge.test", 443, true, 11, 8)?;
    * = pool.connect("edge.test", 443, true, 12, 8)?;
    * = pool.connect("edge.test", 443, true, 13, 8)?;
    expect_eq(pool.count(), 1);

    const failed = pool.failSession(c1.session_id);
    expect_eq(failed, 3);
    expect_eq(pool.count(), 0);
    expect_eq(pool.live_sessions, 0);
    expect_eq(pool.failSession(c1.session_id), 0);
});

test("http h3 session_pool: InvalidHost, InvalidHttpId, and PoolFull guards", fn()! {
    let pool = H3SessionPool.init();
    let errs: usize = 0;

    if (_ := pool.connect("", 443, true, 1, 4)) {} else { errs += 1; }
    if (_ := pool.connect("ok.test", 443, true, 0, 4)) {} else { errs += 1; }

    // Fill all 32 session slots (each with max_streams=1 so every connect opens a new session)
    let i: u32 = 1;
    while (i <= 32) {
        * = pool.connect("pool.test", 443, true, i, 1)?;
        i += 1;
    }
    expect_eq(pool.count(), 32);

    // 33rd session must fail with PoolFull
    if (_ := pool.connect("pool.test", 443, true, 33, 1)) {} else { errs += 1; }
    expect_eq(errs, 3);
});
