// Official BLAKE3 Test Vectors & Streaming / Merkle-Tree / Keyed / KDF / XOF Regression Suite
import {
    OUT_LEN,
    KEY_LEN,
    BLOCK_LEN,
    CHUNK_LEN,
    Blake3Hasher,
    hash,
    keyedHash,
    deriveKey,
    hashEql,
    toHex,
} from "../../packages/@cap/blake3/src/index.cp";

fn assert(cond: bool) {
    if (!cond) {
        @panic("BLAKE3 test assertion failed");
    }
}

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn bytesToHex(bytes: []const u8, out: []u8): []const u8 {
    const digits: [16]u8 = [
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    ];
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        const b = bytes[i];
        out[i * 2] = digits[usize((b >> 4) & 0x0F)];
        out[i * 2 + 1] = digits[usize(b & 0x0F)];
    }
    return out[0..bytes.len * 2];
}

fn assertDigestHex(digest: [32]u8, expected_hex: string) {
    let hex_buf: [64]u8 = [0; 64];
    const got = toHex(digest, hex_buf[0..]);
    assert(sliceEql(got, expected_hex));
}

fn fillPattern(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) : (i += 1) {
        buf[i] = (i % 251) as u8;
    }
}

fn officialTestKey(): [32]u8 {
    // Official BLAKE3 test vector key: "whats the Elvish word for friend"
    const s = "whats the Elvish word for friend";
    let key: [32]u8 = [0; 32];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        key[i] = s[i];
    }
    return key;
}

fn testConstantsAndBasicVectors() {
    assert(OUT_LEN == 32);
    assert(KEY_LEN == 32);
    assert(BLOCK_LEN == 64);
    assert(CHUNK_LEN == 1024);

    // 1. Empty input ""
    const d_empty = hash("");
    assertDigestHex(d_empty, "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262");

    // 2. Single zero byte [0]
    const one_zero: [1]u8 = [0];
    const d_zero = hash(one_zero[0..]);
    assertDigestHex(d_zero, "2d3adedff11b61f14c886e35afa036736dcd87a74d27b5c1510225d0f592e213");

    // 3. 3-byte input "abc"
    const d_abc = hash("abc");
    assertDigestHex(d_abc, "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85");

    // 4. Constant-time comparison hashEql
    assert(hashEql(d_empty, hash("")));
    assert(hashEql(d_abc, hash("abc")));
    assert(!hashEql(d_empty, d_abc));

    let mutated = d_abc;
    mutated[31] ^= 0x01;
    assert(!hashEql(d_abc, mutated));
    mutated[31] ^= 0x01;
    assert(hashEql(d_abc, mutated));
}

fn testMultiChunkMerkleVectors() {
    // Official BLAKE3 pattern input: buf[i] = i % 251
    let buf: [10000]u8 = [0; 10000];
    fillPattern(buf[0..]);

    // Single block boundary (64B) & cross-block (65B)
    assertDigestHex(hash(buf[0..64]), "4eed7141ea4a5cd4b788606bd23f46e212af9cacebacdc7d1f4c6dc7f2511b98");
    assertDigestHex(hash(buf[0..65]), "de1e5fa0be70df6d2be8fffd0e99ceaa8eb6e8c93a63f2d8d1c30ecb6b263dee");

    // Exact 1 chunk (1024B)
    assertDigestHex(hash(buf[0..1024]), "42214739f095a406f3fc83deb889744ac00df831c10daa55189b5d121c855af7");

    // 2 chunks (1025B) -> first Merkle parent merge
    assertDigestHex(hash(buf[0..1025]), "d00278ae47eb27b34faecf67b4fe263f82d5412916c1ffd97c8cb7fb814b8444");

    // Exact 2 chunks (2048B)
    assertDigestHex(hash(buf[0..2048]), "e776b6028c7cd22a4d0ba182a8bf62205d2ef576467e838ed6f2529b85fba24a");

    // 3 chunks (2049B) -> asymmetric Merkle tree
    assertDigestHex(hash(buf[0..2049]), "5f4d72f40d7a5f82b15ca2b2e44b1de3c2ef86c426c95c1af0b6879522563030");

    // Exact 4 chunks (4096B) -> multi-level CV stack collapse
    assertDigestHex(hash(buf[0..4096]), "015094013f57a5277b59d8475c0501042c0b642e531b0a1c8f58d2163229e969");

    // 10 chunks (10000B) -> deep Merkle tree merge across 10 chunks
    assertDigestHex(hash(buf[0..10000]), "5f81f9e4ab67627b6b036d5d4e3bc40d9d3daa6fcc2b6dd07ab2bbf0a877da54");
}

fn testStreamingAndReset() {
    let buf: [4096]u8 = [0; 4096];
    fillPattern(buf[0..]);

    const expected_2049 = hash(buf[0..2049]);
    const expected_4096 = hash(buf[0..4096]);

    // Irregular chunked streaming updates (1, 7, 63, 64, 137, 513, 1025, ...)
    let hasher = Blake3Hasher.init();
    const steps: [8]usize = [1, 7, 63, 64, 137, 513, 1025, 239];
    let pos: usize = 0;
    let s: usize = 0;
    while (s < 8) : (s += 1) {
        const n = steps[s];
        hasher.update(buf[pos..pos + n]);
        pos += n;
    }
    assert(pos == 2049);

    // Non-destructive finalize: calling finalize twice yields identical results
    const d1 = hasher.finalize();
    const d2 = hasher.finalize();
    assert(hashEql(d1, expected_2049));
    assert(hashEql(d2, expected_2049));

    // Continue updating after finalize up to 4096 bytes
    hasher.update(buf[2049..4096]);
    const d_full = hasher.finalize();
    assert(hashEql(d_full, expected_4096));

    // Reset and reuse for "abc"
    hasher.reset();
    hasher.update("a");
    hasher.update("bc");
    assertDigestHex(hasher.finalize(), "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85");
}

fn testKeyedAndDeriveKey() {
    const key = officialTestKey();
    let buf: [2049]u8 = [0; 2049];
    fillPattern(buf[0..]);

    // Keyed hash (MAC) official vectors
    assertDigestHex(
        keyedHash(key, ""),
        "92b2b75604ed3c761f9d6f62392c8a9227ad0ea3f09573e783f1498a4ed60d26",
    );
    assertDigestHex(
        keyedHash(key, buf[0..2049]),
        "9f29700902f7c86e514ddc4df1e3049f258b2472b6dd5267f61bf13983b78dd5",
    );

    // Streaming keyed hasher + reset preserves key & mode
    let kh = Blake3Hasher.initKeyed(key);
    kh.update(buf[0..1000]);
    kh.update(buf[1000..2049]);
    assert(hashEql(kh.finalize(), keyedHash(key, buf[0..2049])));
    kh.reset();
    assert(hashEql(kh.finalize(), keyedHash(key, "")));

    // Key derivation (KDF) official vectors
    const ctx = "BLAKE3 2019-12-27 16:29:52 test vectors context";
    assertDigestHex(
        deriveKey(ctx, ""),
        "2cc39783c223154fea8dfb7c1b1660f2ac2dcbd1c1de8277b0b0dd39b7e50d7d",
    );
    assertDigestHex(
        deriveKey(ctx, buf[0..2049]),
        "2ea477c5515cc3dd606512ee72bb3e0e758cfae7232826f35fb98ca1bcbdf273",
    );

    // Streaming KDF + reset preserves derived context key
    let dh = Blake3Hasher.initDeriveKey(ctx);
    dh.update(buf[0..1024]);
    dh.update(buf[1024..2049]);
    assert(hashEql(dh.finalize(), deriveKey(ctx, buf[0..2049])));
    dh.reset();
    assert(hashEql(dh.finalize(), deriveKey(ctx, "")));
}

fn testXofOutputs() {
    // 1. 64-byte XOF on "abc"
    let h1 = Blake3Hasher.init();
    h1.update("abc");
    let xof64: [64]u8 = [0; 64];
    h1.finalizeXof(xof64[0..]);
    let hex128: [128]u8 = [0; 128];
    const got64 = bytesToHex(xof64[0..], hex128[0..]);
    assert(sliceEql(
        got64,
        "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d851fb250ae7393f5d02813b65d521a0d492d9ba09cf7ce7f4cffd900f23374bf0b",
    ));

    // 2. 128-byte XOF on 2049-byte multi-chunk pattern
    let buf: [2049]u8 = [0; 2049];
    fillPattern(buf[0..]);
    let h2 = Blake3Hasher.init();
    h2.update(buf[0..2049]);
    let xof128: [128]u8 = [0; 128];
    h2.finalizeXof(xof128[0..]);
    let hex256: [256]u8 = [0; 256];
    const got128 = bytesToHex(xof128[0..], hex256[0..]);
    assert(sliceEql(
        got128,
        "5f4d72f40d7a5f82b15ca2b2e44b1de3c2ef86c426c95c1af0b687952256303096de31d71d74103403822a2e0bc1eb193e7aecc9643a76b7bbc0c9f9c52e8783aae98764ca468962b5c2ec92f0c74eb5448d519713e09413719431c802f948dd5d90425a4ecdadece9eb178d80f26efccae630734dff63340285adec2aed3b51",
    ));

    // 3. toHex truncation safety when output buffer is smaller than 64 bytes
    let small_hex: [16]u8 = [0; 16];
    const short_str = toHex(hash("abc"), small_hex[0..]);
    assert(sliceEql(short_str, "6437b3ac38465133"));
}

pub fn main() {
    testConstantsAndBasicVectors();
    testMultiChunkMerkleVectors();
    testStreamingAndReset();
    testKeyedAndDeriveKey();
    testXofOutputs();
}
