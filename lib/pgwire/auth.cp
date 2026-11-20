// Source: cp/lib/pgwire/auth.cp (PostgreSQL MD5 Password Authentication Helper)

const std = @import("std");

pub fn computePgMd5Password(
    alloc: std.mem.Allocator,
    user: []const u8,
    password: []const u8,
    salt: [4]u8,
): ![]u8 {
    let inner_hasher = std.crypto.hash.Md5.init(.{});
    inner_hasher.update(password);
    inner_hasher.update(user);
    let inner_digest: [16]u8 = undefined;
    inner_hasher.final(&inner_digest);

    const inner_hex = std.fmt.bytesToHex(inner_digest, .lower);

    let outer_hasher = std.crypto.hash.Md5.init(.{});
    outer_hasher.update(&inner_hex);
    outer_hasher.update(&salt);
    let outer_digest: [16]u8 = undefined;
    outer_hasher.final(&outer_digest);

    const outer_hex = std.fmt.bytesToHex(outer_digest, .lower);

    let out = try alloc.alloc(u8, 35);
    out[0] = 'm';
    out[1] = 'd';
    out[2] = '5';
    @memcpy(out[3..35], &outer_hex);
    return out;
}
