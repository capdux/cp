import std from "std";

pub const HashAlgorithm = enum {
    Sha256,
    Sha512,
    Sha1,
    Md5,

    pub fn fromString(str: []const u8): ?HashAlgorithm {
        if (std.mem.eql(u8, str, "sha256") or std.mem.eql(u8, str, "SHA256")) return .Sha256;
        if (std.mem.eql(u8, str, "sha512") or std.mem.eql(u8, str, "SHA512")) return .Sha512;
        if (std.mem.eql(u8, str, "sha1") or std.mem.eql(u8, str, "SHA1")) return .Sha1;
        if (std.mem.eql(u8, str, "md5") or std.mem.eql(u8, str, "MD5")) return .Md5;
        return null;
    }
};

pub const Hash = struct {
    algo: HashAlgorithm = .Sha256,
    state: union {
        sha256: std.crypto.hash.sha2.Sha256,
        sha512: std.crypto.hash.sha2.Sha512,
        sha1: std.crypto.hash.Sha1,
        md5: std.crypto.hash.Md5,
    } = .{ .sha256 = undefined },

    pub fn init(algo: HashAlgorithm) Hash {
        var h = Hash{ .algo = algo };
        switch (algo) {
            .Sha256 => h.state = .{ .sha256 = .init(.{}) },
            .Sha512 => h.state = .{ .sha512 = .init(.{}) },
            .Sha1 => h.state = .{ .sha1 = .init(.{}) },
            .Md5 => h.state = .{ .md5 = .init(.{}) },
        }
        return h;
    }

    pub fn update(this: *Hash, data: []const u8) void {
        switch (this.algo) {
            .Sha256 => this.state.sha256.update(data),
            .Sha512 => this.state.sha512.update(data),
            .Sha1 => this.state.sha1.update(data),
            .Md5 => this.state.md5.update(data),
        }
    }

    // Helper functions for direct hashing omitted for brevity but should be static const functions
};
