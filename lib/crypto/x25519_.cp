// Reference Only: Functional overlap with lib/crypto/asymmetric.cp | Kept for comparison/analysis
// Source: cp/lib/x25519/x25519.cp (Pure CP RFC 7748 X25519 ECDH Key Exchange)
const std = @import("std");
const X25519 = std.crypto.dh.X25519;

pub const X25519_KEY_LEN: usize = 32;

pub const X25519Error = error{
    InvalidKeyLength,
    IdentityElement,
    WeakPublicKey,
};

pub const KeyPair = struct {
    public_key: [32]u8,
    secret_key: [32]u8,

    pub fn generate(): KeyPair {
        let seed: [32]u8 = undefined;
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        threaded.io().random(&seed);
        return KeyPair.fromSecret(seed);
    }

    pub fn fromSecret(secret: [32]u8): KeyPair {
        const kp = X25519.KeyPair.generateDeterministic(secret) catch unreachable;
        return KeyPair{
            .public_key = kp.public_key,
            .secret_key = kp.secret_key,
        };
    }

    pub fn computeShared(peer_public: [32]u8): X25519Error![32]u8 {
        return scalarmult(_.secret_key, peer_public);
    }
};

pub fn scalarmult(secret_key: [32]u8, public_key: [32]u8): X25519Error![32]u8 {
    return X25519.scalarmult(secret_key, public_key) catch return X25519Error.IdentityElement;
}

pub fn scalarmultBase(secret_key: [32]u8): X25519Error![32]u8 {
    return X25519.recoverPublicKey(secret_key) catch return X25519Error.IdentityElement;
}

pub fn isAllZero(key: [32]u8): bool {
    let acc: u8 = 0;
    for (b of key) {
        acc |= b;
    }
    return acc == 0;
}
