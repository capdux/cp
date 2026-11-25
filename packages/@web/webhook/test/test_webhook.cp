// Unit & integration tests for @web/webhook
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    WebhookConfig,
    webhookSignHex,
    webhookVerifySignature,
    setWebhookTestClock,
    resetWebhookStore,
    whSeenCount,
    registerWebhook,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn testStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn pingHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"pong\":true}");
}

pub fn main() {
    // 1. Known HMAC-SHA256 test vector
    let hex_buf: [64]u8 = [0; 64];
    const hex_len = webhookSignHex("key", "The quick brown fox jumps over the lazy dog", hex_buf[0..]) orelse 0;
    assertTrue(hex_len == 64, "webhookSignHex returns 64 bytes");
    assertTrue(
        testStrEql(hex_buf[0..64], "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8"),
        "HMAC-SHA256 matches RFC test vector",
    );

    // 2. Signature verification: sha256= prefix, bare hex, tampered payload, wrong secret
    assertTrue(
        webhookVerifySignature(
            "key",
            "The quick brown fox jumps over the lazy dog",
            "sha256=f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
        ),
        "verify with sha256= prefix",
    );
    assertTrue(
        webhookVerifySignature(
            "key",
            "The quick brown fox jumps over the lazy dog",
            "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
        ),
        "verify with bare hex",
    );
    assertTrue(
        !webhookVerifySignature(
            "wrong_key",
            "The quick brown fox jumps over the lazy dog",
            "sha256=f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
        ),
        "reject wrong secret",
    );
    assertTrue(
        !webhookVerifySignature(
            "key",
            "tampered payload",
            "sha256=f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
        ),
        "reject tampered payload",
    );
    assertTrue(
        !webhookVerifySignature("key", "abc", "sha256=short"),
        "reject malformed short signature",
    );

    // 3. App integration: signature, timestamp drift, idempotency deduplication
    resetWebhookStore();
    setWebhookTestClock(1700000000);

    let app = App.init();
    * = registerWebhook(*app, WebhookConfig{
        secret: "whsec_test_secret_123",
        endpoint_path: "/webhooks",
        max_skew_secs: 300,
    });
    * = app.get("/ping", pingHandler);

    // Non-webhook route passes through untouched
    const r_ping = app.dispatch(Request.init(AppMethod.GET, "/ping"));
    assertTrue(r_ping.status == 200, "GET /ping passes through");

    const payload: string = "{\"event\":\"payment.succeeded\",\"amount\":4200}";
    let sig_hex: [64]u8 = [0; 64];
    const sig_n = webhookSignHex("whsec_test_secret_123", payload, sig_hex[0..]) orelse 0;
    assertTrue(sig_n == 64, "signed webhook payload");

    let prefixed_sig: [71]u8 = [0; 71];
    prefixed_sig[0] = 's';
    prefixed_sig[1] = 'h';
    prefixed_sig[2] = 'a';
    prefixed_sig[3] = '2';
    prefixed_sig[4] = '5';
    prefixed_sig[5] = '6';
    prefixed_sig[6] = '=';
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        prefixed_sig[7 + i] = sig_hex[i];
    }

    // Missing signature -> 401
    let req_nosig = Request.init(AppMethod.POST, "/webhooks");
    * = req_nosig.withBody(payload);
    assertTrue(app.dispatch(req_nosig).status == 401, "missing sig -> 401");

    // Invalid signature -> 401
    let req_badsig = Request.init(AppMethod.POST, "/webhooks");
    * = req_badsig
        .withHeader("X-Hub-Signature-256", "sha256=0000000000000000000000000000000000000000000000000000000000000000")
        .withBody(payload);
    assertTrue(app.dispatch(req_badsig).status == 401, "invalid sig -> 401");

    // Expired timestamp (> 300s drift) -> 401
    let req_old = Request.init(AppMethod.POST, "/webhooks");
    * = req_old
        .withHeader("X-Hub-Signature-256", prefixed_sig[0..71])
        .withHeader("X-Webhook-Timestamp", "1699999000")
        .withHeader("X-Webhook-Id", "evt_001")
        .withBody(payload);
    assertTrue(app.dispatch(req_old).status == 401, "expired timestamp -> 401");
    assertTrue(whSeenCount() == 0, "expired event not recorded in store");

    // Valid event -> 200 {"received":true}
    let req_ok = Request.init(AppMethod.POST, "/webhooks");
    * = req_ok
        .withHeader("X-Hub-Signature-256", prefixed_sig[0..71])
        .withHeader("X-Webhook-Timestamp", "1699999950")
        .withHeader("X-Webhook-Id", "evt_001")
        .withBody(payload);
    const r_ok = app.dispatch(req_ok);
    assertTrue(r_ok.status == 200, "valid webhook -> 200");
    assertTrue(testStrEql(r_ok.body, "{\"received\":true}"), "valid webhook body");
    assertTrue(whSeenCount() == 1, "event recorded in store");

    // Duplicate event ID -> 200 {"duplicate":true}
    let req_dup = Request.init(AppMethod.POST, "/webhooks");
    * = req_dup
        .withHeader("X-Hub-Signature-256", sig_hex[0..64])
        .withHeader("X-Webhook-Timestamp", "1700000010")
        .withHeader("X-Webhook-Id", "evt_001")
        .withBody(payload);
    const r_dup = app.dispatch(req_dup);
    assertTrue(r_dup.status == 200, "duplicate webhook -> 200");
    assertTrue(testStrEql(r_dup.body, "{\"duplicate\":true}"), "duplicate webhook body");

    // New event ID -> 200 {"received":true}
    let req_new = Request.init(AppMethod.POST, "/webhooks");
    * = req_new
        .withHeader("X-Hub-Signature-256", prefixed_sig[0..71])
        .withHeader("X-Webhook-Timestamp", "1700000020")
        .withHeader("X-Webhook-Id", "evt_002")
        .withBody(payload);
    const r_new = app.dispatch(req_new);
    assertTrue(r_new.status == 200 && testStrEql(r_new.body, "{\"received\":true}"), "new event accepted");
    assertTrue(whSeenCount() == 2, "2 events in store");

    // Reset store allows evt_001 again
    resetWebhookStore();
    assertTrue(whSeenCount() == 0, "store cleared");
    const r_after_reset = app.dispatch(req_ok);
    assertTrue(r_after_reset.status == 200 && testStrEql(r_after_reset.body, "{\"received\":true}"), "evt_001 accepted after reset");

    setWebhookTestClock(0);
    std.debug.print("PASS: @web/webhook all tests passed\n", .{});
}
