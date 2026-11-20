// Source: cp/cap/test/test_webhook_production_deep.cp
// Deep Production Verification Suite for cap:webhook & cap.webhook
// Covers:
//   1. StandardWebhook sign/verify, space-separated key rotation, tampering & replay window rejection
//   2. GitHubWebhook "sha256=..." sign/verify (incl. official test vector) & single-byte tampering rejection
//   3. StripeWebhook "t=...,v1=..." sign/verify, multi-v1 key rotation, malformed header & replay window checks
//   4. IdempotencyStore first-pass / duplicate block / TTL expiration re-allow / purgeExpired
//   5. `import cap from "cap"; cap.webhook` automatic dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    WebhookError,
    StandardWebhook,
    GitHubWebhook,
    StripeWebhook,
    IdempotencyStore,
} from "cap:webhook";

fn testStandardWebhookAndRotation(): !void {
    const alloc = std.heap.page_allocator;
    const secret = "super-secret-signing-key";
    const old_secret = "legacy-rotated-key";
    const msg_id = "msg_2026_001";
    const ts: i64 = 1700000000;
    const body = "{\"event\":\"invoice.paid\",\"amount\":4900}";

    const sig = StandardWebhook.sign(alloc, secret, msg_id, ts, body)?;
    defer alloc.free(sig);

    expect_eq(std.mem.startsWith(u8, sig, "v1,"), true);
    expect_eq(sig.len, 47 as usize);

    // 1. Valid verification at exact timestamp and within tolerance window
    StandardWebhook.verify(secret, msg_id, ts, body, sig, ts, 300)?;
    StandardWebhook.verify(secret, msg_id, ts, body, sig, ts + 299, 300)?;
    StandardWebhook.verify(secret, msg_id, ts, body, sig, ts - 299, 300)?;

    // 2. Multi-signature space-separated key rotation
    const old_sig = StandardWebhook.sign(alloc, old_secret, msg_id, ts, body)?;
    defer alloc.free(old_sig);
    const rotated_header = std.fmt.allocPrint(alloc, "v2,ignoredsig {s} {s}", .{ old_sig, sig }) catch unreachable;
    defer alloc.free(rotated_header);
    StandardWebhook.verify(secret, msg_id, ts, body, rotated_header, ts + 10, 300)?;

    // 3. Tampered payload & wrong secret must fail with InvalidSignature
    let hit_tampered = false;
    StandardWebhook.verify(
        secret,
        msg_id,
        ts,
        "{\"event\":\"invoice.paid\",\"amount\":4901}",
        sig,
        ts,
        300,
    ) catch |e| {
        if (e == WebhookError.InvalidSignature) hit_tampered = true;
    };
    expect_eq(hit_tampered, true);

    let hit_wrong_secret = false;
    StandardWebhook.verify("wrong-secret", msg_id, ts, body, sig, ts, 300) catch |e| {
        if (e == WebhookError.InvalidSignature) hit_wrong_secret = true;
    };
    expect_eq(hit_wrong_secret, true);

    // 4. Replay window: expired and future timestamps
    let hit_expired = false;
    StandardWebhook.verify(secret, msg_id, ts, body, sig, ts + 301, 300) catch |e| {
        if (e == WebhookError.TimestampExpired) hit_expired = true;
    };
    expect_eq(hit_expired, true);

    let hit_future = false;
    StandardWebhook.verify(secret, msg_id, ts, body, sig, ts - 301, 300) catch |e| {
        if (e == WebhookError.TimestampFuture) hit_future = true;
    };
    expect_eq(hit_future, true);

    // 5. Malformed headers
    const bad_headers: [4]string = ["", "invalid-no-comma", "v1,", "v2,abc"];
    for (idx of 0..bad_headers.len) {
        let hit_malformed = false;
        StandardWebhook.verify(secret, msg_id, ts, body, bad_headers[idx], ts, 300) catch |e| {
            if (e == WebhookError.MalformedHeader) hit_malformed = true;
        };
        expect_eq(hit_malformed, true);
    }
}

fn testGitHubWebhookSha256(): !void {
    const alloc = std.heap.page_allocator;

    // Official GitHub webhook HMAC-SHA256 documentation vector
    const gh_vec_sig = GitHubWebhook.signSha256(alloc, "It's a Secret to Everybody", "Hello, World!")?;
    defer alloc.free(gh_vec_sig);
    expect_eq(
        std.mem.eql(u8, gh_vec_sig, "sha256=757107ea0eb2509fc211221cce984b8a37570b6d7586c22c46f4379c8b043e17"),
        true,
    );
    expect_eq(GitHubWebhook.verifySha256("It's a Secret to Everybody", "Hello, World!", gh_vec_sig), true);

    // Single-byte payload tampering rejection
    expect_eq(GitHubWebhook.verifySha256("It's a Secret to Everybody", "Hello, World?", gh_vec_sig), false);

    // Single-character signature tampering rejection
    let tampered_sig = alloc.dupe(u8, gh_vec_sig) catch unreachable;
    defer alloc.free(tampered_sig);
    tampered_sig[70] = if (tampered_sig[70] == '0') '1' else '0';
    expect_eq(GitHubWebhook.verifySha256("It's a Secret to Everybody", "Hello, World!", tampered_sig), false);

    // Wrong secret, wrong prefix, wrong length, and non-hex rejection
    expect_eq(GitHubWebhook.verifySha256("wrong-secret", "Hello, World!", gh_vec_sig), false);
    expect_eq(GitHubWebhook.verifySha256("It's a Secret to Everybody", "Hello, World!", "sha1=757107ea0eb2509fc211221cce984b8a37570b6d7586c22c46f4379c8b043e17"), false);
    expect_eq(GitHubWebhook.verifySha256("It's a Secret to Everybody", "Hello, World!", "sha256=short"), false);
    expect_eq(
        GitHubWebhook.verifySha256(
            "It's a Secret to Everybody",
            "Hello, World!",
            "sha256=zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
        ),
        false,
    );
}

fn testStripeWebhookHeaderAndReplay(): !void {
    const alloc = std.heap.page_allocator;
    const secret = "whsec_stripe_live_key";
    const ts: i64 = 1700001234;
    const payload = "{\"id\":\"evt_123\",\"type\":\"checkout.session.completed\"}";

    const hdr = StripeWebhook.signHeader(alloc, secret, ts, payload)?;
    defer alloc.free(hdr);
    expect_eq(std.mem.startsWith(u8, hdr, "t=1700001234,v1="), true);

    // Valid check
    StripeWebhook.verifyHeader(secret, payload, hdr, ts + 120, 300)?;

    // Extract v1=... slice and verify multi-v1 key rotation + auxiliary v0 field
    const v1_part = hdr["t=1700001234,".len ..];
    const rotated = std.fmt.allocPrint(
        alloc,
        "t=1700001234, v0=deadbeef, v1=0000000000000000000000000000000000000000000000000000000000000000, {s}",
        .{v1_part},
    ) catch unreachable;
    defer alloc.free(rotated);
    StripeWebhook.verifyHeader(secret, payload, rotated, ts + 50, 300)?;

    // Tampered payload rejected with InvalidSignature
    let hit_invalid = false;
    StripeWebhook.verifyHeader(secret, "{\"id\":\"evt_124\"}", hdr, ts, 300) catch |e| {
        if (e == WebhookError.InvalidSignature) hit_invalid = true;
    };
    expect_eq(hit_invalid, true);

    // Replay window: expired and future
    let hit_expired = false;
    StripeWebhook.verifyHeader(secret, payload, hdr, ts + 301, 300) catch |e| {
        if (e == WebhookError.TimestampExpired) hit_expired = true;
    };
    expect_eq(hit_expired, true);

    let hit_future = false;
    StripeWebhook.verifyHeader(secret, payload, hdr, ts - 301, 300) catch |e| {
        if (e == WebhookError.TimestampFuture) hit_future = true;
    };
    expect_eq(hit_future, true);

    // Malformed Stripe headers
    const bad_stripe: [5]string = [
        "",
        "v1=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "t=1700001234",
        "t=170_000,v1=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "t=1700001234,t=1700001235,v1=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    ];
    for (idx of 0..bad_stripe.len) {
        let hit_malformed = false;
        StripeWebhook.verifyHeader(secret, payload, bad_stripe[idx], ts, 300) catch |e| {
            if (e == WebhookError.MalformedHeader) hit_malformed = true;
        };
        expect_eq(hit_malformed, true);
    }
}

fn testIdempotencyStoreLifecycle(): !void {
    const alloc = std.heap.page_allocator;
    let store = IdempotencyStore.init(alloc, 5000);
    defer store.deinit();

    expect_eq(store.len(), 0 as usize);
    expect_eq(store.isSeen("evt_alpha", 1000), false);

    // 1. First delivery allowed; duplicate within TTL blocked
    expect_eq(store.checkAndRecord("evt_alpha", 1000), true);
    expect_eq(store.len(), 1 as usize);
    expect_eq(store.isSeen("evt_alpha", 1000), true);
    expect_eq(store.isSeen("evt_alpha", 5999), true);
    expect_eq(store.checkAndRecord("evt_alpha", 5999), false);

    // 2. At exact TTL expiration (now_ms >= 6000), isSeen is false and checkAndRecord re-allows
    expect_eq(store.isSeen("evt_alpha", 6000), false);
    expect_eq(store.checkAndRecord("evt_alpha", 6000), true);
    expect_eq(store.isSeen("evt_alpha", 10999), true);
    expect_eq(store.checkAndRecord("evt_alpha", 10999), false);

    // 3. Batch purgeExpired (>64 items to verify multi-batch zero-alloc sweep)
    let i: usize = 0;
    while (i < 70) : (i += 1) {
        const key = std.fmt.allocPrint(alloc, "evt_batch_{d}", .{i}) catch unreachable;
        defer alloc.free(key);
        expect_eq(store.checkAndRecordWithTtl(key, 1000, 10000), true);
    }
    expect_eq(store.len(), 71 as usize); // evt_alpha (expires 11000) + 70 batch (expires 11000)

    // Add 3 long-lived keys expiring at 20000
    expect_eq(store.checkAndRecordWithTtl("evt_live_1", 10000, 10000), true);
    expect_eq(store.checkAndRecordWithTtl("evt_live_2", 10000, 10000), true);
    expect_eq(store.checkAndRecordWithTtl("evt_live_3", 10000, 10000), true);
    expect_eq(store.len(), 74 as usize);

    const purged = store.purgeExpired(11000);
    expect_eq(purged, 71 as usize);
    expect_eq(store.len(), 3 as usize);
    expect_eq(store.isSeen("evt_live_1", 11000), true);
    expect_eq(store.isSeen("evt_alpha", 11000), false);
    expect_eq(store.remove("evt_live_1"), true);
    expect_eq(store.remove("evt_live_1"), false);
    expect_eq(store.len(), 2 as usize);
}

fn testCapWebhookDynamicMount(): !void {
    const alloc = std.heap.page_allocator;
    const payload = "{\"ok\":true}";
    const ts: i64 = 1700005000;

    // 1. cap.webhook.StandardWebhook
    const std_hdr = cap.webhook.StandardWebhook.sign(alloc, "mount_sec", "msg_mount", ts, payload)?;
    defer alloc.free(std_hdr);
    cap.webhook.StandardWebhook.verify("mount_sec", "msg_mount", ts, payload, std_hdr, ts, 60)?;

    // 2. cap.webhook.GitHubWebhook
    const gh_hdr = cap.webhook.GitHubWebhook.signSha256(alloc, "mount_gh", payload)?;
    defer alloc.free(gh_hdr);
    expect_eq(cap.webhook.GitHubWebhook.verifySha256("mount_gh", payload, gh_hdr), true);

    // 3. cap.webhook.StripeWebhook & WebhookError
    const st_hdr = cap.webhook.StripeWebhook.signHeader(alloc, "mount_st", ts, payload)?;
    defer alloc.free(st_hdr);
    cap.webhook.StripeWebhook.verifyHeader("mount_st", payload, st_hdr, ts, 60)?;

    let caught_expired = false;
    cap.webhook.StripeWebhook.verifyHeader("mount_st", payload, st_hdr, ts + 61, 60) catch |e| {
        if (e == cap.webhook.WebhookError.TimestampExpired) caught_expired = true;
    };
    expect_eq(caught_expired, true);

    // 4. cap.webhook.IdempotencyStore
    let dedup = cap.webhook.IdempotencyStore.init(alloc, 2000);
    defer dedup.deinit();
    expect_eq(dedup.checkAndRecord("msg_mount", 100), true);
    expect_eq(dedup.checkAndRecord("msg_mount", 500), false);
    expect_eq(dedup.checkAndRecord("msg_mount", 2100), true);
}

fn main() {
    test("1. StandardWebhook sign/verify, multi-sig key rotation, tampering & replay rejection", () => {
        testStandardWebhookAndRotation()?;
    });
    test("2. GitHubWebhook sha256= sign/verify, official vector & single-byte tampering rejection", () => {
        testGitHubWebhookSha256()?;
    });
    test("3. StripeWebhook t=...,v1= sign/verify, v1 rotation, malformed & replay window guards", () => {
        testStripeWebhookHeaderAndReplay()?;
    });
    test("4. IdempotencyStore first-pass/duplicate block/TTL expiry re-allow & purgeExpired", () => {
        testIdempotencyStoreLifecycle()?;
    });
    test("5. import cap from \"cap\"; cap.webhook Automatic Dynamic Mounting", () => {
        testCapWebhookDynamicMount()?;
    });
}
