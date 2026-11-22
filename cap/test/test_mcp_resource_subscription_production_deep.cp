// Production Deep Test Suite for cap:mcp Resource Subscription & Fan-Out Notifier (lib/mcp/resource_subscription.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    MCP_RSUB_MAX_SUBSCRIPTIONS,
    validateRfc6570Template,
    matchRfc6570Template,
    McpSubStatus,
    McpPendingNotification,
    McpResourceSubHub,
} from "cap:mcp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: RFC 6570 template validation & zero-copy parameter extraction
// ============================================================================
test("mcp_rsub: RFC 6570 template validation & zero-copy param extraction", fn() {
    const tpl1 = "db://{tenant}/tables/{table}";
    const uri1 = "db://acme-prod/tables/orders_2026";
    expect_true(validateRfc6570Template(tpl1));

    const m1 = matchRfc6570Template(tpl1, uri1);
    expect_true(m1.matched);
    expect_eq(m1.param_count, 2 as usize);
    const p0 = m1.params[0];
    const p1 = m1.params[1];
    expect_true(strEq(tpl1[p0.name_start as usize .. (p0.name_start + p0.name_len) as usize], "tenant"));
    expect_true(strEq(uri1[p0.val_start as usize .. (p0.val_start + p0.val_len) as usize], "acme-prod"));
    expect_true(strEq(tpl1[p1.name_start as usize .. (p1.name_start + p1.name_len) as usize], "table"));
    expect_true(strEq(uri1[p1.val_start as usize .. (p1.val_start + p1.val_len) as usize], "orders_2026"));

    // Single-segment {var} must NOT cross '/'
    const m_slash_reject = matchRfc6570Template("file:///{repo}/{file}", "file:///cp/lib/mcp/index.cp");
    expect_true(!m_slash_reject.matched);

    // Reserved expansion {+path} DOES cross '/'
    const tpl2 = "file:///{repo}/{+path}";
    const uri2 = "file:///cp/lib/mcp/index.cp";
    expect_true(validateRfc6570Template(tpl2));
    const m2 = matchRfc6570Template(tpl2, uri2);
    expect_true(m2.matched);
    expect_eq(m2.param_count, 2 as usize);
    const r_p0 = m2.params[0];
    const r_p1 = m2.params[1];
    expect_true(strEq(uri2[r_p0.val_start as usize .. (r_p0.val_start + r_p0.val_len) as usize], "cp"));
    expect_true(strEq(uri2[r_p1.val_start as usize .. (r_p1.val_start + r_p1.val_len) as usize], "lib/mcp/index.cp"));
});

// ============================================================================
// Test 2: Subscribe / unsubscribe exact & template patterns with deduplication
// ============================================================================
test("mcp_rsub: subscribe/unsubscribe exact & template patterns with dedup", fn() {
    let hub = McpResourceSubHub.init(8) ?? return;

    expect_eq(hub.subscribe(1001, "file:///workspace/README.md"), McpSubStatus.ok);
    expect_eq(hub.subscribe(1001, "file:///workspace/README.md"), McpSubStatus.already_subscribed);
    expect_eq(hub.subscribe(1001, "db://{tenant}/tables/{table}"), McpSubStatus.ok);
    expect_eq(hub.subscribe(1002, "file:///workspace/README.md"), McpSubStatus.ok);

    expect_eq(hub.sessionSubscriptionCount(1001), 2 as usize);
    expect_eq(hub.sessionSubscriptionCount(1002), 1 as usize);
    expect_eq(hub.stats().active_subscriptions, 3 as usize);

    // Unsubscribe exact entry & verify second unsubscribe returns not_found
    expect_eq(hub.unsubscribe(1001, "file:///workspace/README.md"), McpSubStatus.ok);
    expect_eq(hub.unsubscribe(1001, "file:///workspace/README.md"), McpSubStatus.not_found);
    expect_eq(hub.sessionSubscriptionCount(1001), 1 as usize);
    expect_eq(hub.stats().active_subscriptions, 2 as usize);
});

// ============================================================================
// Test 3: Fan-out notification, multi-pattern subscriber dedup & etag_version
// ============================================================================
test("mcp_rsub: notifyResourceUpdated fan-out, subscriber dedup & monotonic etag_version", fn() {
    let hub = McpResourceSubHub.init(8) ?? return;

    // Session 10 subscribes to BOTH exact URI and matching template -> must be notified ONCE!
    expect_eq(hub.subscribe(10, "db://acme/tables/users"), McpSubStatus.ok);
    expect_eq(hub.subscribe(10, "db://{tenant}/tables/{table}"), McpSubStatus.ok);
    // Session 20 subscribes to template
    expect_eq(hub.subscribe(20, "db://{tenant}/tables/{table}"), McpSubStatus.ok);
    // Session 30 subscribes to unrelated resource
    expect_eq(hub.subscribe(30, "file:///other/{name}"), McpSubStatus.ok);

    let targets: [8]u64 = [0; 8];
    let targets_s: []u64 = targets[0..];
    const r1 = hub.notifyResourceUpdated("db://acme/tables/users", *targets_s);
    expect_eq(r1.etag_version, 1 as u64);
    expect_eq(r1.matched_subscribers, 2 as usize);
    expect_eq(r1.enqueued_notifications, 2 as usize);
    expect_eq(r1.coalesced_notifications, 0 as usize);
    expect_eq(targets[0], 10 as u64);
    expect_eq(targets[1], 20 as u64);

    // Unmatched resource still increments etag_version cleanly with 0 subscribers
    const r2 = hub.notifyResourceUpdated("s3://bucket/key.bin", *targets_s);
    expect_eq(r2.etag_version, 2 as u64);
    expect_eq(r2.matched_subscribers, 0 as usize);
    expect_eq(r2.enqueued_notifications, 0 as usize);
});

// ============================================================================
// Test 4: Dirty notification debouncing/coalescing & batched draining
// ============================================================================
test("mcp_rsub: debouncing/coalescing duplicate updates & batched drainPendingNotifications", fn() {
    let hub = McpResourceSubHub.init(8) ?? return;
    expect_eq(hub.subscribe(42, "file:///{repo}/{+path}"), McpSubStatus.ok);
    expect_eq(hub.subscribe(99, "file:///{repo}/{+path}"), McpSubStatus.ok);

    let targets: [4]u64 = [0; 4];
    let targets_s: []u64 = targets[0..];

    // 3 rapid updates to the SAME URI -> coalesced into 1 pending notification per subscriber!
    const r1 = hub.notifyResourceUpdated("file:///cp/src/main.cp", *targets_s);
    const r2 = hub.notifyResourceUpdated("file:///cp/src/main.cp", *targets_s);
    const r3 = hub.notifyResourceUpdated("file:///cp/src/main.cp", *targets_s);
    expect_eq(r1.enqueued_notifications, 2 as usize);
    expect_eq(r2.coalesced_notifications, 2 as usize);
    expect_eq(r3.coalesced_notifications, 2 as usize);
    expect_eq(r3.etag_version, 3 as u64);

    // Distinct URI creates separate pending notification per subscriber
    * = hub.notifyResourceUpdated("file:///cp/src/lib.cp", *targets_s);
    expect_eq(hub.stats().pending_notifications, 4 as usize); // 2 URIs x 2 subscribers

    // Batched drain for session 42 with buffer size 1 -> drains 1 at a time without losing the 2nd
    let batch: [1]McpPendingNotification = [McpPendingNotification{}; 1];
    let batch_s: []McpPendingNotification = batch[0..];
    const d1 = hub.drainPendingNotifications(42, *batch_s);
    expect_eq(d1, 1 as usize);
    expect_eq(batch[0].subscriber_id, 42 as u64);
    if (strEq(batch[0].uriSlice(), "file:///cp/src/main.cp")) {
        expect_eq(batch[0].etag_version, 3 as u64);
        expect_eq(batch[0].coalesced_count, 3 as u32);
    } else {
        expect_eq(batch[0].etag_version, 4 as u64);
        expect_eq(batch[0].coalesced_count, 1 as u32);
    }

    const d2 = hub.drainPendingNotifications(42, *batch_s);
    expect_eq(d2, 1 as usize);
    expect_eq(hub.drainPendingNotifications(42, *batch_s), 0 as usize);
    // Session 99's 2 pending notifications remain untouched
    expect_eq(hub.stats().pending_notifications, 2 as usize);

    // Global drain (subscriber_id == 0) drains remaining 2 notifications
    let all_buf: [4]McpPendingNotification = [McpPendingNotification{}; 4];
    let all_s: []McpPendingNotification = all_buf[0..];
    expect_eq(hub.drainPendingNotifications(0, *all_s), 2 as usize);
    expect_eq(hub.stats().pending_notifications, 0 as usize);
    expect_eq(hub.stats().total_coalesced, 4 as u64);
});

// ============================================================================
// Test 5: unsubscribeAllForSession clears subscriptions & pending notifications
// ============================================================================
test("mcp_rsub: unsubscribeAllForSession cleans subscriptions and pending notifications", fn() {
    let hub = McpResourceSubHub.init(8) ?? return;

    expect_eq(hub.subscribe(7, "file:///a/1"), McpSubStatus.ok);
    expect_eq(hub.subscribe(7, "file:///a/{id}"), McpSubStatus.ok);
    expect_eq(hub.subscribe(8, "file:///a/1"), McpSubStatus.ok);

    let targets: [4]u64 = [0; 4];
    let targets_s: []u64 = targets[0..];
    * = hub.notifyResourceUpdated("file:///a/1", *targets_s);
    expect_eq(hub.stats().pending_notifications, 2 as usize);

    // Disconnect session 7 -> removes its 2 subscriptions and its 1 pending notification
    const removed = hub.unsubscribeAllForSession(7);
    expect_eq(removed, 2 as usize);
    expect_eq(hub.sessionSubscriptionCount(7), 0 as usize);
    expect_eq(hub.stats().active_subscriptions, 1 as usize);
    expect_eq(hub.stats().pending_notifications, 1 as usize);

    // Unknown session cleanup is a safe no-op
    expect_eq(hub.unsubscribeAllForSession(9999), 0 as usize);
    expect_eq(hub.unsubscribeAllForSession(0), 0 as usize);
});

// ============================================================================
// Test 6: Per-session quota & global capacity exhaustion guards
// ============================================================================
test("mcp_rsub: per-session quota & global capacity exhaustion guards", fn() {
    let hub_small = McpResourceSubHub.init(2) ?? return;
    expect_eq(hub_small.subscribe(1, "file:///r/1"), McpSubStatus.ok);
    expect_eq(hub_small.subscribe(1, "file:///r/2"), McpSubStatus.ok);
    expect_eq(hub_small.subscribe(1, "file:///r/3"), McpSubStatus.session_limit_reached);

    // Global capacity test: allow max_per_session = MCP_RSUB_MAX_SUBSCRIPTIONS
    let hub_full = McpResourceSubHub.init(MCP_RSUB_MAX_SUBSCRIPTIONS) ?? return;
    let i: usize = 0;
    while (i < MCP_RSUB_MAX_SUBSCRIPTIONS) : (i += 1) {
        let uri_buf: [16]u8 = [0; 16];
        uri_buf[0] = 102; // 'f'
        uri_buf[1] = 58;  // ':'
        uri_buf[2] = 47;  // '/'
        uri_buf[3] = (48 + (i / 100)) as u8;
        uri_buf[4] = (48 + ((i / 10) % 10)) as u8;
        uri_buf[5] = (48 + (i % 10)) as u8;
        expect_eq(hub_full.subscribe((i + 1) as u64, uri_buf[0..6]), McpSubStatus.ok);
    }
    expect_eq(hub_full.subscribe(9999, "f:/999"), McpSubStatus.capacity_full);
});

// ============================================================================
// Test 7: Strict boundary guards (empty URI, malformed braces, invalid session)
// ============================================================================
test("mcp_rsub: strict boundary guards (empty URI, malformed braces, invalid session)", fn() {
    expect_true(McpResourceSubHub.init(0) == null);
    expect_true(McpResourceSubHub.init(MCP_RSUB_MAX_SUBSCRIPTIONS + 1) == null);

    // Malformed RFC 6570 templates
    expect_true(!validateRfc6570Template(""));
    expect_true(!validateRfc6570Template("file:///unclosed/{repo"));
    expect_true(!validateRfc6570Template("file:///stray/repo}"));
    expect_true(!validateRfc6570Template("file:///empty/{}"));
    expect_true(!validateRfc6570Template("file:///emptyplus/{+}"));
    expect_true(!validateRfc6570Template("file:///nested/{{a}}"));
    expect_true(!validateRfc6570Template("file:///adjacent/{a}{b}"));
    expect_true(!validateRfc6570Template("file:///has space/{a}"));

    let hub = McpResourceSubHub.init(8) ?? return;
    expect_eq(hub.subscribe(0, "file:///valid"), McpSubStatus.invalid_session);
    expect_eq(hub.subscribe(1, ""), McpSubStatus.invalid_uri);
    expect_eq(hub.subscribe(1, "file:///{bad"), McpSubStatus.invalid_uri);
    expect_eq(hub.unsubscribe(0, "file:///valid"), McpSubStatus.invalid_session);
    expect_eq(hub.unsubscribe(1, ""), McpSubStatus.invalid_uri);

    // Invalid concrete resource_uri on notify (empty, whitespace, or unexpanded template braces)
    let targets: [4]u64 = [0; 4];
    let targets_s: []u64 = targets[0..];
    expect_eq(hub.notifyResourceUpdated("", *targets_s).etag_version, 0 as u64);
    expect_eq(hub.notifyResourceUpdated("file:///{unexpanded}", *targets_s).etag_version, 0 as u64);
    expect_eq(hub.notifyResourceUpdated("file:///bad space", *targets_s).etag_version, 0 as u64);
});
