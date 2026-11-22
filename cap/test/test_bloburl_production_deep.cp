// Source: cp/cap/test/test_bloburl_production_deep.cp (Production deep tests for cap:bloburl)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BLOB_URL_MIN_LEN,
    UUID_STR_LEN,
    BlobUuid,
    BlobUrlEntry,
    BlobUrlRegistry,
    isBlobUrl,
    parseBlobUrl,
    formatBlobUrl,
} from "cap:bloburl";

test("1. BlobUuid parse/format round-trip and case-insensitive hex handling", () => {
    const upper_uuid = "550E8400-E29B-41D4-A716-446655440000";
    const lower_uuid = "550e8400-e29b-41d4-a716-446655440000";

    const u1 = BlobUuid.parse(upper_uuid) ?? unreachable;
    const u2 = BlobUuid.parse(lower_uuid) ?? unreachable;
    expect_eq(u1.eql(u2), true);
    expect_eq(u1.bytes[0], 0x55);
    expect_eq(u1.bytes[15], 0x00);

    let fmt_buf: [40]u8 = std.mem.zeroes([40]u8);
    let fmt_s: []u8 = fmt_buf[0..];
    const n = u1.format(&fmt_s) ?? unreachable;
    expect_eq(n, UUID_STR_LEN);
    expect_eq(std.mem.eql(u8, fmt_buf[0..n], lower_uuid), true);
});

test("2. parseBlobUrl & formatBlobUrl support both Bun blob:<uuid> and W3C blob:<origin>/<uuid>", () => {
    const bun_url = "blob:550e8400-e29b-41d4-a716-446655440000";
    expect_eq(bun_url.len, BLOB_URL_MIN_LEN);
    expect_eq(isBlobUrl(bun_url), true);

    const p1 = parseBlobUrl(bun_url) ?? unreachable;
    expect_eq(p1.origin.len, 0);

    const w3c_url = "blob:https://example.com:8443/550e8400-e29b-41d4-a716-446655440000";
    expect_eq(isBlobUrl(w3c_url), true);
    const p2 = parseBlobUrl(w3c_url) ?? unreachable;
    expect_eq(std.mem.eql(u8, p2.origin, "https://example.com:8443"), true);
    expect_eq(p1.uuid.eql(p2.uuid), true);

    let out_buf: [128]u8 = std.mem.zeroes([128]u8);
    let out_s: []u8 = out_buf[0..];
    const f1 = formatBlobUrl("", p1.uuid, &out_s) ?? unreachable;
    expect_eq(std.mem.eql(u8, out_buf[0..f1], bun_url), true);

    const f2 = formatBlobUrl("https://example.com:8443", p2.uuid, &out_s) ?? unreachable;
    expect_eq(std.mem.eql(u8, out_buf[0..f2], w3c_url), true);
});

test("3. BlobUrlRegistry lifecycle: createObjectUrl (RFC 4122 v4), resolve, has, revoke, and slot reuse", () => {
    let slots: [2]BlobUrlEntry = std.mem.zeroes([2]BlobUrlEntry);
    let slots_s: []BlobUrlEntry = slots[0..];
    let reg = BlobUrlRegistry.init(&slots_s, 2026) ?? unreachable;
    expect_eq(reg.activeCount(), 0);

    let u1_buf: [128]u8 = std.mem.zeroes([128]u8);
    let u2_buf: [128]u8 = std.mem.zeroes([128]u8);
    let u3_buf: [128]u8 = std.mem.zeroes([128]u8);
    let u1_s: []u8 = u1_buf[0..];
    let u2_s: []u8 = u2_buf[0..];
    let u3_s: []u8 = u3_buf[0..];

    const n1 = reg.createObjectUrl(101, "application/pdf", 8192, "", &u1_s) ?? unreachable;
    const n2 = reg.createObjectUrl(202, "text/plain", 64, "https://cp.dev", &u2_s) ?? unreachable;
    expect_eq(reg.activeCount(), 2);

    // Registry full (capacity = 2) -> returns null
    expect_eq(reg.createObjectUrl(303, "text/html", 12, "", &u3_s) == null, true);

    // Check RFC 4122 v4 version/variant bits on generated UUID
    const parsed1 = parseBlobUrl(u1_buf[0..n1]) ?? unreachable;
    expect_eq((parsed1.uuid.bytes[6] & 0xF0), 0x40);
    expect_eq((parsed1.uuid.bytes[8] & 0xC0), 0x80);

    // Resolve & verify metadata
    const e1 = reg.resolve(u1_buf[0..n1]) ?? unreachable;
    expect_eq(e1.payload_id, 101);
    expect_eq(e1.size, 8192);
    expect_eq(std.mem.eql(u8, e1.content_type, "application/pdf"), true);
    expect_eq(reg.has(u2_buf[0..n2]), true);

    // Revoke u1 -> idempotent second revoke returns false -> slot reused for u3
    expect_eq(reg.revoke(u1_buf[0..n1]), true);
    expect_eq(reg.revoke(u1_buf[0..n1]), false);
    expect_eq(reg.has(u1_buf[0..n1]), false);
    expect_eq(reg.activeCount(), 1);

    const n3 = reg.createObjectUrl(303, "image/webp", 512, "", &u3_s) ?? unreachable;
    expect_eq(reg.activeCount(), 2);
    const e3 = reg.resolve(u3_buf[0..n3]) ?? unreachable;
    expect_eq(e3.payload_id, 303);
});

test("4. Deterministic UUID generation with identical seed", () => {
    let s1_arr: [4]BlobUrlEntry = std.mem.zeroes([4]BlobUrlEntry);
    let s2_arr: [4]BlobUrlEntry = std.mem.zeroes([4]BlobUrlEntry);
    let s1: []BlobUrlEntry = s1_arr[0..];
    let s2: []BlobUrlEntry = s2_arr[0..];

    let r1 = BlobUrlRegistry.init(&s1, 0xCAFEBABE) ?? unreachable;
    let r2 = BlobUrlRegistry.init(&s2, 0xCAFEBABE) ?? unreachable;

    let b1: [64]u8 = std.mem.zeroes([64]u8);
    let b2: [64]u8 = std.mem.zeroes([64]u8);
    let b1_s: []u8 = b1[0..];
    let b2_s: []u8 = b2[0..];

    const l1 = r1.createObjectUrl(1, "text/plain", 10, "", &b1_s) ?? unreachable;
    const l2 = r2.createObjectUrl(1, "text/plain", 10, "", &b2_s) ?? unreachable;
    expect_eq(l1, l2);
    expect_eq(std.mem.eql(u8, b1[0..l1], b2[0..l2]), true);
});

test("5. Defensive guards: malformed UUIDs, bad prefixes, empty pool, and short buffers", () => {
    expect_eq(isBlobUrl("http://example.com/550e8400-e29b-41d4-a716-446655440000"), false);
    expect_eq(isBlobUrl("blob:short"), false);
    expect_eq(parseBlobUrl("blob:550e8400e29b41d4a716446655440000") == null, true); // missing hyphens
    expect_eq(parseBlobUrl("blob:550e8400-e29b-41d4-a716-44665544000g") == null, true); // invalid hex 'g'
    expect_eq(parseBlobUrl("blob:/550e8400-e29b-41d4-a716-446655440000") == null, true); // empty origin before '/'

    let empty_slots: [0]BlobUrlEntry = [];
    let empty_s: []BlobUrlEntry = empty_slots[0..];
    expect_eq(BlobUrlRegistry.init(&empty_s, 1) == null, true);

    let slots: [2]BlobUrlEntry = std.mem.zeroes([2]BlobUrlEntry);
    let slots_s: []BlobUrlEntry = slots[0..];
    let reg = BlobUrlRegistry.init(&slots_s, 1) ?? unreachable;
    let tiny: [16]u8 = std.mem.zeroes([16]u8);
    let tiny_s: []u8 = tiny[0..];
    expect_eq(reg.createObjectUrl(1, "text/plain", 1, "", &tiny_s) == null, true);
    expect_eq(reg.activeCount(), 0);
});
