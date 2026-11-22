// Source: cp/lib/webauthn/client_data.cp (WebAuthn CollectedClientData JSON parser & verifier)
const std = @import("std");
import {
    WebAuthnError,
    ClientData,
    ExpectedClientData,
} from "./types.cp";

pub fn encodeChallengeB64u(alloc: std.mem.Allocator, raw_challenge: []const u8): WebAuthnError![]u8 {
    const encoder = std.base64.url_safe_no_pad.Encoder;
    const out_len = encoder.calcSize(raw_challenge.len);
    const out = alloc.alloc(u8, out_len) catch return WebAuthnError.OutOfMemory;
    const encoded = encoder.encode(out, raw_challenge);
    if (encoded.len == out.len) {
        return out;
    }
    return out;
}

pub fn constantTimeEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

pub fn parseClientDataJson(alloc: std.mem.Allocator, json_bytes: []const u8): WebAuthnError!ClientData {
    let parsed = std.json.parseFromSlice(std.json.Value, alloc, json_bytes, .{}) catch {
        return WebAuthnError.InvalidClientData;
    };
    defer parsed.deinit();

    switch (parsed.value) {
        .object => {},
        else => return WebAuthnError.InvalidClientData,
    }
    const obj = parsed.value.object;

    const type_val = obj.get("type") orelse return WebAuthnError.InvalidClientData;
    switch (type_val) {
        .string => {},
        else => return WebAuthnError.InvalidClientData,
    }

    const chall_val = obj.get("challenge") orelse return WebAuthnError.InvalidClientData;
    switch (chall_val) {
        .string => {},
        else => return WebAuthnError.InvalidClientData,
    }

    const origin_val = obj.get("origin") orelse return WebAuthnError.InvalidClientData;
    switch (origin_val) {
        .string => {},
        else => return WebAuthnError.InvalidClientData,
    }

    let cross_origin: bool? = null;
    if (co_val := obj.get("crossOrigin")) {
        switch (co_val) {
            .bool => {},
            else => return WebAuthnError.InvalidClientData,
        }
        cross_origin = co_val.bool;
    }

    const type_str = alloc.dupe(u8, type_val.string) catch return WebAuthnError.OutOfMemory;
    errdefer alloc.free(type_str);

    const challenge = alloc.dupe(u8, chall_val.string) catch return WebAuthnError.OutOfMemory;
    errdefer alloc.free(challenge);

    const origin = alloc.dupe(u8, origin_val.string) catch return WebAuthnError.OutOfMemory;

    return ClientData{
        .type_str = type_str,
        .challenge = challenge,
        .origin = origin,
        .cross_origin = cross_origin,
    };
}

pub fn verifyClientData(client_data: ClientData, expected: ExpectedClientData): WebAuthnError!void {
    if (!std.mem.eql(u8, client_data.type_str, expected.expected_type)) {
        return WebAuthnError.TypeMismatch;
    }
    if (!constantTimeEql(client_data.challenge, expected.expected_challenge)) {
        return WebAuthnError.ChallengeMismatch;
    }
    if (!std.mem.eql(u8, client_data.origin, expected.expected_origin)) {
        return WebAuthnError.OriginMismatch;
    }
    if (co := client_data.cross_origin) {
        if (co and !expected.allow_cross_origin) {
            return WebAuthnError.CrossOriginNotAllowed;
        }
    }
}
