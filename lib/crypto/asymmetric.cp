// Source: Absorbed from /data/dev/bun/src/boringssl/boringssl.cp | Synced: 2026-11-22T11:07:59Z
// lib/crypto/asymmetric.cp
// Modern Asymmetric Cryptography: Ed25519, X25519 ECDH & ECDSA P-256 (Zero C Dependencies)

pub struct Ed25519KeyPair {
    const SEED_LENGTH: usize = 32;
    const PUBLIC_KEY_LENGTH: usize = 32;
    const SECRET_KEY_LENGTH: usize = 64;
    const SIGNATURE_LENGTH: usize = 64;

    public_key: [32]u8;
    secret_key: [64]u8;

    static fn generate(): Ed25519KeyPair {
        let seed: [32]u8 = undefined;
        while (true) {
            * = std.os.linux.getrandom(&seed, 32, 0);
            const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(seed) catch continue;
            return Ed25519KeyPair{
                public_key: kp.public_key.toBytes(),
                secret_key: kp.secret_key.toBytes(),
            };
        }
    }

    static fn fromSeed(seed: []const u8): Ed25519KeyPair? {
        if (seed.len != 32) return null;
        let s: [32]u8 = undefined;
        @memcpy(&s, seed[0..32]);
        const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(s) catch return null;
        return Ed25519KeyPair{
            public_key: kp.public_key.toBytes(),
            secret_key: kp.secret_key.toBytes(),
        };
    }

    static fn fromSecretKey(secret_key: []const u8): Ed25519KeyPair? {
        if (secret_key.len != 64) return null;
        let sk_bytes: [64]u8 = undefined;
        @memcpy(&sk_bytes, secret_key[0..64]);
        const sk = std.crypto.sign.Ed25519.SecretKey.fromBytes(sk_bytes) catch return null;
        const kp = std.crypto.sign.Ed25519.KeyPair.fromSecretKey(sk) catch return null;
        return Ed25519KeyPair{
            public_key: kp.public_key.toBytes(),
            secret_key: kp.secret_key.toBytes(),
        };
    }

    fn sign(msg: string): [64]u8 {
        const sk = std.crypto.sign.Ed25519.SecretKey.fromBytes(_.secret_key) catch unreachable;
        const kp = std.crypto.sign.Ed25519.KeyPair.fromSecretKey(sk) catch unreachable;
        const sig = kp.sign(msg, null) catch unreachable;
        return sig.toBytes();
    }

    fn signInto(out: *[64]u8, msg: string) {
        const sig = _.sign(msg);
        @memcpy(out, sig[0..64]);
    }

    static fn verify(sig: []const u8, msg: string, pub_key: []const u8): bool {
        if (sig.len != 64 || pub_key.len != 32) return false;
        let sig_bytes: [64]u8 = undefined;
        @memcpy(&sig_bytes, sig[0..64]);
        let pk_bytes: [32]u8 = undefined;
        @memcpy(&pk_bytes, pub_key[0..32]);
        const pk = std.crypto.sign.Ed25519.PublicKey.fromBytes(pk_bytes) catch return false;
        const parsed_sig = std.crypto.sign.Ed25519.Signature.fromBytes(sig_bytes);
        parsed_sig.verify(msg, pk) catch return false;
        return true;
    }

    static fn verifyStrict(sig: []const u8, msg: string, pub_key: []const u8): bool {
        if (sig.len != 64 || pub_key.len != 32) return false;
        let sig_bytes: [64]u8 = undefined;
        @memcpy(&sig_bytes, sig[0..64]);
        let pk_bytes: [32]u8 = undefined;
        @memcpy(&pk_bytes, pub_key[0..32]);
        const pk = std.crypto.sign.Ed25519.PublicKey.fromBytes(pk_bytes) catch return false;
        const parsed_sig = std.crypto.sign.Ed25519.Signature.fromBytes(sig_bytes);
        parsed_sig.verifyStrict(msg, pk) catch return false;
        return true;
    }
}

pub struct X25519KeyPair {
    const SECRET_LENGTH: usize = 32;
    const PUBLIC_KEY_LENGTH: usize = 32;
    const SHARED_LENGTH: usize = 32;
    const SEED_LENGTH: usize = 32;

    public_key: [32]u8;
    secret_key: [32]u8;

    static fn generate(): X25519KeyPair {
        let seed: [32]u8 = undefined;
        * = std.os.linux.getrandom(&seed, 32, 0);
        const kp = std.crypto.dh.X25519.KeyPair.generateDeterministic(seed);
        return X25519KeyPair{
            public_key: kp.public_key,
            secret_key: kp.secret_key,
        };
    }

    static fn fromSecret(sk: []const u8): X25519KeyPair? {
        if (sk.len != 32) return null;
        let sk_bytes: [32]u8 = undefined;
        @memcpy(&sk_bytes, sk[0..32]);
        const kp = std.crypto.dh.X25519.KeyPair.generateDeterministic(sk_bytes);
        return X25519KeyPair{
            public_key: kp.public_key,
            secret_key: kp.secret_key,
        };
    }

    static fn recoverPublicKey(sk: []const u8): ([32]u8)? {
        if (sk.len != 32) return null;
        let sk_bytes: [32]u8 = undefined;
        @memcpy(&sk_bytes, sk[0..32]);
        return std.crypto.dh.X25519.recoverPublicKey(sk_bytes);
    }

    static fn sharedSecret(my_sk: []const u8, peer_pk: []const u8): ([32]u8)? {
        if (my_sk.len != 32 || peer_pk.len != 32) return null;
        let sk_bytes: [32]u8 = undefined;
        @memcpy(&sk_bytes, my_sk[0..32]);
        let pk_bytes: [32]u8 = undefined;
        @memcpy(&pk_bytes, peer_pk[0..32]);
        return std.crypto.dh.X25519.scalarmult(sk_bytes, pk_bytes) catch return null;
    }

    fn computeShared(peer_pk: []const u8): ([32]u8)? {
        return X25519KeyPair.sharedSecret(_.secret_key[0..], peer_pk);
    }
}

pub struct EcdsaP256KeyPair {
    const SECRET_KEY_LENGTH: usize = 32;
    const PUBLIC_KEY_LENGTH: usize = 33;
    const UNCOMPRESSED_PUBLIC_KEY_LENGTH: usize = 65;
    const SIGNATURE_LENGTH: usize = 64;
    const DER_MAX_LENGTH: usize = 72;

    secret_key: [32]u8;
    public_key: [33]u8;
    uncompressed_public_key: [65]u8;

    static fn generate(): EcdsaP256KeyPair {
        let seed: [32]u8 = undefined;
        while (true) {
            * = std.os.linux.getrandom(&seed, 32, 0);
            const kp = std.crypto.sign.ecdsa.EcdsaP256Sha256.KeyPair.generateDeterministic(seed) catch continue;
            return EcdsaP256KeyPair{
                secret_key: kp.secret_key.toBytes(),
                public_key: kp.public_key.toCompressedSec1(),
                uncompressed_public_key: kp.public_key.toUncompressedSec1(),
            };
        }
    }

    static fn fromSeed(seed: []const u8): EcdsaP256KeyPair? {
        if (seed.len != 32) return null;
        let s: [32]u8 = undefined;
        @memcpy(&s, seed[0..32]);
        const kp = std.crypto.sign.ecdsa.EcdsaP256Sha256.KeyPair.generateDeterministic(s) catch return null;
        return EcdsaP256KeyPair{
            secret_key: kp.secret_key.toBytes(),
            public_key: kp.public_key.toCompressedSec1(),
            uncompressed_public_key: kp.public_key.toUncompressedSec1(),
        };
    }

    static fn fromSecret(sk: []const u8): EcdsaP256KeyPair? {
        if (sk.len != 32) return null;
        let sk_bytes: [32]u8 = undefined;
        @memcpy(&sk_bytes, sk[0..32]);
        const parsed_sk = std.crypto.sign.ecdsa.EcdsaP256Sha256.SecretKey.fromBytes(sk_bytes) catch return null;
        const kp = std.crypto.sign.ecdsa.EcdsaP256Sha256.KeyPair.fromSecretKey(parsed_sk) catch return null;
        return EcdsaP256KeyPair{
            secret_key: kp.secret_key.toBytes(),
            public_key: kp.public_key.toCompressedSec1(),
            uncompressed_public_key: kp.public_key.toUncompressedSec1(),
        };
    }

    fn sign(msg: string): [64]u8 {
        const parsed_sk = std.crypto.sign.ecdsa.EcdsaP256Sha256.SecretKey.fromBytes(_.secret_key) catch unreachable;
        const kp = std.crypto.sign.ecdsa.EcdsaP256Sha256.KeyPair.fromSecretKey(parsed_sk) catch unreachable;
        const sig = kp.sign(msg, null) catch unreachable;
        return sig.toBytes();
    }

    fn signDer(msg: string): []u8 {
        const parsed_sk = std.crypto.sign.ecdsa.EcdsaP256Sha256.SecretKey.fromBytes(_.secret_key) catch unreachable;
        const kp = std.crypto.sign.ecdsa.EcdsaP256Sha256.KeyPair.fromSecretKey(parsed_sk) catch unreachable;
        const sig = kp.sign(msg, null) catch unreachable;
        let der_buf: [72]u8 = undefined;
        const der_slice = sig.toDer(&der_buf);
        const out = std.heap.page_allocator.alloc(u8, der_slice.len) catch unreachable;
        @memcpy(out, der_slice);
        return out;
    }

    static fn verify(sig: []const u8, msg: string, pk: []const u8): bool {
        if (sig.len != 64) return false;
        let sig_bytes: [64]u8 = undefined;
        @memcpy(&sig_bytes, sig[0..64]);
        const parsed_pk = std.crypto.sign.ecdsa.EcdsaP256Sha256.PublicKey.fromSec1(pk) catch return false;
        const parsed_sig = std.crypto.sign.ecdsa.EcdsaP256Sha256.Signature.fromBytes(sig_bytes);
        parsed_sig.verify(msg, parsed_pk) catch return false;
        return true;
    }

    static fn verifyDer(der_sig: []const u8, msg: string, pk: []const u8): bool {
        const parsed_pk = std.crypto.sign.ecdsa.EcdsaP256Sha256.PublicKey.fromSec1(pk) catch return false;
        const parsed_sig = std.crypto.sign.ecdsa.EcdsaP256Sha256.Signature.fromDer(der_sig) catch return false;
        parsed_sig.verify(msg, parsed_pk) catch return false;
        return true;
    }
}
